/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TODO: shorten "ha_dev" to "mydev" or "myd */

#ifndef _HA_DEVS_H_
#define _HA_DEVS_H_

#include <zephyr/kernel.h>

#include <sys/types.h>

#include <zephyr/sys/slist.h>

#include "ha.h"
#include "room.h"
#include "data.h"

/* No filters, iterate over all devices */
#define HA_DEV_FILTER_DISABLED NULL

#define HA_DEV_ENDPOINT_MAX_COUNT 2u

#define HA_DEVICES_ENDPOINT_TYPE_SEARCH_OPTIMIZATION 1u

#define _HA_DEV_FILTER_BY_DEVICE_MEDIUM(_medium) \
	(&(ha_dev_filter_t) { \
		.flags = HA_DEV_FILTER_MEDIUM, \
		.medium = _medium, \
	})

#define _HA_DEV_FILTER_BY_DEVICE_TYPE(_type) \
	(&(ha_dev_filter_t) { \
		.flags = HA_DEV_FILTER_DEVICE_TYPE, \
		.type = _type, \
	})

#define HA_DEV_FILTER_CAN _HA_DEV_FILTER_BY_DEVICE_MEDIUM(HA_DEV_MEDIUM_CAN)
#define HA_DEV_FILTER_BLE _HA_DEV_FILTER_BY_DEVICE_MEDIUM(HA_DEV_MEDIUM_BLE)

#define HA_DEV_FILTER_CANIOT _HA_DEV_FILTER_BY_DEVICE_TYPE(HA_DEV_TYPE_CANIOT)
#define HA_DEV_FILTER_XIAOMI_MIJIA _HA_DEV_FILTER_BY_DEVICE_TYPE(HA_DEV_TYPE_XIAOMI_MIJIA)
#define HA_DEV_FILTER_NUCLEO_F429ZI _HA_DEV_FILTER_BY_DEVICE_TYPE(HA_DEV_TYPE_NUCLEO_F429ZI)

#define HA_DEV_API_SELECT_ENDPOINT_0_CB NULL

typedef enum
{
	HA_DEV_FILTER_MEDIUM = BIT(0), /* filter by medium */
	HA_DEV_FILTER_DEVICE_TYPE = BIT(1), /* filter by device type */
	HA_DEV_FILTER_DATA_EXIST = BIT(2), /* Filter by existing data on given endpoint */
	// HA_DEV_FILTER_SENSOR_TYPE, /* filter by temperature sensor type */
	HA_DEV_FILTER_DATA_TIMESTAMP = BIT(3), /* filter devices with recent measurements */
	// HA_DEV_FILTER_REGISTERED_TIMESTAMP, /* filter recent devices */
	HA_DEV_FILTER_HAS_TEMPERATURE = BIT(4), /* filter devices with temperature sensor */
	HA_DEV_FILTER_ROOM_ID = BIT(5), /* filter devices with defined room id */
	HA_DEV_FILTER_FROM_INDEX = BIT(6), /* filter devices from index (included) */
	HA_DEV_FILTER_TO_INDEX = BIT(7), /* filter devices to index (excluded) */

	/* filter devices until count of valid devices reach the given value "to_count" */
	HA_DEV_FILTER_TO_COUNT = BIT(8), 
} ha_dev_filter_flags_t;

typedef enum 
{
	HA_DEV_ENDPOINT_NONE, /* Mean either: UNDEFINED endpoint or ANY endpoint */
	HA_DEV_ENDPOINT_XIAOMI_MIJIA,
	HA_DEV_ENDPOINT_NUCLEO_F429ZI,

	/* CANIOT Board Level Control */
	HA_DEV_ENDPOINT_CANIOT_BLC0,
	HA_DEV_ENDPOINT_CANIOT_BLC1,
	HA_DEV_ENDPOINT_CANIOT_BLC2,
	HA_DEV_ENDPOINT_CANIOT_BLC3,
	HA_DEV_ENDPOINT_CANIOT_BLC4,
	HA_DEV_ENDPOINT_CANIOT_BLC5,
	HA_DEV_ENDPOINT_CANIOT_BLC6,
	HA_DEV_ENDPOINT_CANIOT_BLC7,

	/* CANIOT specific application endpoints */
	HA_DEV_ENDPOINT_CANIOT_HEATING,
} ha_endpoint_id_t;

typedef enum
{
	HA_EV_TYPE_DATA = 0u,
	// HA_EV_TYPE_CONTROL = 1,
	HA_EV_TYPE_COMMAND = 2u,
	HA_EV_TYPE_ERROR = 3u,
} ha_ev_type_t;

/* TODO Incompatible masks */

typedef struct
{
	ha_dev_filter_flags_t flags;

	/* Filter rules */
	ha_dev_medium_type_t medium;
	ha_dev_type_t device_type;
	uint32_t data_timestamp;
	ha_room_id_t rid;
	uint32_t from_index: 8u;
	uint32_t to_index: 8u;
	uint32_t to_count: 4u;
	ha_endpoint_id_t endpoint_id;
} ha_dev_filter_t;

typedef struct
{
	/**
	 * @brief Bitmask of flags telling the last events of the endpoint 
	 * should be locked. Bit n is set if the endpoint n last event should be locked.
	 */
	uint8_t ep_lock_last_ev_mask;
} ha_dev_iter_opt_t;

/**
 * @brief Lock only the last event of the first endpoint
 */
#define HA_DEV_ITER_OPT_DEFAULT() \
	((const ha_dev_iter_opt_t) { \
		.ep_lock_last_ev_mask = 1u, \
	})

/**
 * @brief Lock the last event of all endpoints
 */
#define HA_DEV_ITER_OPT_LOCK_ALL() \
	((const ha_dev_iter_opt_t) { \
		.ep_lock_last_ev_mask = (uint8_t) -1, \
	})

typedef struct ha_dev_cmd
{
	/* Command type */
	uint32_t type;
} ha_dev_cmd_t;

struct ha_stats
{
	/* General stats */
	uint32_t ev; /* Number of events successfully processed */

	uint32_t dev_dropped; /* Number of devices dropped */

	/* Reasons for dropped devices */
	uint32_t dev_no_mem; /* No Slot available */
	uint32_t dev_no_api; /* No API found */
	uint32_t dev_ep_init; /* Endpoints initialization failed */
	uint32_t dev_no_ep; /* No endpoints found */
	uint32_t dev_toomuch_ep; /* Too much endpoints */

	/* Dropped events */
	uint32_t ev_dropped; /* Number of events dropped */
	uint32_t ev_data_dropped; /* Number of data events dropped */
	uint32_t ev_cmd_dropped; /* Number of command events dropped */

	/* Reasons for dropped events */
	uint32_t ev_no_mem; /* No memory available */
	uint32_t ev_no_ep; /* No endpoint found */
	uint32_t ev_ep; /* Invalid endpoint */
	uint32_t ev_payload_size; /* Payload size too big */
	uint32_t ev_no_data_mem; /* No memory available for data */
	uint32_t ev_ingest; /* DATA Ingestion failed */
	uint32_t ev_never_ref; /* Event never referenced */

	/* Memory, buffers, blocks usage */
	uint32_t mem_ev_count; /* Number of events currently in use (allocated) */
	uint32_t mem_ev_remaining; /* Number of events remaining */
	uint32_t mem_device_count; /* Maximum number of events in use (allocated) */
	uint32_t mem_device_remaining; /* Number of events remaining */
	uint32_t mem_sub_count; /* Number of subscriptions currently in use (allocated) */
	uint32_t mem_sub_remaining; /* Number of subscriptions remaining */

	uint32_t mem_heap_alloc; /* Heap currently allocated */
	uint32_t mem_heap_total; /* Heap allocated in total */
};

#define HA_DEV_STATS_ERR_FLAG_MASK 0xFFu

/* No memory available for event */
#define HA_DEV_STATS_ERR_FLAG_EV_NO_MEM BIT(0u)

/* No endpoint found for event */
#define HA_DEV_STATS_ERR_FLAG_EV_NO_EP BIT(1u)

/* Invalid endpoint for event */
#define HA_DEV_STATS_ERR_FLAG_EV_EP BIT(2u)

/* Payload size too big for event */
#define HA_DEV_STATS_ERR_FLAG_EV_PAYLOAD_SIZE BIT(3u)

/* No memory available for data */
#define HA_DEV_STATS_ERR_FLAG_EV_NO_DATA_MEM BIT(4u)

/* DATA Ingestion failed */
#define HA_DEV_STATS_ERR_FLAG_EV_INGEST BIT(5u)


struct ha_dev_stats
{
	uint32_t rx; /* number of received packets */
	uint32_t rx_bytes; /* number of received bytes */

	uint32_t tx; /* number of transmitted packets */
	uint32_t tx_bytes; /* number of transmitted bytes */

	uint32_t max_inactivity; /* number of seconds without any activity */

	/* Events errors count */
	uint32_t err_ev;

	/* Error flags */
	uint32_t err_flags;
};

/* Forward declaration */
struct ha_event;

struct ha_event_stats
{
	uint8_t notified; /* Number of times the event has been notified */
	uint32_t alive_ms; /* Time the event has been alive (ms) */
};

#define HA_ENDPOINT_INDEX(_idx) (_idx)

struct ha_device;

struct ha_device_endpoint_api
{
	/* Endpoint identifier */
	ha_endpoint_id_t eid: 8u;

	/* Endpoint data size, internal format */
	uint32_t data_size : 8u;

	/* Endpoint expected payload size, 0 for unspecified*/
	uint32_t expected_payload_size : 8u;

	uint32_t retain_last_event: 1u;

	uint32_t _unused: 7u;

	const struct ha_data_descr *data_descr;
	const struct ha_data_descr *cmd_descr;

	uint8_t data_descr_size;
	uint8_t cmd_descr_size;

	int (*ingest)(struct ha_event *ev,
		      struct ha_dev_payload *pl);

	int (*command)(struct ha_device *dev,
		       const ha_dev_cmd_t *cmd);
};

struct ha_device_endpoint
{
	const struct ha_device_endpoint_api *api;

	/* Endpoint last data event item */
	struct ha_event *last_data_event;

#if HA_DEVICES_ENDPOINT_TYPE_SEARCH_OPTIMIZATION
	/* Flags telling what kind of data types can be found in the endpoint 
	 * For optimization purpose
	 */
	uint32_t _data_types;
#endif /* HA_DEVICES_ENDPOINT_TYPE_SEARCH_OPTIMIZATION */
};

#define HA_DEV_ENDPOINT_API_INIT(_eid, _data_size, _expected_payload_size, _ingest, _command) \
	{ \
		.eid = _eid, \
		.data_size = _data_size, \
		.expected_payload_size = _expected_payload_size, \
		.ingest = _ingest, \
		.command = _command, \
	}


struct ha_device_api {
	/**
	 * @brief 
	 * 
	 * @param addr Device address
	 * @param api Allow to overwrite the default device API
	 * @return true to accept the device, false to refuse the registration
	 */
	int (*init_endpoints)(const ha_dev_addr_t *addr,
			      struct ha_device_endpoint *endpoints,
			      uint8_t *endpoints_count);

	/**
	 * @brief Choose which endpoint to use for a given payload
	 * 
	 * If NULL, the first endpoint is used
	 */
	int (*select_endpoint)(const ha_dev_addr_t *addr,
			       const struct ha_dev_payload *pl);

	/**
	 * @brief Called when a new data is received, in order to know what size
	 * should be allocated for the data.
	 * 
	 * y is a pointer to additionnal specific context for data (NULL to ignore)
	 */
	// ssize_t(*get_internal_format_size)(struct ha_device *dev,
	// 				   const void *ipayload,
	// 				   size_t ilen,
	// 				   void *y);

	/**
	 * @brief Called when a new data is received from the device, 
	 * this function should convert the data to the internal format if needed.
	 * 
	 * Or at least should just copy the data to the internal buffer.
	 * 
	 * The timestamp variable allow to adjust the timestamp of the data.
	 * 
	 * y is a pointer to additionnal specific context for data (NULL to ignore)
	 */
	// int (*convert_data)(struct ha_device *dev,
	// 		     const void *ipayload,
	// 		     size_t ilen,
	// 		     void *odata,
	// 		     size_t olen,
	// 		     uint32_t *timestamp,
	// 		     void *y);
};

struct ha_device {
	/* Addr which uniquely identifies the device */
	ha_dev_addr_t addr;

	/* UNIX timestamps in seconds */
	uint32_t registered_timestamp;

	/* Device API */
	const struct ha_device_api *api;

#if defined(CONFIG_HA_DEVICE_STATS)
	/* Device statistics */
	struct ha_dev_stats stats;
#endif

	/* Endpoints */
	struct ha_device_endpoint endpoints[HA_DEV_ENDPOINT_MAX_COUNT];

	/* Endpoints count */
	uint8_t endpoints_count;

	/* Room where the device is */
	struct ha_room *room;
};

typedef struct ha_device ha_dev_t;

typedef struct ha_event {

	/******************/
	/* Private members */
	/******************/

	/* Event type */
	ha_ev_type_t type: 8u;

	/* Number of times the event is referenced
	 * If ref_count is 0, the event data can be freed */
	atomic_val_t ref_count;

	/* Device the event is related to */
	ha_dev_t *dev;

	/* Singly linked list of data item */
	sys_slist_t slist;

	/******************/
	/* Public members */
	/******************/

	/* Event time */
	uint32_t timestamp;

	/* Event payload */
	void *data;

#if defined(CONFIG_HA_STATS)
	uint16_t data_size;
#endif

} ha_ev_t;

ha_dev_t *ha_dev_get_by_addr(const ha_dev_addr_t *addr);

// ha_dev_t *ha_dev_register(const ha_dev_addr_t *addr);

int ha_dev_register_data(const ha_dev_addr_t *addr,
			 const void *payload,
			 size_t payload_len,
			 uint32_t timestamp,
			 void *y);

struct ha_device_endpoint *ha_dev_get_endpoint(ha_dev_t *dev, uint32_t ep);

struct ha_device_endpoint *ha_dev_endpoint_get_by_id(ha_dev_t *dev, ha_endpoint_id_t eid);

int ha_dev_endpoint_get_index_by_id(ha_dev_t *dev, ha_endpoint_id_t eid);

ha_ev_t *ha_dev_get_last_event(ha_dev_t *dev, uint32_t ep);

const void *ha_dev_get_last_event_data(ha_dev_t *dev, uint32_t ep);

#define HA_DEV_EP0_GET_CAST_LAST_DATA(_dev, _type) \
	((_type *)ha_dev_get_last_event_data(_dev, 0u))

typedef bool ha_dev_iterate_cb_t(ha_dev_t *dev,
				 void *user_data);

int ha_dev_addr_cmp(const ha_dev_addr_t *a,
		    const ha_dev_addr_t *b);

int ha_dev_addr_to_str(const ha_dev_addr_t *addr,
		       char *buf,
		       size_t buf_len);

int ha_dev_get_index(ha_dev_t *dev);

/**
 * @brief Iterate over all devices, with the option to filter them
 * 
 * @param callback 
 * @param filter 
 * @param user_data 
 * @return size_t Number of devices iterated, negative on error
 */
ssize_t ha_dev_iterate(ha_dev_iterate_cb_t callback,
		       const ha_dev_filter_t *filter,
		       const ha_dev_iter_opt_t *options,
		       void *user_data);

static inline ssize_t ha_dev_xiaomi_iterate_data(ha_dev_iterate_cb_t callback,
					   void *user_data)
{
	const ha_dev_filter_t filter = {
		.flags =
			HA_DEV_FILTER_DATA_EXIST |
			HA_DEV_FILTER_DEVICE_TYPE,
		.device_type = HA_DEV_TYPE_XIAOMI_MIJIA,
	};

	return ha_dev_iterate(callback, &filter, NULL, user_data);
}


static inline ssize_t ha_dev_caniot_iterate_data(ha_dev_iterate_cb_t callback,
						void *user_data)
{
	const ha_dev_filter_t filter = {
		.flags =
			HA_DEV_FILTER_DATA_EXIST |
			HA_DEV_FILTER_DEVICE_TYPE,
		.device_type = HA_DEV_TYPE_CANIOT,
	};

	return ha_dev_iterate(callback, &filter, NULL, user_data);
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

uint32_t ha_ev_free_count(void);

void ha_ev_ref(ha_ev_t *event);

void *ha_ev_get_data(const ha_ev_t *event);

const void *ha_ev_get_data_check_type(const ha_ev_t *event,
					 ha_dev_type_t expected_type);

#define HA_EV_GET_CAST_DATA(_ev, _type) \
	((_type *)ha_ev_get_data(_ev))

void ha_ev_unref(ha_ev_t *event);

typedef bool (*event_subs_filter_func_t)(ha_ev_t *event);

typedef struct ha_ev_subs
{
	sys_dnode_t _handle;

	/* Queue of events to be notified to the waiter */
	struct k_fifo evq;

	/* On queued event hook */
	void (*on_queued)(struct ha_ev_subs *sub, ha_ev_t *event);

	/* Flag describing on which event should the waiter be notified */
	atomic_t flags;

	/* Depends on filter */
	event_subs_filter_func_t func;
	ha_dev_mac_t device_mac;
	ha_dev_type_t device_type;

} ha_ev_subs_t;

/* Event subscription Control flags */
#define HA_EV_SUBS_FLAG_SUBSCRIBED_BIT 	0u
#define HA_EV_SUBS_FLAG_SUBSCRIBED 	BIT(HA_EV_SUBS_FLAG_SUBSCRIBED_BIT)

#define HA_EV_SUBS_FLAG_ON_QUEUED_BIT 	1u
#define HA_EV_SUBS_FLAG_ON_QUEUED_HOOK 	BIT(HA_EV_SUBS_FLAG_ON_QUEUED_BIT)

/* Event subscription filter flags */
#define HA_EV_SUBS_DEVICE_TYPE 		BIT(2u)
#define HA_EV_SUBS_DEVICE_ADDR 		BIT(3u)
#define HA_EV_SUBS_DEVICE_DATA 		BIT(4u)
#define HA_EV_SUBS_DEVICE_COMMAND 	BIT(5u)
#define HA_EV_SUBS_DEVICE_ERROR 	BIT(6u)
#define HA_EV_SUBS_FUNCTION 		BIT(7u)

/* Notify subscriber only after a minimum interval (per device) */
#define HA_EV_SUBS_INTERVAL_MS 		BIT(8u)

/* Notify subscriber only every n events (per device) */
#define HA_EV_SUBS_ONE_OF_N 		BIT(9u)

#define HA_EV_SUBS_SUBSCRIBED(_subs) (atomic_test_bit(&_subs->flags, HA_EV_SUBS_FLAG_SUBSCRIBED_BIT))

typedef struct ha_ev_subs_conf
{
	uint32_t flags;

	/* Depends on filter */
	event_subs_filter_func_t func;
	const ha_dev_mac_t *device_mac;
	ha_dev_type_t device_type;
	
	/* Callback */
	void (*on_queued)(struct ha_ev_subs *sub, ha_ev_t *event);
} ha_ev_subs_conf_t;

/**
 * @brief Notify an event to all subscribers
 * 
 * @param event 
 * @return int Number of waiters notified, negative on error
 */
int ha_ev_notify_all(ha_ev_t *event);

/**
 * @brief Subscribe to a specific type of event, using given subscription 
 *  configuration. If subscription succeeds, the subscription handle is returned in
 *  the provided pointer "subs".
 * 
 * @param conf Subscription configuration
 * @param sub Pointer to event subscription handle
 * @return int 0 on success, negative error code otherwise
 */
int ha_ev_subscribe(const ha_ev_subs_conf_t *conf,
		    struct ha_ev_subs **sub);

/**
 * @brief Unsubscribe from a specific type of event, using given subscription.
 * 
 * @param sub 
 * @return int 
 */
int ha_ev_unsubscribe(struct ha_ev_subs *sub);

/**
 * @brief Wait for an event to be notified on given subscription.
 * 
 * @param sub 
 * @param timeout 
 * @return ha_ev_t* 
 */
ha_ev_t *ha_ev_wait(struct ha_ev_subs *sub,
			  k_timeout_t timeout);


struct ha_room_assoc
{	
	ha_room_id_t rid;
	ha_dev_addr_t addr;
};

struct ha_room *ha_dev_get_room(ha_dev_t *const dev);

/*____________________________________________________________________________*/

ha_ev_t *ha_dev_command(const ha_dev_addr_t *addr,
			ha_dev_cmd_t *cmd,
			k_timeout_t timeout);

/*____________________________________________________________________________*/

int ha_stats_copy(struct ha_stats *dest);

/*____________________________________________________________________________*/

bool ha_dev_endpoint_exists(const ha_dev_t *dev,
			    uint8_t endpoint_index);

bool ha_dev_endpoint_has_datatype(const ha_dev_t *dev,
				  uint8_t endpoint_index,
				  const ha_data_type_t datatype);

bool ha_dev_endpoint_check_data_support(const ha_dev_t *dev,
				  uint8_t endpoint_index);

bool ha_dev_endpoint_check_cmd_support(const ha_dev_t *dev,
				 uint8_t endpoint_index);

/*____________________________________________________________________________*/

/* Iterate over all devices/endpoints and count all data inputs matching given filter */
int ha_dev_count_data_inputs(ha_data_type_t type,
			     ha_data_assignement_t assignement);

/* Iterate over all devices/endpoints and count all control outputs matching given filter */
int ha_dev_count_control_outputs(ha_data_type_t type,
				 ha_data_assignement_t assignement);

#endif /* _HA_DEVS_H_ */