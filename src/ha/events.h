/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_EVENTS_H_
#define _HA_EVENTS_H_

#include <stdio.h>

#include <zephyr.h>

#include "devices.h"

typedef enum
{
	HA_EVENT_TYPE_DATA = 0u,
	// HA_EVENT_TYPE_CONTROL = 1,
	HA_EVENT_TYPE_COMMAND = 2u,
	HA_EVENT_TYPE_ERROR = 3u,
} ha_event_type_t;

typedef struct ha_event {

	/* Event time */
	uint32_t time;

	/* Event type */
	ha_event_type_t type;

	/* Number of times the event is referenced
	 * If ref_count is 0, the event data can be freed */
	atomic_val_t ref_count;

	/* Flags */
	uint32_t isbroadcast: 1u;

	/* Device the event is related to */
	ha_dev_t *dev;

	/* Event data */
	void *data;
} ha_event_t;

ha_event_t *ha_event_alloc_and_reset(void);

void ha_event_free(ha_event_t *ev);

uint32_t ha_event_free_count(void);

void ha_event_ref(ha_event_t *event);

void ha_event_unref(ha_event_t *event);

#define HA_EV_TRIG_FLAG_SUBSCRIBED 0u

#define HA_EV_TRIG_DEVICE_DATA 3u
#define HA_EV_TRIG_DEVICE_COMMAND 4u
#define HA_EV_TRIG_DEVICE_ERROR 5u
#define HA_EV_TRIG_FUNCTION 6u
#define HA_EV_TRIG_PARAMS 7u

typedef bool (*event_trigger_func_t)(ha_event_t *event);

typedef struct ha_event_trigger
{
	sys_dnode_t _handle;

	/* Queue of events to be notified to the waiter */
	struct k_fifo evq;

	/* Flag describing on which event should the waiter be notified */
	atomic_t flags;

	union {
		event_trigger_func_t func;
		void *params;
	};

} ha_ev_trig_t;

#define HA_EV_SUBS_FLAG_ALL BIT(0u)

typedef struct ha_ev_subs_conf
{
	uint32_t flags;
} ha_ev_subs_conf_t;

/**
 * @brief Notify waiters of an event
 * 
 * @param event 
 * @return int 
 */
int ha_event_notify_all(ha_event_t *event);

int ha_event_subscribe(const ha_ev_subs_conf_t *sub,
		       struct ha_event_trigger **trig);

int ha_event_unsubscribe(struct ha_event_trigger *trig);

static inline ha_event_t *ha_event_wait(struct ha_event_trigger *trig,
					k_timeout_t timeout)
{
	__ASSERT_NO_MSG(atomic_test_bit(&trig->flags, HA_EV_TRIG_FLAG_SUBSCRIBED));

	return (ha_event_t *)k_fifo_get(&trig->evq, timeout);
}

#endif /* _HA_EVENTS_H_ */