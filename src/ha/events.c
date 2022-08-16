/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "events.h"
#include "utils/freelist.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ha_events, LOG_LEVEL_INF);

#define HA_EV_TRIG_BLOCK_COUNT 8u

/* Trig structure can be allocated from any thread, so we need to protect it */
K_MEM_SLAB_DEFINE(trig_slab, sizeof(struct ha_event_trigger), HA_EV_TRIG_BLOCK_COUNT, 4);

K_MUTEX_DEFINE(trig_mutex);
static sys_dlist_t trig_dlist = SYS_DLIST_STATIC_INIT(&trig_dlist);

static struct ha_event_trigger *trig_alloc(void)
{
	static struct ha_event_trigger *trig;

	/* "mem" set to NULL on k_mem_slab_alloc(,,K_NO_WAIT) error,
	 * but don't want to suffer from an API change */
	if (k_mem_slab_alloc(&trig_slab, (void **)&trig, K_NO_WAIT) != 0) {
		trig = NULL;
	} else {
		LOG_DBG("Trig %p allocated", trig);
	}

	return trig;
}

static void trig_free(struct ha_event_trigger *trig)
{
	__ASSERT_NO_MSG(trig != NULL);

	/* Same comment */
	k_mem_slab_free(&trig_slab, (void **)&trig);
	LOG_DBG("Trig %p freed", trig);
}

static void trig_init(struct ha_event_trigger *trig)
{
	k_fifo_init(&trig->evq);
	trig->func = NULL;
	trig->flags = 0u;
}

#define HA_EV_COUNT 16u

K_MEM_SLAB_DEFINE(ev_slab, sizeof(struct ha_event), HA_EV_COUNT, 4);

static struct ha_event *ha_event_alloc(void)
{
	static struct ha_event *ev;

	/* Same comment */
	if (k_mem_slab_alloc(&ev_slab, (void **)&ev, K_NO_WAIT) != 0) {
		ev = NULL;
	} else {
		LOG_DBG("Event %p allocated", ev);
	}

	return ev;
}

struct ha_event *ha_event_alloc_and_reset(void)
{
	struct ha_event *ev = ha_event_alloc();

	if (ev != NULL) {
		memset(ev, 0, sizeof(struct ha_event));
	}

	return ev;
}

void ha_event_free(struct ha_event *ev)
{
	__ASSERT_NO_MSG(ev != NULL);
	__ASSERT_NO_MSG(atomic_get(&ev->ref_count) == (atomic_val_t)0);

	/* Deallocate event data buffer */
	k_free(ev->data);

	/* Same comment */
	k_mem_slab_free(&ev_slab, (void **)&ev);
	LOG_DBG("Event %p freed", ev);
}

uint32_t ha_event_free_count(void)
{
	return k_mem_slab_num_free_get(&ev_slab);
}

void ha_event_ref(struct ha_event *event)
{
	__ASSERT_NO_MSG(event != NULL);
	
	atomic_inc(&event->ref_count);
}

void ha_event_unref(struct ha_event *event)
{
	if (event != NULL) {
		/* If last reference, free the event */
		if (atomic_dec(&event->ref_count) == (atomic_val_t)1) {
			/* Deallocate event buffer */
			ha_event_free(event);
		}
	}
}

/**
 * @brief Notify the event to the event queue if it is listening for it
 * 
 * @param trig 
 * @param event 
 * @return int 1 if notified, 0 if not, negative value on error
 */
static int event_notify_single(struct ha_event_trigger *trig,
			       struct ha_event *event)
{
	int ret = 0;

	/* TODO Validate match */
	bool match = true;

	if (match) {
		ha_event_ref(event);

		ret = k_fifo_alloc_put(&trig->evq, event);
		
		if (ret == 0) {
			ret = 1;
		}
	}

	return ret;
}

int ha_event_notify_all(struct ha_event *event)
{
	int ret = 0, notified = 0;
	struct ha_event_trigger *_dnode, *trig;

	k_mutex_lock(&trig_mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&trig_dlist, trig, _dnode, _handle) {
		ret = event_notify_single(trig, event);

		if (ret == 1) {
			notified++;
		} else if (ret < 0) {
			break;
		}
	}
	k_mutex_unlock(&trig_mutex);

	if (ret < 0) {
		LOG_ERR("Failed to notify event %p to %p, err=%d",
			event, trig, ret);
	} else {
		ret = notified;
	}

	return ret;
}

int ha_event_subscribe(const ha_ev_subs_conf_t *sub,
		       struct ha_event_trigger **trig)
{
	int ret;
	struct ha_event_trigger *ptrig = NULL;

	/* TODO validate tconf */
	ARG_UNUSED(sub);

	if (trig == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	ptrig = trig_alloc();
	if (ptrig == NULL) {
		ret = -ENOMEM;
		goto exit;
	}

	trig_init(ptrig);

	/* Configure the trigger with tconf */

	/* prefer irq_lock() to mutex here */
	k_mutex_lock(&trig_mutex, K_FOREVER);
	sys_dlist_append(&trig_dlist, &ptrig->_handle);
	k_mutex_unlock(&trig_mutex);

	/* Mark as subscribed */
	atomic_set_bit(&ptrig->flags, HA_EV_TRIG_FLAG_SUBSCRIBED);

	*trig = ptrig;

	ret = 0;
exit:
	if (ret != 0) {
		trig_free(ptrig);
	}
	return ret;
}

int ha_event_unsubscribe(struct ha_event_trigger *trig)
{
	int ret = -EINVAL;

	if ((trig != NULL) &&
	    atomic_test_and_clear_bit(&trig->flags, HA_EV_TRIG_FLAG_SUBSCRIBED)) {
		k_mutex_lock(&trig_mutex, K_FOREVER);
		sys_dlist_remove(&trig->_handle);
		k_mutex_unlock(&trig_mutex);

		trig_free(trig);

		ret = 0;
	}

	return ret;
}