/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <caniot/datatype.h>

#include "devices.h"
#include "net_time.h"
#include "config.h"
#include "system.h"

#include "devices/caniot.h"
#include "devices/f429zi.h"
#include "devices/garage.h"
#include "devices/xiaomi.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ha_dev, LOG_LEVEL_WRN);

// #define HA_DEVICES_IGNORE_UNVERIFIED_DEVICES 1

#define HA_EV_SUBS_BLOCK_COUNT 8u
#define HA_EV_COUNT 16u

/* Forward declaration */

static struct ha_event *ha_ev_alloc_and_reset(void);
static void ha_ev_free(struct ha_event *ev);

struct {
	struct k_mutex mutex;
	ha_dev_t list[HA_MAX_DEVICES];
	uint8_t count;
} devices = {
	.mutex = Z_MUTEX_INITIALIZER(devices.mutex),
	.count = 0U
};

typedef int (*mac_cmp_func_t)(const ha_dev_mac_addr_t *a,
			      const ha_dev_mac_addr_t *b);

static int internal_caniot_addr_cmp(const ha_dev_mac_addr_t *a,
				    const ha_dev_mac_addr_t *b)
{
	return caniot_deviceid_cmp(a->caniot, b->caniot);
}

static int internal_ble_addr_cmp(const ha_dev_mac_addr_t *a,
				 const ha_dev_mac_addr_t *b)
{
	return bt_addr_le_cmp(&a->ble, &b->ble);
}

static mac_cmp_func_t get_mac_cmp_func(ha_dev_medium_type_t medium)
{
	switch (medium) {
	case HA_DEV_MEDIUM_BLE:
		return internal_ble_addr_cmp;
	case HA_DEV_MEDIUM_CAN:
		return internal_caniot_addr_cmp;
	default:
		return NULL;
	}	
}

static bool mac_valid(const ha_dev_mac_t *mac)
{
	return get_mac_cmp_func(mac->medium) != NULL;
}

static int mac_cmp(const ha_dev_mac_t *m1, const ha_dev_mac_t *m2)
{
	int ret = -EINVAL;

	mac_cmp_func_t cmpf = get_mac_cmp_func(m1->medium);

	if (cmpf != NULL) {
		ret = cmpf(&m1->addr, &m2->addr);
	}

	return ret;
}

static ha_dev_t *get_device_by_mac(const ha_dev_mac_t *mac)
{
	ha_dev_t *device = NULL;

	/* Get MAC compare function */
	mac_cmp_func_t cmp = get_mac_cmp_func(mac->medium);

	if (cmp != NULL) {
		for (ha_dev_t *dev = devices.list;
		     dev < devices.list + devices.count;
		     dev++) {
			/* Device medium should match and
			 * MAC address should match */
			if ((dev->addr.mac.medium == mac->medium) &&
			    (cmp(&dev->addr.mac.addr, &mac->addr) == 0)) {
				device = dev;
				break;
			}
		}
	}

	return device;
}

static ha_dev_t *get_first_device_by_type(ha_dev_type_t type)
{
	ha_dev_t *device = NULL;

	for (ha_dev_t *dev = devices.list;
	     dev < devices.list + devices.count;
	     dev++) {
		if (dev->addr.type == type) {
			device = dev;
			break;
		}
	}

	return device;
}

// static bool ha_dev_valid(ha_dev_t *const dev)
// {
// 	return (dev != NULL) && (mac_valid(&dev->addr.mac) == true);
// }

ha_dev_t *ha_dev_get_by_addr(const ha_dev_addr_t *addr)
{
	ha_dev_t *device = NULL;

	if (mac_valid(&addr->mac)) {
		/* Get device by address if possible */
		device = get_device_by_mac(&addr->mac);
	} else {
		/* if medium type is not set, device should be
		* differienciated using their device_type */
		device = get_first_device_by_type(addr->type);
	}

	return device;
}

int ha_dev_addr_cmp(const ha_dev_addr_t *a,
		    const ha_dev_addr_t *b)
{
	if (mac_valid(&a->mac) && mac_valid(&b->mac)) {
		return mac_cmp(&a->mac, &b->mac);
	} else {
		return a->type - b->type;
	}
}

static void ha_dev_clear(ha_dev_t *dev)
{
	memset(dev, 0U, sizeof(*dev));
}

extern const struct ha_device_api ha_device_api_xiaomi;
extern const struct ha_device_api ha_device_api_caniot;
extern const struct ha_device_api ha_device_api_f429zi;

static const struct ha_device_api *ha_device_get_default_api(ha_dev_type_t type)
{
	switch (type) {
	case HA_DEV_TYPE_XIAOMI_MIJIA:
		return &ha_device_api_xiaomi;
	case HA_DEV_TYPE_CANIOT:
		return &ha_device_api_caniot;
	case HA_DEV_TYPE_NUCLEO_F429ZI:
		return &ha_device_api_f429zi;
	default:
		return NULL;
	}
}

/**
 * @brief Register a new device in the list, addr duplicates are not verified
 *
 * @param medium
 * @param type
 * @param addr
 * @return int
 */
ha_dev_t *ha_dev_register(const ha_dev_addr_t *addr)
{
	ha_dev_t *dev = NULL;
	k_mutex_lock(&devices.mutex, K_FOREVER);

	if (devices.count >= ARRAY_SIZE(devices.list)) {
		goto exit;
	}

	/* Get default api */
	const struct ha_device_api *api = ha_device_get_default_api(addr->type);
	
	if (api == NULL) {
		LOG_WRN("(addr %p) Unknown device type %d",
			addr, addr->type);
		goto exit;
	}

	/* Allocate memory */
	dev = devices.list + devices.count;

	ha_dev_clear(dev);

	dev->addr = *addr;
	dev->registered_timestamp = sys_time_get();
	dev->api = api;

	if ((api != NULL) && (api->on_registration != NULL)) {
		if (api->on_registration(addr) != true) {
			LOG_WRN("(addr %p) Device registration refused", addr);
			dev = NULL;
			goto exit;
		}
	}

	/* Increment device count */
	devices.count++;

	/* Reference the room */
	if ((dev->room = ha_dev_get_room(dev)) != NULL) {
		atomic_inc(&dev->room->devices_count);
	}

exit:
	k_mutex_unlock(&devices.mutex);
	return dev;
}

static bool ha_dev_match_filter(ha_dev_t *dev, const ha_dev_filter_t *filter)
{
	if (dev == NULL) {
		return false;
	}

	if (filter == NULL) {
		return true;
	}

	if (filter->flags == 0u) {
		return true;
	}

	if (filter->flags & HA_DEV_FILTER_MEDIUM) {
		if (dev->addr.mac.medium != filter->medium) {
			return false;
		}
	}

	if (filter->flags & HA_DEV_FILTER_DEVICE_TYPE) {
		if (dev->addr.type != filter->device_type) {
			return false;
		}
	}

	if (filter->flags & HA_DEV_FILTER_DATA_EXIST) {
		if (dev->last_data_event == NULL) {
			return false;
		}
	}

	if (filter->flags & HA_DEV_FILTER_DATA_TIMESTAMP) {
		if (dev->last_data_event->time < filter->data_timestamp) {
			return false;
		}
	}

	if (((filter->flags & HA_DEV_FILTER_ROOM_ID) != 0) && dev->room) {
		if (dev->room->rid != filter->rid) {
			return false;
		}
	}

	return true;
}




size_t ha_dev_iterate(void (*callback)(ha_dev_t *dev,
				       void *user_data),
		      const ha_dev_filter_t *filter,
		      void *user_data)
{
	k_mutex_lock(&devices.mutex, K_FOREVER);

	size_t count = 0U;

	for (ha_dev_t *dev = devices.list;
	     dev < devices.list + devices.count;
	     dev++) {
		if (ha_dev_match_filter(dev, filter) == true) {
			if (dev->last_data_event != NULL) {
				/* Reference device event in case the callback wants to
				* access it.
				* Even the callback may reference the event itself,
				* if it needs to access it after the callback returns
				*/
				ha_ev_ref(dev->last_data_event);
				callback(dev, user_data);
				ha_ev_unref(dev->last_data_event);
			} else {
				callback(dev, user_data);
			}
			count++;
		}
	}

	k_mutex_unlock(&devices.mutex);

	return count;
}

static int device_process_data(ha_dev_t *dev,
			       const void *data,
			       size_t data_len,
			       uint32_t timestamp)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(dev->api != NULL);
	__ASSERT_NO_MSG(data != NULL);

	int ret = -EINVAL;
	ha_ev_t *ev = NULL;
	struct ha_xiaomi_dataset *odata = NULL;

	k_mutex_lock(&devices.mutex, K_FOREVER);

	/* Allocate memory */
	ev = ha_ev_alloc_and_reset();
	if (!ev) {
		LOG_ERR("(%p) Failed to allocate ha_ev_t", dev);
		goto exit;
	}

	ev->dev = dev;
	ev->type = HA_EV_TYPE_DATA;
	ev->time = timestamp ? timestamp : sys_time_get();

	size_t odata_size = dev->api->get_internal_format_size(dev, data, data_len);
	LOG_DBG("(%p) get_internal_format_size(%p, %p, %u) = %u",
		ev, dev, data, data_len, odata_size);
	if (odata_size != 0) {
		/* Allocate buffer to handle data to be converted */
		odata = k_malloc(odata_size);
		if (!odata) {
			LOG_ERR("(%p) Failed to allocate data req len=%u",
				dev, odata_size);
			goto exit;
		}

		/* Convert data to internal format */
		LOG_DBG("(%p) convert_data(%p, %p, %u, %p, %u, *%u)",
			ev, dev, data, data_len, odata, odata_size, ev->time);
		if (dev->api->convert_data(dev, data, data_len, odata, 
					   odata_size, &ev->time) != true) {
			LOG_ERR("(%p) Conversion failed", dev);
			goto exit;
		}
	}

	ev->data = odata;

	/* If there is a previous data event is referenced here, unref it */
	ha_ev_t *const prev_data_ev = dev->last_data_event;
	if (prev_data_ev != NULL) {
		dev->last_data_event = NULL;
		ha_ev_unref(prev_data_ev);
	}

	/* Update statistics */
	ha_dev_inc_stats_rx(dev, odata_size);

	/* Reference current event */
	atomic_set(&ev->ref_count, 1u);
	dev->last_data_event = ev;	

	/* Notify the event to listeners */
	ret = ha_ev_notify_all(ev);

	LOG_INF("Event %p notified to %d listeners", ev, ret);

exit:
	/* Free event on error or if not referenced anymore */
	if ((ev != NULL) && ((ret < 0) || (atomic_get(&ev->ref_count) == 0u))) {
		ha_ev_free(ev);
	}

	k_mutex_unlock(&devices.mutex);

	return ret;
}

int ha_dev_register_data(const ha_dev_addr_t *addr,
			 const void *data,
			 size_t data_len,
			 uint32_t timestamp)
{
	ha_dev_t *dev;
	int ret = -ENOMEM;

	dev = ha_dev_get_by_addr(addr);

	if (dev == NULL) {
		dev = ha_dev_register(addr);
	}

	if (dev != NULL) {
		ret = device_process_data(dev, data, data_len, timestamp);
	}

	return ret;
}

const void *ha_dev_get_last_data(ha_dev_t *dev)
{
	if (dev != NULL) {
		return ha_ev_get_data(dev->last_data_event);
	}

	return NULL;
}

/* Subscription structure can be allocated from any thread, so we need to protect it */
K_MEM_SLAB_DEFINE(sub_slab, sizeof(struct ha_ev_subs), HA_EV_SUBS_BLOCK_COUNT, 4);

K_MUTEX_DEFINE(sub_mutex);
static sys_dlist_t sub_dlist = SYS_DLIST_STATIC_INIT(&sub_dlist);

static struct ha_ev_subs *sub_alloc(void)
{
	static struct ha_ev_subs *sub;

	/* "mem" set to NULL on k_mem_slab_alloc(,,K_NO_WAIT) error,
	 * but don't want to suffer from an API change */
	if (k_mem_slab_alloc(&sub_slab, (void **)&sub, K_NO_WAIT) != 0) {
		sub = NULL;
	} else {
		LOG_DBG("Sub %p allocated", sub);
	}

	return sub;
}

static void sub_free(struct ha_ev_subs *sub)
{
	__ASSERT_NO_MSG(sub != NULL);

	/* Same comment */
	k_mem_slab_free(&sub_slab, (void **)&sub);
	LOG_DBG("Sub %p freed", sub);
}

static void sub_init(struct ha_ev_subs *sub)
{
	k_fifo_init(&sub->evq);
	sub->func = NULL;
	sub->flags = 0u;
	sub->on_queued = NULL;
}

K_MEM_SLAB_DEFINE(ev_slab, sizeof(struct ha_event), HA_EV_COUNT, 4);

static struct ha_event *ha_ev_alloc(void)
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

static struct ha_event *ha_ev_alloc_and_reset(void)
{
	struct ha_event *ev = ha_ev_alloc();

	if (ev != NULL) {
		memset(ev, 0, sizeof(struct ha_event));
	}

	return ev;
}

static void ha_ev_free(struct ha_event *ev)
{
	__ASSERT_NO_MSG(ev != NULL);
	__ASSERT_NO_MSG(atomic_get(&ev->ref_count) == (atomic_val_t)0);

	/* Deallocate event data buffer */
	k_free(ev->data);

	/* Same comment */
	k_mem_slab_free(&ev_slab, (void **)&ev);
	LOG_DBG("Event %p freed", ev);
}

uint32_t ha_ev_free_count(void)
{
	return k_mem_slab_num_free_get(&ev_slab);
}

void ha_ev_ref(struct ha_event *event)
{
	__ASSERT_NO_MSG(event != NULL);
	
	atomic_inc(&event->ref_count);
}

void ha_ev_unref(struct ha_event *event)
{
	if (event != NULL) {
		/* If last reference, free the event */
		if (atomic_dec(&event->ref_count) == (atomic_val_t)1) {
			/* Deallocate event buffer */
			ha_ev_free(event);
		}
	}
}

static bool event_match_sub(struct ha_ev_subs *sub,
			    struct ha_event *event)
{
	if (sub->flags & HA_EV_SUBS_DEVICE_TYPE) {
		if (event->dev->addr.type != sub->device_type) {
			return false;
		}
	}

	if (sub->flags & HA_EV_SUBS_DEVICE_ADDR) {
		if (mac_cmp(&sub->device_addr, &event->dev->addr.mac) != 0) {
			return false;
		}
	}

	if (sub->flags & HA_EV_SUBS_DEVICE_DATA) {
		if (event->type != HA_EV_TYPE_DATA) {
			return false;
		}
	}

	if (sub->flags & HA_EV_SUBS_DEVICE_COMMAND) {
		if (event->type != HA_EV_TYPE_COMMAND) {
			return false;
		}
	}

	if (sub->flags & HA_EV_SUBS_DEVICE_ERROR) {
		if (event->type != HA_EV_TYPE_ERROR) {
			return false;
		}
	}

	return true;
}

/**
 * @brief Notify the event to the subscription event queue if it is listening for it
 * 
 * @param sub 
 * @param event 
 * @return int 1 if notified, 0 if not, negative value on error
 */
static int event_notify_single(struct ha_ev_subs *sub,
			       struct ha_event *event)
{
	int ret = 0;

	/* Validate match */
	bool match = event_match_sub(sub, event);

	if (match) {
		/* Reference the event, the subscriber will have to unref it 
		 * when it will don't need it anymore */
		ha_ev_ref(event);

		/* TODO: Find a way to use a regular k_fifo_put() 
		 * i.e. without k_malloc() */
		ret = k_fifo_alloc_put(&sub->evq, event);
		
		if (ret == 0) {
			/* call event function hook */
			if (sub->flags & HA_EV_SUBS_ON_QUEUED_HOOK) {
				__ASSERT_NO_MSG(sub->on_queued != NULL);
				sub->on_queued(sub, event);
			}
			ret = 1;
		} else {
			/* Unref on error */
			ha_ev_unref(event);

			LOG_ERR("k_fifo_alloc_put() error %d", ret);
		}
	}

	return ret;
}

int ha_ev_notify_all(struct ha_event *event)
{
	int ret = 0, notified = 0;
	struct ha_ev_subs *_dnode, *sub;

	k_mutex_lock(&sub_mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&sub_dlist, sub, _dnode, _handle) {
		ret = event_notify_single(sub, event);

		if (ret == 1) {
			notified++;
		} else if (ret < 0) {
			break;
		}
	}
	k_mutex_unlock(&sub_mutex);

	if (ret < 0) {
		LOG_ERR("Failed to notify event %p to %p, err=%d",
			event, sub, ret);
	} else {
		ret = notified;
	}

	return ret;
}

/**
 * @brief Validate subscription is valid
 * 
 * @param sub 
 */
static bool subscription_conf_validate(const ha_ev_subs_conf_t *conf)
{
	if (conf == NULL) {
		return false;
	}

	if (conf->flags & HA_EV_SUBS_DEVICE_ADDR) {
		if (conf->device_addr == 0) {
			return false;
		}
	}

	if (conf->flags & HA_EV_SUBS_FUNCTION) {
		if (conf->func == NULL) {
			return false;
		}
	}

	/* TODO check for conflicting flags */

	return true;
}

int ha_ev_subscribe(const ha_ev_subs_conf_t *conf,
		    struct ha_ev_subs **sub)
{
	int ret;
	struct ha_ev_subs *psub = NULL;

	/* validate tconf */

	if (!subscription_conf_validate(conf)) {
		ret = -EINVAL;
		goto exit;
	}

	psub = sub_alloc();
	if (psub == NULL) {
		ret = -ENOMEM;
		goto exit;
	}

	sub_init(psub);

	/* Configure the sub structure with conf */
	psub->flags = conf->flags;
	psub->device_type = conf->device_type;
	if (conf->flags & HA_EV_SUBS_DEVICE_ADDR) {
		memcpy(&psub->device_addr, conf->device_addr, sizeof(ha_dev_mac_t));
	}

	/* Set on_queued hook */
	if (conf->flags & HA_EV_SUBS_ON_QUEUED_HOOK) {
		psub->on_queued = conf->on_queued;
	}

	/* prefer irq_lock() to mutex here */
	k_mutex_lock(&sub_mutex, K_FOREVER);
	sys_dlist_append(&sub_dlist, &psub->_handle);
	k_mutex_unlock(&sub_mutex);

	/* Mark as subscribed */
	atomic_set_bit(&psub->flags, HA_EV_SUBS_FLAG_SUBSCRIBED_BIT);

	*sub = psub;

	ret = 0;
exit:
	if (ret != 0) {
		sub_free(psub);
	}
	return ret;
}

int ha_ev_unsubscribe(struct ha_ev_subs *sub)
{
	int ret = -EINVAL;

	if ((sub != NULL) &&
	    atomic_test_and_clear_bit(&sub->flags, HA_EV_SUBS_FLAG_SUBSCRIBED_BIT)) {
		k_mutex_lock(&sub_mutex, K_FOREVER);
		sys_dlist_remove(&sub->_handle);
		k_mutex_unlock(&sub_mutex);

		/* TODO how to cancel all threads waiting on this sub ? */
		k_fifo_cancel_wait(&sub->evq);

		sub_free(sub);

		ret = 0;
	}

	return ret;
}

ha_ev_t *ha_ev_wait(struct ha_ev_subs *sub,
		    k_timeout_t timeout)
{
	if ((sub != NULL) && HA_EV_SUBS_SUBSCRIBED(sub)) {
		return (ha_ev_t *)k_fifo_get(&sub->evq, timeout);
	}
	return NULL;
}

const void *ha_ev_get_data(const ha_ev_t *event)
{
	if (event != NULL) {
		return event->data;
	}
	return NULL;
}

const void *ha_ev_get_data_check_type(const ha_ev_t *event,
					 ha_dev_type_t expected_type)
{
	if ((event != NULL) &&
	    (event->dev != NULL) &&
	    (expected_type == event->dev->addr.type)) {
		return event->data;
	}
	return NULL;
}

struct ha_room *ha_dev_get_room(ha_dev_t *const dev)
{
	struct ha_room_assoc *assoc = NULL;
	struct ha_room *room = NULL;

	/* Find the room associated to the device */
	for (uint32_t i = 0u; i < ha_cfg_rooms_assoc_count; i++) {
		if (ha_dev_addr_cmp(&dev->addr, &ha_cfg_rooms_assoc[i].addr) == 0) {
			assoc = &ha_cfg_rooms_assoc[i];
			break;
		}
	}

	if (assoc != NULL) {
		/* Retrieve room structure addr */
		for (uint32_t i = 0u; i < ha_cfg_rooms_count; i++) {
			if (assoc->rid == ha_cfg_rooms[i].rid) {
				room = &ha_cfg_rooms[i];
				break;
			}
		}
	}

	return room;
}