/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config.h"
#include "ha.h"
#include "net_time.h"
#include "system.h"

#include <stdio.h>

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <malloc.h>
LOG_MODULE_REGISTER(ha_dev, LOG_LEVEL_INF);

// #define HA_DEVICES_IGNORE_UNVERIFIED_DEVICES 1

/* Forward declaration */

static struct ha_event *ha_ev_alloc_and_reset(void);
static void ha_ev_free(struct ha_event *ev);

struct {
	struct k_mutex mutex;
	ha_dev_t list[HA_DEVICES_MAX_COUNT];
	uint8_t count;
	uint32_t sdevuid; /* Session Device Unique ID reference */
} devices = {
	.mutex	 = Z_MUTEX_INITIALIZER(devices.mutex),
	.count	 = 0u,
	.sdevuid = 1u,
};

#define __DEV_CONTEXT_LOCK()   k_mutex_lock(&devices.mutex, K_FOREVER)
#define __DEV_CONTEXT_UNLOCK() k_mutex_unlock(&devices.mutex)

static struct ha_stats stats = {
	.mem_ev_remaining	  = HA_EVENTS_MAX_COUNT,
	.mem_device_remaining = HA_DEVICES_MAX_COUNT,
	.mem_sub_remaining	  = HA_SUBSCRIPTIONS_MAX_COUNT,
};

typedef int (*addr_cmp_func_t)(const ha_dev_mac_addr_t *a, const ha_dev_mac_addr_t *b);

typedef int (*addr_str_func_t)(const ha_dev_mac_addr_t *a, char *str, size_t len);

static int internal_caniot_addr_cmp(const ha_dev_mac_addr_t *a,
									const ha_dev_mac_addr_t *b)
{
	return caniot_deviceid_cmp(a->caniot, b->caniot);
}

static int internal_ble_addr_cmp(const ha_dev_mac_addr_t *a, const ha_dev_mac_addr_t *b)
{
	return bt_addr_le_cmp(&a->ble, &b->ble);
}

static int internal_can_addr_cmp(const ha_dev_mac_addr_t *a, const ha_dev_mac_addr_t *b)
{
	const uint32_t id_a = a->can.id & (a->can.ext ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK);
	const uint32_t id_b = b->can.id & (b->can.ext ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK);

	/* TODO what to do if ext is different? */

	return id_a - id_b;
}

static int internal_ble_addr_str(const ha_dev_mac_addr_t *a, char *str, size_t len)
{
	return bt_addr_le_to_str(&a->ble, str, len);
}

static int internal_caniot_addr_str(const ha_dev_mac_addr_t *a, char *str, size_t len)
{
	return snprintf(str, len, "0x%x (cls=%u did=%u)", a->caniot,
					CANIOT_DID_CLS(a->caniot), CANIOT_DID_SID(a->caniot));
}

static int internal_can_addr_str(const ha_dev_mac_addr_t *a, char *str, size_t len)
{
	const uint32_t id = a->can.id & (a->can.ext ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK);

	return snprintf(str, len, "%u", id);
}

struct mac_funcs {
	addr_cmp_func_t cmp;
	addr_str_func_t str;
};

#define MAC_FUNCS_CANIOT 0u
#define MAC_FUNCS_BLE	 1u

static const struct mac_funcs mac_medium_funcs[] = {
	[HA_DEV_MEDIUM_CAN] = {.cmp = internal_can_addr_cmp, .str = internal_can_addr_str},
	[HA_DEV_MEDIUM_BLE] = {.cmp = internal_ble_addr_cmp, .str = internal_ble_addr_str},
};

/* Overload the medium mac address functions */
static const struct mac_funcs mac_type_funcs[] = {
	[HA_DEV_TYPE_CANIOT] = {.cmp = internal_caniot_addr_cmp,
							.str = internal_caniot_addr_str},
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

int ha_dev_addr_to_str(const ha_dev_addr_t *addr, char *buf, size_t buf_len)
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
		for (ha_dev_t *dev = devices.list; dev < devices.list + devices.count; dev++) {
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

	for (ha_dev_t *dev = devices.list; dev < devices.list + devices.count; dev++) {
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

int ha_dev_addr_cmp(const ha_dev_addr_t *a, const ha_dev_addr_t *b)
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
static ha_dev_t *ha_dev_register(const ha_dev_addr_t *addr)
{
	int ret;
	ha_dev_t *dev = NULL;

	__DEV_CONTEXT_LOCK();

	if (devices.count >= ARRAY_SIZE(devices.list)) {
		stats.dev_dropped++;
		stats.dev_no_mem++;
		goto exit;
	}

	/* Allocate memory */
	dev = devices.list + devices.count;

	ha_dev_clear(dev);

	dev->sdevuid			  = devices.sdevuid;
	dev->addr				  = *addr;
	dev->registered_timestamp = sys_time_get();

	dev->api = ha_device_get_default_api(addr->type);
	if (dev->api == NULL) {
		stats.dev_dropped++;
		stats.dev_no_api++;
		LOG_WRN("No api for device type %p", addr);
		goto exit;
	}

	ret = dev->api->init_endpoints(&dev->addr, dev->endpoints, &dev->endpoints_count);
	if (ret < 0) {
		stats.dev_dropped++;
		stats.dev_ep_init++;
		LOG_ERR("Failed to register device %p", addr);
		goto exit;
	}

	if (dev->endpoints_count == 0) {
		stats.dev_dropped++;
		stats.dev_ep_init++;
		LOG_WRN("No endpoints for device %p", addr);
		goto exit;
	}

	if (dev->endpoints_count > HA_DEV_EP_MAX_COUNT) {
		stats.dev_dropped++;
		stats.dev_toomuch_ep++;
		LOG_ERR("Too many endpoints (%hhu) defined for device addr %p",
				dev->endpoints_count, addr);
		goto exit;
	}

#if HA_DEV_EP_TYPE_SEARCH_OPTIMIZATION
	/* Finalize endpoints initialization */
	for (int i = 0; i < dev->endpoints_count; i++) {
		dev->endpoints[i]._data_types = ha_data_descr_data_types_mask(
			dev->endpoints[i].cfg->data_descr, dev->endpoints[i].cfg->data_descr_size);
	}
#endif /* HA_DEV_EP_TYPE_SEARCH_OPTIMIZATION */

	/* Increment device count */
	devices.count++;

	/* Use a seperate counter to define a Session Device Unique ID
	 * because in the future, removal of devices could be supported.
	 * Then we need a counter which doesn't get decremented when a device
	 * is removed to guarantee that the "sdevuid" is unique.
	 */
	devices.sdevuid++;

	stats.mem_device_count++;
	stats.mem_device_remaining--;

	/* Reference the room */
	if ((dev->room = ha_dev_get_room(dev)) != NULL) {
		atomic_inc(&dev->room->devices_count);
	}

exit:
	__DEV_CONTEXT_UNLOCK();

	return dev;
}

/* TODO add an argument to have filtering context
 * e.g. what endpoint event has made the match condition valid
 * 	goal is to lock only this endpoint event
 */
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

	/* Note: HA_DEV_FILTER_FROM_INDEX and HA_DEV_FILTER_TO_INDEX
	 * have been moved to ha_dev_iterate() function
	 */

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

	struct ha_device_endpoint *ep = NULL;
	struct ha_event *ev			  = NULL;

	if (filter->flags & HA_DEV_FILTER_DATA_EXIST) {
		if (filter->endpoint_id == HA_DEV_EP_NONE) {
			/* Find first valid event through endpoints */
			for (int i = 0; i < dev->endpoints_count; i++) {
				ep = &dev->endpoints[i];
				if (ep->last_data_event != NULL) {
					break;
				}
			}
		} else {
			/* Find searched endpoint */
			ep = ha_dev_ep_get_by_id(dev, filter->endpoint_id);
		}

		if (!ep || !ep->last_data_event) return false;

		ev = ep->last_data_event;
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

static uint32_t dev_ep_lock_ev_mask(ha_dev_t *dev, uint32_t mask)
{
	uint32_t ep_index	 = 0u;
	uint32_t locked_mask = 0u;
	while (mask) {
		if (ep_index < dev->endpoints_count) {
			if (mask & 1u) {
				ha_ev_t *ev = dev->endpoints[ep_index].last_data_event;
				ha_ev_ref(ev);
				locked_mask |= BIT(ep_index);
			}
			mask >>= 1u;
			ep_index++;
		} else {
			mask = 0u;
		}
	}
	return locked_mask;
}

static void dev_ep_unlock_ev_mask(ha_dev_t *dev, uint32_t locked_mask)
{
	uint32_t ep_index = 0u;
	while (locked_mask) {
		if (locked_mask & 1u) {
			ha_ev_t *ev = dev->endpoints[ep_index].last_data_event;
			ha_ev_unref(ev);
		}
		ep_index++;
		locked_mask >>= 1u;
	}
}

ssize_t ha_dev_iterate(ha_dev_iterate_cb_t callback,
					   const ha_dev_filter_t *filter,
					   const ha_dev_iter_opt_t *options,
					   void *user_data)
{
	size_t count	 = 0u;
	size_t max_count = devices.count;
	ha_dev_t *dev	 = devices.list;
	ha_dev_t *last	 = devices.list + devices.count;

	/* Lock only first endpoint event by default */
	if (options == NULL) {
		options = &HA_DEV_ITER_OPT_DEFAULT();
	}

	/* Take boundaries into account */
	if (filter) {
		if (filter->flags & HA_DEV_FILTER_FROM_INDEX) {
			dev += filter->from_index;
		}

		if (filter->flags & HA_DEV_FILTER_TO_INDEX) {
			/* Never go beyond the end of the list of registered
			 * devices */
			last = MIN(last, devices.list + filter->to_index);
		}

		if (filter->flags & HA_DEV_FILTER_TO_COUNT) {
			max_count = filter->to_count;
		}
	}

	/* Check that we are actually iterating over existing devices */
	if (dev >= last) {
		return -ENOENT;
	}

	__DEV_CONTEXT_LOCK(); /* TODO, evaluate if good idea */

	while (dev < last) {
		if (ha_dev_match_filter(dev, filter) == true) {
			/*
			 * Reference endpoints devices event in case the
			 * callback wants to keep a reference to it/them.
			 */
			/* TODO only lock necessary events and not all */
			const uint32_t locked_mask =
				dev_ep_lock_ev_mask(dev, options->ep_lock_last_ev_mask);

			__DEV_CONTEXT_UNLOCK(); /* TODO, evaluate if good idea
									 */

			/* Mutex should not be locked in application callback
			 * context as it could last a lot of time */
			bool zcontinue = callback(dev, user_data);

			__DEV_CONTEXT_LOCK(); /* TODO, evaluate if good idea */

			dev_ep_unlock_ev_mask(dev, locked_mask);

			count++;

			if (!zcontinue || (count >= max_count)) {
				break;
			}
		}

		/* Fetch next device */
		dev++;
	}

	__DEV_CONTEXT_UNLOCK(); /* TODO, evaluate if good idea */

	return count;
}

static inline void ha_dev_inc_stats_rx(ha_dev_t *dev, uint32_t rx_bytes)
{
	__ASSERT(dev != NULL, "dev is NULL");

	dev->stats.rx_bytes += rx_bytes;
	dev->stats.rx++;
}

static inline void ha_dev_inc_stats_tx(ha_dev_t *dev, uint32_t tx_bytes)
{
	__ASSERT(dev != NULL, "dev is NULL");

	dev->stats.tx_bytes += tx_bytes;
	dev->stats.tx++;
}

static int device_process_data(ha_dev_t *dev, const struct ha_device_payload *pl)
{
	int ret			 = -EINVAL;
	uint8_t ep_index = 0u;
	ha_ev_t *ev		 = NULL;
	ha_ev_t *prev_data_ev;
	const struct ha_device_endpoint_config *ep_cfg;
	struct ha_device_endpoint *ep;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(dev->api != NULL);
	__ASSERT_NO_MSG(pl->buffer != NULL);

	/* Allocate memory */
	ev = ha_ev_alloc_and_reset();
	if (!ev) {
		ret = -ENOMEM;
		dev->stats.err_flags |= HA_DEV_STATS_ERR_FLAG_EV_NO_MEM;
		stats.ev_no_mem++;
		LOG_ERR("(%p) Failed to allocate ha_ev_t", dev);
		goto exit;
	}

	ev->data	  = NULL;
	ev->dev		  = dev;
	ev->type	  = HA_EV_TYPE_DATA;
	ev->timestamp = pl->timestamp ? pl->timestamp : sys_time_get();
	sys_slist_init(&ev->_data_slist);

	/* Find endpoint */
	if (dev->api->select_endpoint != NULL) {
		ret = dev->api->select_endpoint(&dev->addr, pl);
		if (ret < 0) {
			dev->stats.err_flags |= HA_DEV_STATS_ERR_FLAG_EV_NO_EP;
			stats.ev_no_ep++;
			LOG_DBG("(%p) No endpoint for payload", dev);
			goto exit;
		}
		ep_index = (uint8_t)ret;
	}

	if (ep_index >= MIN(dev->endpoints_count, HA_DEV_EP_MAX_COUNT)) {
		dev->stats.err_flags |= HA_DEV_STATS_ERR_FLAG_EV_EP;
		stats.ev_ep++;
		ret = -ENOENT;
		LOG_DBG("(%p) Invalid endpoint %u", dev, ep_index);
		goto exit;
	}

	ep	   = &dev->endpoints[ep_index];
	ep_cfg = ep->cfg;

	if (ep_cfg->eid == HA_DEV_EP_NONE) {
		dev->stats.err_flags |= HA_DEV_STATS_ERR_FLAG_EV_NO_EP;
		stats.ev_no_ep++;
		ret = -ENOENT;
		LOG_DBG("(%p) %u is not a valid endpoint", dev, ep_index);
		goto exit;
	}

	if (ep_cfg->expected_payload_size && (ep_cfg->expected_payload_size != pl->len)) {
		dev->stats.err_flags |= HA_DEV_STATS_ERR_FLAG_EV_PAYLOAD_SIZE;
		stats.ev_payload_size++;
		ret = -ENOTSUP;
		LOG_DBG("(%p) Invalid payload size %u, expected %u for "
				"endpoint %u",
				dev, pl->len, ep_cfg->expected_payload_size, ep_index);
		goto exit;
	}

	if (ep_cfg->data_size) {
		/* Allocate buffer to handle data to be converted if not null */
		ev->data = malloc(ep_cfg->data_size);
		if (ev->data) {
			stats.mem_heap_alloc += ep_cfg->data_size;
			stats.mem_heap_total += ep_cfg->data_size;
			ev->data_size = ep_cfg->data_size;
		} else {
			ret = -ENOMEM;
			dev->stats.err_flags |= HA_DEV_STATS_ERR_FLAG_EV_NO_DATA_MEM;
			stats.ev_no_data_mem++;
			LOG_ERR("(%p) Failed to allocate data req len=%u", dev, ep_cfg->data_size);
			goto exit;
		}
	}

	/* Convert data to internal format */
	ret = ep_cfg->ingest(ev, pl);
	if (ret < 0) {
		dev->stats.err_flags |= HA_DEV_STATS_ERR_FLAG_EV_INGEST;
		stats.ev_ingest++;
		LOG_DBG("(%p) Conversion failed reason=%d", dev, ret);
		goto exit;
	}

	/* If a previous data event is referenced here, unref it */
	prev_data_ev = ep->last_data_event;
	if (prev_data_ev != NULL) {
		ep->last_data_event = NULL;
		ha_ev_unref(prev_data_ev);
	}

	/* Update statistics */
	ha_dev_inc_stats_rx(dev, ep_cfg->data_size);

	atomic_set(&ev->ref_count, 0u);

	if (ep->cfg->flags & HA_DEV_EP_FLAG_RETAIN_LAST_EVENT) {
		ha_ev_ref(ev);
		ep->last_data_event = ev;
	}

	/* Notify the event to listeners */
	ret = ha_ev_notify_all(ev);

	stats.ev++;

	LOG_DBG("Event %p notified to %d listeners", ev, ret);

exit:
	/* Free event on error or if not referenced anymore */
	if (!ev || (ret < 0)) {
		if (ev) {
			ha_ev_free(ev);
		}
		stats.ev_dropped++;
		stats.ev_data_dropped++;
		dev->stats.err_ev++;
	} else if (atomic_get(&ev->ref_count) == 0u) {
		ha_ev_free(ev);
		stats.ev_never_ref++;
	}

	return ret;
}

int ha_dev_register_data(const ha_dev_addr_t *addr,
						 const struct ha_device_payload *payload)
{
	ha_dev_t *dev;
	int ret = -ENOMEM;

	dev = ha_dev_get_by_addr(addr);

	if (dev == NULL) {
		dev = ha_dev_register(addr);
	}

	if (dev != NULL) {
		ret = device_process_data(dev, payload);
	}

	return ret;
}

int ha_dev_command(struct ha_cmd_query *query, ha_ev_t **ev)
{
	return -ENOTSUP;
}

struct ha_device_endpoint *ha_dev_ep_get(ha_dev_t *dev, uint32_t ep_index)
{
	if (!dev || (ep_index >= dev->endpoints_count)) {
		return NULL;
	}

	return &dev->endpoints[ep_index];
}

struct ha_device_endpoint *ha_dev_ep_get_by_id(ha_dev_t *dev, ha_endpoint_id_t eid)
{
	if (!dev) {
		return NULL;
	}

	for (uint8_t i = 0; i < dev->endpoints_count; i++) {
		if (dev->endpoints[i].cfg->eid == eid) {
			return &dev->endpoints[i];
		}
	}

	return NULL;
}

int ha_dev_ep_get_index_by_id(ha_dev_t *dev, ha_endpoint_id_t eid)
{
	if (!dev) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < dev->endpoints_count; i++) {
		if (dev->endpoints[i].cfg->eid == eid) {
			return i;
		}
	}

	return -ENOENT;
}

ha_ev_t *ha_dev_get_last_event(ha_dev_t *dev, uint32_t ep_index)
{
	return ha_dev_ep_get(dev, ep_index)->last_data_event;
}

const void *ha_dev_get_last_event_data(ha_dev_t *dev, uint32_t ep_index)
{
	ha_ev_t *ev = ha_dev_get_last_event(dev, ep_index);

	if (ev) {
		return ev->data;
	} else {
		return NULL;
	}
}

/* Subscription structure can be allocated from any thread, so we need to
 * protect it */
K_MEM_SLAB_DEFINE(sub_slab, sizeof(struct ha_ev_subs), HA_SUBSCRIPTIONS_MAX_COUNT, 4);

K_MUTEX_DEFINE(sub_mutex);
static sys_dlist_t sub_dlist = SYS_DLIST_STATIC_INIT(&sub_dlist);

static struct ha_ev_subs *sub_alloc(void)
{
	static struct ha_ev_subs *sub;

	if (k_mem_slab_alloc(&sub_slab, (void **)&sub, K_NO_WAIT) == 0) {
		stats.mem_sub_count++;
		stats.mem_sub_remaining--;
		LOG_DBG("Sub %p allocated", sub);
	}

	return sub;
}

static void sub_free(struct ha_ev_subs *sub)
{
	if (sub != NULL) {
		k_mem_slab_free(&sub_slab, (void **)&sub);
		stats.mem_sub_count--;
		stats.mem_sub_remaining++;
		LOG_DBG("Sub %p freed", sub);
	}
}

static void sub_init(struct ha_ev_subs *sub)
{
	k_fifo_init(&sub->_evq);
	atomic_set(&sub->_ctrl, 0u);
	sub->conf = NULL;
	sys_dnode_init(&sub->_handle);
}

K_MEM_SLAB_DEFINE(ev_slab, sizeof(struct ha_event), HA_EVENTS_MAX_COUNT, 4);

static struct ha_event *ha_ev_alloc(void)
{
	static struct ha_event *ev;

	/* Same comment */
	if (k_mem_slab_alloc(&ev_slab, (void **)&ev, K_NO_WAIT) != 0) {
		ev = NULL;
	} else {
		stats.mem_ev_count++;
		stats.mem_ev_remaining--;
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
	free(ev->data);

	/* Decrease before freeing */
	stats.mem_heap_alloc -= ev->data_size;

	k_mem_slab_free(&ev_slab, (void **)&ev);

	stats.mem_ev_count--;
	stats.mem_ev_remaining++;
}

uint32_t ha_ev_free_count(void)
{
	return k_mem_slab_num_free_get(&ev_slab);
}

void ha_ev_ref(struct ha_event *event)
{
	if (event) {
		atomic_val_t prev_val = atomic_inc(&event->ref_count);

		LOG_DBG("[ ev %p ref_count %u ++> %u ]", event, (uint32_t)prev_val,
				(uint32_t)(prev_val + 1));
	}
}

void ha_ev_unref(struct ha_event *event)
{
	if (event != NULL) {
		/* If last reference, free the event */
		atomic_val_t prev_val = atomic_dec(&event->ref_count);

		if (prev_val == (atomic_val_t)1) {
			/* Deallocate event buffer */
			ha_ev_free(event);
		}

		LOG_DBG("[ ev %p ref_count %u --> %u ]", event, (uint32_t)prev_val,
				(uint32_t)(prev_val - 1));
	}
}

static bool event_match_sub(struct ha_ev_subs *sub, struct ha_event *event)
{
	const struct ha_ev_subs_conf *const conf = sub->conf;

	if (conf->flags & HA_EV_SUBS_CONF_DEVICE_TYPE) {
		if (event->dev->addr.type != conf->device_type) {
			return false;
		}
	}

	if (conf->flags & HA_EV_SUBS_CONF_DEVICE_ADDR) {
		ha_dev_addr_t addr = {.type = conf->device_type, .mac = conf->device_mac};
		if (addr_cmp(&addr, &event->dev->addr) != 0) {
			return false;
		}
	}

	if (conf->flags & HA_EV_SUBS_CONF_DEVICE_DATA) {
		if (event->type != HA_EV_TYPE_DATA) {
			return false;
		}
	}

	if (conf->flags & HA_EV_SUBS_CONF_DEVICE_COMMAND) {
		if (event->type != HA_EV_TYPE_COMMAND) {
			return false;
		}
	}

	if (conf->flags & HA_EV_SUBS_CONF_DEVICE_ERROR) {
		if (event->type != HA_EV_TYPE_ERROR) {
			return false;
		}
	}

	if (conf->flags & HA_EV_SUBS_CONF_FILTER_FUNCTION) {
		if (conf->filter_cb(sub, event) == false) {
			return false;
		}
	}

	return true;
}

/**
 * @brief Notify the event to the subscription event queue if it is listening
 * for it
 *
 * @param sub
 * @param event
 * @return int 1 if notified, 0 if not, negative value on error
 */
static int event_notify_single(struct ha_ev_subs *sub, struct ha_event *event)
{
	int ret = 0;

	/* Validate match */
	bool match = event_match_sub(sub, event);

	if (match) {
		/* Reference the event, the subscriber will have to unref it
		 * when it will don't need it anymore */
		ha_ev_ref(event);

		k_fifo_put(&sub->_evq, event);

		const struct ha_ev_subs_conf *const conf = sub->conf;

		/* call event function hook */
		if (conf->flags & HA_EV_SUBS_CONF_ON_QUEUED_HOOK) {
			__ASSERT_NO_MSG(conf->on_queued_cb != NULL);
			conf->on_queued_cb(sub, event);
		}
		ret = 1;
	}

	return ret;
}

int ha_ev_notify_all(struct ha_event *event)
{
	int ret = 0, notified = 0;
	struct ha_ev_subs *_dnode, *sub;

	k_mutex_lock(&sub_mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE (&sub_dlist, sub, _dnode, _handle) {
		ret = event_notify_single(sub, event);

		if (ret == 1) {
			notified++;
		} else if (ret < 0) {
			break;
		}
	}
	k_mutex_unlock(&sub_mutex);

	if (ret < 0) {
		LOG_ERR("Failed to notify event %p to %p, err=%d", event, sub, ret);
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

	if (conf->flags & HA_EV_SUBS_CONF_DEVICE_ADDR) {
		if (conf->device_type == HA_DEV_TYPE_NONE) {
			return false;
		}
	}

	if (conf->flags & HA_EV_SUBS_CONF_FILTER_FUNCTION) {
		if (conf->filter_cb == NULL) {
			return false;
		}
	}

	if (conf->flags & HA_EV_SUBS_CONF_ON_QUEUED_HOOK) {
		if (conf->on_queued_cb == NULL) {
			return false;
		}
	} else if (conf->on_queued_cb != NULL) {
		LOG_WRN("on_queued_cb hook set (%p) but "
				"HA_EV_SUBS_CONF_ON_QUEUED_HOOK "
				"flag is missing",
				conf->on_queued_cb);
	}

	/* TODO check for conflicting flags */

	return true;
}

int ha_ev_subs_conf_init(ha_ev_subs_conf_t *conf)
{
	if (conf == NULL) {
		return -EINVAL;
	}

	memset(conf, 0, sizeof(*conf));

	return 0;
}

int ha_subscribe(const ha_ev_subs_conf_t *conf, struct ha_ev_subs **sub)
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

	/* Reference configuration */
	psub->conf = conf;

	/* prefer k_spin_lock() to mutex here */
	k_mutex_lock(&sub_mutex, K_FOREVER);
	sys_dlist_append(&sub_dlist, &psub->_handle);
	k_mutex_unlock(&sub_mutex);

	/* Mark as subscribed */
	atomic_set_bit(&psub->_ctrl, HA_EV_SUBS_FLAG_SUBSCRIBED_BIT);

	*sub = psub;

	LOG_DBG("%p subscribed", *sub);

	ret = 0;
exit:
	if (ret != 0) {
		sub_free(psub);
	}
	return ret;
}

int ha_unsubscribe(struct ha_ev_subs *sub)
{
	if (sub == NULL) {
		return -EINVAL;
	}

	if (atomic_test_and_clear_bit(&sub->_ctrl, HA_EV_SUBS_FLAG_SUBSCRIBED_BIT)) {
		/* prefer k_spin_lock() to mutex here */
		k_mutex_lock(&sub_mutex, K_FOREVER);
		sys_dlist_remove(&sub->_handle);
		k_mutex_unlock(&sub_mutex);

		if (k_fifo_is_empty(&sub->_evq)) {
			/* TODO how to cancel all threads waiting on this sub ?
			 */
			k_fifo_cancel_wait(&sub->_evq);
		} else {
			/* Empty the fifo if not empty */
			ha_ev_t *ev;
			uint32_t count = 0u;
			while ((ev = k_fifo_get(&sub->_evq, K_NO_WAIT)) != NULL) {
				ha_ev_unref(ev);
				count++;
			}
			LOG_WRN("%u events not consumed because of "
					"unsubscription of %p",
					count, sub);
		}

		LOG_DBG("%p unsubscribed", sub);

		sub_free(sub);
	}

	return 0;
}

ha_ev_t *ha_ev_wait(struct ha_ev_subs *sub, k_timeout_t timeout)
{
	if ((sub != NULL) && HA_EV_SUBS_CONF_SUBSCRIBED(sub)) {
		return (ha_ev_t *)k_fifo_get(&sub->_evq, timeout);
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

struct ha_room *ha_dev_get_room(ha_dev_t *const dev)
{
	struct ha_room_assoc *assoc = NULL;
	struct ha_room *room		= NULL;

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

int ha_stats_copy(struct ha_stats *dest)
{
	if (dest == NULL) {
		return -EINVAL;
	}

	memcpy(dest, &stats, sizeof(struct ha_stats));

	return 0;
}

bool ha_dev_ep_exists(const ha_dev_t *dev, uint8_t endpoint_index)
{
	return dev && (endpoint_index < dev->endpoints_count);
}

bool ha_dev_ep_has_datatype(const ha_dev_t *dev,
							uint8_t endpoint_index,
							const ha_data_type_t datatype)
{
	if (!ha_dev_ep_exists(dev, endpoint_index)) {
		return false;
	}

	const struct ha_device_endpoint *const ep = &dev->endpoints[endpoint_index];

#if HA_DEV_EP_TYPE_SEARCH_OPTIMIZATION
	return (bool)(ep->_data_types & (1u << datatype));
#else
	return ha_data_descr_data_type_has(ep->data_descr, ep->data_descr_count, datatype);
#endif
}

bool ha_dev_ep_check_data_support(const ha_dev_t *dev, uint8_t endpoint_index)
{
	bool support = false;

	if (ha_dev_ep_exists(dev, endpoint_index)) {
		const struct ha_device_endpoint_config *const ep_cfg =
			dev->endpoints[endpoint_index].cfg;

		support = ep_cfg->ingest && ep_cfg->data_descr && ep_cfg->data_descr_size;
	}

	return support == true;
}

bool ha_dev_ep_check_cmd_support(const ha_dev_t *dev, uint8_t endpoint_index)
{
	bool support = false;

	if (ha_dev_ep_exists(dev, endpoint_index)) {
		const struct ha_device_endpoint_config *const ep_cfg =
			dev->endpoints[endpoint_index].cfg;

		support = ep_cfg->command && ep_cfg->cmd_descr && ep_cfg->cmd_descr_size;
	}

	return support == true;
}

int ha_ev_data_append(ha_ev_t *event, ha_data_t *data)
{
	if ((event == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	/* Check if event is not already referenced */
	if (atomic_get(&event->ref_count) != 0) {
		return -EINVAL;
	}

	sys_slist_append(&event->_data_slist, &data->_node);

	return 0;
}

int ha_ev_data_append_array(ha_ev_t *event, ha_data_t **data, size_t count)
{
	if ((event == NULL) || (data == NULL) || (count == 0u)) {
		return -EINVAL;
	}

	/* Check if event is not already referenced */
	if (atomic_get(&event->ref_count) != 0) {
		return -EINVAL;
	}

	for (size_t i = 0u; i < count; i++) {
		sys_slist_append(&event->_data_slist, &data[i]->_node);
	}

	return 0;
}

int ha_ev_data_alloc_append(ha_ev_t *event, const ha_data_storage_t *storage)
{
	if (storage == NULL) {
		return -EINVAL;
	}

	int ret;
	size_t value_size = ha_data_get_size_by_type(storage->type);
	ha_data_t *data	  = ha_data_alloc(storage->type);

	if (data != NULL) {
		data->occurence = storage->occurence;
		data->subsys	= storage->subsys;
		memcpy(data->value, &storage->value, value_size);

		ret = ha_ev_data_append(event, data);

		ha_data_free(data);

	} else if (value_size == 0u) {
		ret = -EINVAL;
	} else {
		ret = -ENOMEM;
	}

	return ret;
}