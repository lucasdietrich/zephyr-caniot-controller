/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <stdio.h>
#include <string.h>

#include <caniot/datatype.h>

#include "drivers/can.h"

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
#define HA_EV_COUNT 32u

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

static uint32_t dev_get_index(ha_dev_t *dev)
{
	return (uint32_t)(dev - devices.list);
}

typedef int (*addr_cmp_func_t)(const ha_dev_mac_addr_t *a,
			      const ha_dev_mac_addr_t *b);

typedef int (*addr_str_func_t)(const ha_dev_mac_addr_t *a,
			      char *str, size_t len);

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


static int internal_can_addr_cmp(const ha_dev_mac_addr_t *a,
				 const ha_dev_mac_addr_t *b)
{
	const uint32_t id_a = a->can.id & (a->can.ext ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK);
	const uint32_t id_b = b->can.id & (b->can.ext ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK);

	/* TODO what to do if ext is different? */

	return id_a - id_b;
}

static int internal_ble_addr_str(const ha_dev_mac_addr_t *a,
				 char *str, size_t len)
{
	return bt_addr_le_to_str(&a->ble, str, len);
}

static int internal_caniot_addr_str(const ha_dev_mac_addr_t *a,
				    char *str, size_t len)
{
	return snprintf(str, len, "0x%x (cls=%u did=%u)", a->caniot,
			CANIOT_DID_CLS(a->caniot), CANIOT_DID_SID(a->caniot));
}

static int internal_can_addr_str(const ha_dev_mac_addr_t *a,
				 char *str, size_t len)
{
	const uint32_t id = a->can.id & (a->can.ext ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK);

	return snprintf(str, len, "%u", id);
}

struct mac_funcs {
	addr_cmp_func_t cmp;
	addr_str_func_t str;
};

#define MAC_FUNCS_CANIOT 0u
#define MAC_FUNCS_BLE 1u

static const struct mac_funcs mac_medium_funcs[] = {
	[HA_DEV_MEDIUM_CAN] = {.cmp = internal_can_addr_cmp, .str = internal_can_addr_str },
	[HA_DEV_MEDIUM_BLE] = {.cmp = internal_ble_addr_cmp, .str = internal_ble_addr_str },
};

/* Overload the medium mac address functions */
static const struct mac_funcs mac_type_funcs[] = {
	[HA_DEV_TYPE_CANIOT] = { .cmp = internal_caniot_addr_cmp, .str = internal_caniot_addr_str },
};


static addr_cmp_func_t get_mac_medium_cmp_func(ha_dev_medium_type_t medium)
{
	if (medium >= ARRAY_SIZE(mac_medium_funcs)) {
		return NULL;
	}

	return mac_medium_funcs[medium].cmp;
}

static addr_cmp_func_t get_addr_cmp_func(ha_dev_type_t type, ha_dev_medium_type_t medium)
{
	addr_cmp_func_t func = NULL;

	if (type < ARRAY_SIZE(mac_type_funcs)) {
		func = mac_type_funcs[type].cmp;
	}

	if (!func) {
		func = get_mac_medium_cmp_func(medium);
	}

	return func;
}

static addr_str_func_t get_mac_medium_str_func(ha_dev_medium_type_t medium)
{
	if (medium >= ARRAY_SIZE(mac_medium_funcs)) {
		return NULL;
	}

	return mac_medium_funcs[medium].str;
}

static addr_str_func_t get_addr_str_func(ha_dev_type_t type, ha_dev_medium_type_t medium)
{
	addr_str_func_t func = NULL;

	if (type < ARRAY_SIZE(mac_type_funcs)) {
		func = mac_type_funcs[type].str;
	}

	if (!func) {
		func = get_mac_medium_str_func(medium);
	}

	return func;
}

int ha_dev_addr_to_str(const ha_dev_addr_t *addr,
		       char *buf,
		       size_t buf_len)
{
	if (!buf || !buf_len) {
		return -EINVAL;
	}

	addr_str_func_t str_func = get_addr_str_func(addr->type, addr->mac.medium);

	if (str_func == NULL) {
		*buf = '\0';
		return -ENOTSUP;
	}

	return str_func(&addr->mac.addr, buf, buf_len);
}

static bool addr_valid(const ha_dev_addr_t *addr)
{
	return (addr->type != HA_DEV_TYPE_NONE) &&
		(get_addr_cmp_func(addr->type, addr->mac.medium) != NULL);
}

static int addr_cmp(const ha_dev_addr_t *a1, const ha_dev_addr_t *a2)
{
	addr_cmp_func_t cmpf = get_addr_cmp_func(a1->type, a1->mac.medium);

	if (cmpf == NULL) {
		return -ENOTSUP;
	}

	return cmpf(&a1->mac.addr, &a2->mac.addr);
}

static ha_dev_t *get_device_by_addr(const ha_dev_addr_t *addr)
{
	ha_dev_t *device = NULL;

	/* Get MAC compare function */
	addr_cmp_func_t cmp = get_addr_cmp_func(addr->type, addr->mac.medium);

	if (cmp != NULL) {
		for (ha_dev_t *dev = devices.list;
		     dev < devices.list + devices.count;
		     dev++) {
			/* Device medium should match and
			 * MAC address should match */
			if ((dev->addr.mac.medium == addr->mac.medium) &&
			    (cmp(&dev->addr.mac.addr, &addr->mac.addr) == 0)) {
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

ha_dev_t *ha_dev_get_by_addr(const ha_dev_addr_t *addr)
{
	ha_dev_t *device = NULL;

	if (addr_valid(addr)) {
		/* Get device by address if possible */
		device = get_device_by_addr(addr);
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
	if (addr_valid(a) && addr_valid(b)) {
		return addr_cmp(a, b);
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

	/* Allocate memory */
	dev = devices.list + devices.count;

	ha_dev_clear(dev);

	dev->addr = *addr;
	dev->registered_timestamp = sys_time_get();
	
	dev->api = ha_device_get_default_api(addr->type);
	if (dev->api == NULL) {
		LOG_WRN("No api for device type %p", addr);
		goto exit;
	}

	/* Clear endpoints */
	for (int i = 0; i < HA_DEV_ENDPOINT_MAX_COUNT; i++) {
		dev->endpoints[i] = NULL;
	}

	const int ret = dev->api->init_endpoints(&dev->addr,
						 dev->endpoints,
						 &dev->endpoints_count);
	if (ret < 0) {
		LOG_ERR("Failed to register device %p", addr);
		goto exit;
	}

	if (dev->endpoints_count > HA_DEV_ENDPOINT_MAX_COUNT) {
		LOG_ERR("Too many endpoints (%hhu) defined for device addr %p", 
			dev->endpoints_count, addr);
		goto exit;
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

	if (filter->flags & HA_DEV_FILTER_FROM_INDEX) {
		if (dev_get_index(dev) < filter->from_index) {
			return false;
		}
	}

	if (filter->flags & HA_DEV_FILTER_TO_INDEX) {
		if (dev_get_index(dev) >= filter->to_index) {
			return false;
		}
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

	struct ha_event *ev = NULL;

	if (filter->flags & HA_DEV_FILTER_DATA_EXIST) {
		if (filter->endpoint < dev->endpoints_count) {
			ev = dev->endpoints[filter->endpoint]->last_data_event;
			if (!ev || !ev->data) {
				return false;
			}
		}
	}

	if (filter->flags & HA_DEV_FILTER_DATA_TIMESTAMP) {
		if (ev && ev->timestamp < filter->data_timestamp) {
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


size_t ha_dev_iterate(ha_dev_iterate_cb_t callback,
		      const ha_dev_filter_t *filter,
		      void *user_data)
{
	size_t count = 0U;
	const uint32_t devices_count = devices.count;

	for (ha_dev_t *dev = devices.list;
	     dev < devices.list + devices_count;
	     dev++) {
		if (ha_dev_match_filter(dev, filter) == true) {

			/* ????? TODO HOW TO ?????
			 * 
			 * Reference device event in case the callback wants to
			 * access it.
			 * Even the callback may reference the event itself,
			 * if it needs to access it after the callback returns
			 */
			bool zcontinue = callback(dev, user_data);

			count++;

			if (!zcontinue) {
				break;
			}
		}
	}

	return count;
}

int ha_dev_get_index(ha_dev_t *dev)
{
	if (!dev) {
		return -EINVAL;
	}
	
	return (int) dev_get_index(dev);
}

static int device_process_data(ha_dev_t *dev,
			       struct ha_dev_payload *pl)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(dev->api != NULL);
	__ASSERT_NO_MSG(pl->buffer != NULL);

	int ret = -EINVAL;
	ha_ev_t *ev = NULL;

	/* Allocate memory */
	ev = ha_ev_alloc_and_reset();
	if (!ev) {
		LOG_ERR("(%p) Failed to allocate ha_ev_t", dev);
		goto exit;
	}

	ev->data = NULL;
	ev->dev = dev;
	ev->type = HA_EV_TYPE_DATA;
	ev->timestamp = pl->timestamp ? pl->timestamp : sys_time_get();
	sys_slist_init(&ev->slist);

	/* Find endpoint */
	uint8_t ep_index = 0u;
	if (dev->api->select_endpoint != NULL) {
		ret = dev->api->select_endpoint(&dev->addr, pl);
		if (ret < 0) {
			LOG_ERR("(%p) Failed to select endpoint", dev);
			goto exit;
		}
		ep_index = (uint8_t) ret;
	}

	if (ep_index >= MIN(dev->endpoints_count, HA_DEV_ENDPOINT_MAX_COUNT)) {
		LOG_ERR("(%p) Invalid payloadid %u", dev, ep_index);
		goto exit;
	}

	struct ha_device_endpoint *ep = dev->endpoints[ep_index];
	if (ep->eid == HA_DEV_ENDPOINT_NONE) {
		LOG_ERR("(%p) %u is not a valid endpoint", dev, ep_index);
		goto exit;
	}

	if (ep->expected_payload_size && (ep->expected_payload_size != pl->len)) {
		LOG_ERR("(%p) Invalid payload size %u, expected %u for endpoint %u",
			dev, pl->len, ep->expected_payload_size, ep_index);
		goto exit;
	}

	/* Allocate buffer to handle data to be converted */
	ev->data = k_malloc(ep->data_size);
	if (!ev->data) {
		LOG_ERR("(%p) Failed to allocate data req len=%u",
			dev, ep->data_size);
		goto exit;
	}

	/* Convert data to internal format */
	ret = ep->ingest(ev, pl);
	if (ret != 0) {
		LOG_ERR("(%p) Conversion failed reason=%d", dev, ret);
		goto exit;
	}

	/* If there is a previous data event is referenced here, unref it */
	ha_ev_t *const prev_data_ev = ep->last_data_event;
	if (prev_data_ev != NULL) {
		ep->last_data_event = NULL;
		ha_ev_unref(prev_data_ev);
	}

	/* Update statistics */
	ha_dev_inc_stats_rx(dev, ep->data_size);

	/* Reference current event */
	atomic_set(&ev->ref_count, 1u);
	ep->last_data_event = ev;	

	/* Notify the event to listeners */
	ret = ha_ev_notify_all(ev);

	LOG_INF("Event %p notified to %d listeners", ev, ret);

exit:
	/* Free event on error or if not referenced anymore */
	if ((ev != NULL) && ((ret < 0) || (atomic_get(&ev->ref_count) == 0u))) {
		ha_ev_free(ev);
	}

	return ret;
}

int ha_dev_register_data(const ha_dev_addr_t *addr,
			 const void *payload,
			 size_t payload_len,
			 uint32_t timestamp,
			 void *y)
{
	ha_dev_t *dev;
	int ret = -ENOMEM;

	dev = ha_dev_get_by_addr(addr);

	if (dev == NULL) {
		dev = ha_dev_register(addr);
	}

	struct ha_dev_payload pl = {
		.timestamp = timestamp,
		.buffer = payload,
		.len = payload_len,
		.y = y,
	};

	if (dev != NULL) {
		ret = device_process_data(dev, &pl);
	}

	return ret;
}


ha_ev_t *ha_dev_command(const ha_dev_addr_t *addr,
			ha_dev_cmd_t *cmd,
			k_timeout_t timeout)
{
	return NULL;
}

ha_ev_t *ha_dev_get_last_event(ha_dev_t *dev, ha_endpoint_id_t ep)
{
	if (dev && (ep < dev->endpoints_count)) {
		return dev->endpoints[ep]->last_data_event;
	}

	return NULL;
}

const void *ha_dev_get_last_event_data(ha_dev_t *dev, ha_endpoint_id_t ep)
{
	ha_ev_t *ev = ha_dev_get_last_event(dev, ep);
	
	if (ev) {
		return ev->data;
	} else {
		return NULL;
	}
}

/* Subscription structure can be allocated from any thread, so we need to protect it */
K_MEM_SLAB_DEFINE(sub_slab, sizeof(struct ha_ev_subs), HA_EV_SUBS_BLOCK_COUNT, 4);

K_MUTEX_DEFINE(sub_mutex);
static sys_dlist_t sub_dlist = SYS_DLIST_STATIC_INIT(&sub_dlist);

static struct ha_ev_subs *sub_alloc(void)
{
	static struct ha_ev_subs *sub;

	if (k_mem_slab_alloc(&sub_slab, (void **)&sub, K_NO_WAIT) == 0) {
		LOG_DBG("Sub %p allocated", sub);
	}

	return sub;
}

static void sub_free(struct ha_ev_subs *sub)
{
	if (sub != NULL) {
		k_mem_slab_free(&sub_slab, (void **)&sub);
		LOG_DBG("Sub %p freed", sub);
	}
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
		ha_dev_addr_t addr = {
			.type = sub->device_type,
			.mac = sub->device_mac
		};
		if (addr_cmp(&addr, &event->dev->addr) != 0) {
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
			if (sub->flags & HA_EV_SUBS_FLAG_ON_QUEUED_HOOK) {
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
		if ((conf->device_mac == NULL) ||
		    (conf->device_type == HA_DEV_TYPE_NONE)) {
			return false;
		}
	}

	if (conf->flags & HA_EV_SUBS_FUNCTION) {
		if (conf->func == NULL) {
			return false;
		}
	}

	if (conf->flags & HA_EV_SUBS_FLAG_ON_QUEUED_HOOK) {
		if (conf->on_queued == NULL) {
			return false;
		}
	} else if (conf->on_queued != NULL) {
		LOG_WRN("on_queued hook set (%p) but HA_EV_SUBS_FLAG_ON_QUEUED_HOOK "
			"flag is missing",
			conf->on_queued);
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

	if (!subscription_conf_validate(conf) || !sub) {
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
		memcpy(&psub->device_mac, conf->device_mac, sizeof(ha_dev_mac_t));
	}

	/* Set on_queued hook */
	if (conf->flags & HA_EV_SUBS_FLAG_ON_QUEUED_HOOK) {
		psub->on_queued = conf->on_queued;
	}
	
	/* prefer k_spin_lock() to mutex here */
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
	if (sub == NULL) {
		return -EINVAL;
	}

	if (atomic_test_and_clear_bit(&sub->flags, HA_EV_SUBS_FLAG_SUBSCRIBED_BIT)) {
		/* prefer k_spin_lock() to mutex here */
		k_mutex_lock(&sub_mutex, K_FOREVER);
		sys_dlist_remove(&sub->_handle);
		k_mutex_unlock(&sub_mutex);

		if (k_fifo_is_empty(&sub->evq)) {
			/* TODO how to cancel all threads waiting on this sub ? */
			k_fifo_cancel_wait(&sub->evq);
		} else {
			/* Empty the fifo if not empty */
			ha_ev_t *ev;
			uint32_t count = 0u;
			while ((ev = k_fifo_get(&sub->evq, K_NO_WAIT)) != NULL) {
				ha_ev_unref(ev);
				count++;
			}
			LOG_WRN("%u events not consumed because of "
				"unsubscription of %p", count, sub);
		}

		sub_free(sub);
	}

	return 0;
}

ha_ev_t *ha_ev_wait(struct ha_ev_subs *sub,
		    k_timeout_t timeout)
{
	if ((sub != NULL) && HA_EV_SUBS_SUBSCRIBED(sub)) {
		return (ha_ev_t *)k_fifo_get(&sub->evq, timeout);
	}
	return NULL;
}

void *ha_ev_get_data(const ha_ev_t *event)
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