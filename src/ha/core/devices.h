/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_DEVS_H_
#define _HA_DEVS_H_

#include <sys/types.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#include "ha/core/ha.h"
#include "ha/core/room.h"
#include "ha/core/data.h"

/* (TODO) Specific need rework */
#include "ha/core/ha_spe.h"

/* Defines */
#define HA_DEV_ENDPOINT_MAX_COUNT 2u
#define HA_DEV_ENDPOINT_TYPE_SEARCH_OPTIMIZATION 1u

typedef enum
{
	HA_EV_TYPE_DATA = 0u,
	// HA_EV_TYPE_CONTROL = 1,
	HA_EV_TYPE_COMMAND = 2u,
	HA_EV_TYPE_ERROR = 3u,
} ha_ev_type_t;

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

struct ha_device_api;
struct ha_device_endpoint_api;
struct ha_device_endpoint;
struct ha_device_stats;
struct ha_device_payload;
struct ha_device_cmd;
struct ha_device_filter;
struct ha_device_iter_opt;
struct ha_ev_subs_conf;
struct ha_ev_subs;

typedef struct ha_device_mac 		ha_dev_mac_t;
typedef struct ha_device_address 	ha_dev_addr_t;
typedef struct ha_device 		ha_dev_t;
typedef struct ha_event 		ha_ev_t;
typedef struct ha_ev_subs_conf 		ha_ev_subs_conf_t;
typedef struct ha_ev_subs 		ha_ev_subs_t;
typedef struct ha_device_filter ha_dev_filter_t;
typedef struct ha_device_iter_opt ha_dev_iter_opt_t;

typedef uint32_t ha_timestamp_t;

struct ha_device_mac {
	ha_dev_medium_type_t medium;
	ha_dev_mac_addr_t addr;
};

struct ha_device_address {
	ha_dev_type_t type;
	ha_dev_mac_t mac;
};

struct ha_device_payload
{
	/* Payload timestamp, 0 to use current time */
	ha_timestamp_t timestamp;

	/* Payload */
	const char *buffer;

	/* Payload size */
	size_t len;

	/* Additionnal specific context for data, which will be passed to the
 	 * device API to resolve the endpoint. Set NULL if not needed. */
	void *y;
};

struct ha_device_cmd
{
	/* Command type */
	uint32_t type;
};

struct ha_device_endpoint_api
{
	/* Endpoint identifier */
	ha_endpoint_id_t eid : 8u;

	/* Endpoint data size, internal format */
	uint32_t data_size : 8u;

	/* Endpoint expected payload size, 0 for unspecified*/
	uint32_t expected_payload_size : 8u;

	uint32_t retain_last_event : 1u;

	uint32_t _unused : 7u;

	const struct ha_data_descr *data_descr;
	const struct ha_data_descr *cmd_descr;

	uint8_t data_descr_size;
	uint8_t cmd_descr_size;

	int (*ingest)(struct ha_event *ev,
		      const struct ha_device_payload *pl);

	int (*command)(struct ha_device *dev,
		       const struct ha_device_cmd *cmd);
};

struct ha_device_endpoint
{
	const struct ha_device_endpoint_api *api;

	/* Endpoint last data event item */
	struct ha_event *last_data_event;

#if HA_DEV_ENDPOINT_TYPE_SEARCH_OPTIMIZATION
	/* Flags telling what kind of data types can be found in the endpoint
	 * For optimization purpose
	 */
	uint32_t _data_types;
#endif /* HA_DEV_ENDPOINT_TYPE_SEARCH_OPTIMIZATION */
};


#define HA_DEV_ENDPOINT_SELECT_0_CB NULL

#define HA_DEV_ENDPOINT_INDEX(_idx) (_idx)

#define HA_DEV_EP_0_GET_CAST_LAST_DATA(_dev, _type) \
	((_type *)ha_dev_get_last_event_data(_dev, 0u))

#define HA_DEV_ENDPOINT_API_INIT(_eid, _data_size, _expected_payload_size, _ingest, _command) \
	{ \
		.eid = _eid, \
		.data_size = _data_size, \
		.expected_payload_size = _expected_payload_size, \
		.ingest = _ingest, \
		.command = _command, \
	}

#define HA_DEV_PAYLOAD_INIT(_buf, _len, _ts, _y) \
	{ \
		.timestamp = _ts, \
		.buffer = _buf, \
		.len = _len, \
		.y = _y, \
	}


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

struct ha_device_stats
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

struct ha_device {
	/* Session Device Unique ID
	 * ID guaranteed to be unique accross the current session.
	 * i.e. Between two reboots of the controller.
	 *
	 * Note:
	 * - It can be used as a human readable identifier for the device, which
	 * can be used to differentiate two devices without comparing their
	 * addresses.
	 * - Also, devices with higher sdevuid are more recent than
	 * devices with lower sdevuid.
	 * - This ID is not persistent accross reboots.
	 * - It starts as 1 and is incremented for each new device.
	 */
	uint32_t sdevuid;

	/* Addr which uniquely identifies the device */
	struct ha_device_address addr;

	/* UNIX timestamps in seconds */
	uint32_t registered_timestamp;

	/* Device API */
	const struct ha_device_api *api;

#if defined(CONFIG_APP_HA_DEVICE_STATS)
	/* Device statistics */
	struct ha_device_stats stats;
#endif

	/* Endpoints */
	struct ha_device_endpoint endpoints[HA_DEV_ENDPOINT_MAX_COUNT];

	/* Endpoints count */
	uint8_t endpoints_count;

	/* Room where the device is */
	struct ha_room *room;
};

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
			       const struct ha_device_payload *pl);
};

struct ha_event {
	/******************/
	/* Private members */
	/******************/

	/* Event type */
	ha_ev_type_t type : 8u;

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

#if defined(CONFIG_APP_HA_STATS)
	uint16_t data_size;
#endif
};

/* Event subscription Control flags */
#define HA_EV_SUBS_FLAG_SUBSCRIBED_BIT 	0u
#define HA_EV_SUBS_FLAG_SUBSCRIBED 	BIT(HA_EV_SUBS_FLAG_SUBSCRIBED_BIT)

typedef bool (*ha_subs_ev_filter_func_t)(ha_ev_t *event);
typedef void (*ha_subs_ev_on_queued_func_t)(struct ha_ev_subs *sub, ha_ev_t *event);

struct ha_ev_subs
{
	sys_dnode_t _handle;

	/* Queue of events to be notified to the waiter */
	struct k_fifo _evq;

	/* Subscription Control flags */
	atomic_t _ctrl;

	/* Subscription Configuration */
	const struct ha_ev_subs_conf *conf;

};


/* Event subscription Config flags */
#define HA_EV_SUBS_CONF_ON_QUEUED_HOOK_BIT 	1u
#define HA_EV_SUBS_CONF_ON_QUEUED_HOOK 		BIT(HA_EV_SUBS_CONF_ON_QUEUED_HOOK_BIT)

/* Event subscription filter flags */
#define HA_EV_SUBS_CONF_DEVICE_TYPE 		BIT(2u)
#define HA_EV_SUBS_CONF_DEVICE_ADDR 		BIT(3u)
#define HA_EV_SUBS_CONF_DEVICE_DATA 		BIT(4u)
#define HA_EV_SUBS_CONF_DEVICE_COMMAND 		BIT(5u)
#define HA_EV_SUBS_CONF_DEVICE_ERROR 		BIT(6u)
#define HA_EV_SUBS_CONF_FILTER_FUNCTION 	BIT(7u)

#define HA_EV_SUBS_CONF_SUBSCRIBED(_subs) (atomic_test_bit(&_subs->_ctrl, HA_EV_SUBS_FLAG_SUBSCRIBED_BIT))

struct ha_ev_subs_conf
{
	uint32_t flags;

	/* Depends on filter */
	ha_subs_ev_filter_func_t filter_cb;
	ha_dev_mac_t device_mac;
	ha_dev_type_t device_type;

	/* Callback */
	ha_subs_ev_on_queued_func_t on_queued_cb;
};

/* TODO Incompatible masks */

struct ha_device_filter
{
	ha_dev_filter_flags_t flags;

	/* Filter rules */
	ha_dev_medium_type_t medium;
	ha_dev_type_t device_type;
	uint32_t data_timestamp;
	ha_room_id_t rid;
	uint32_t from_index : 8u;
	uint32_t to_index : 8u;
	uint32_t to_count : 4u;
	ha_endpoint_id_t endpoint_id;
};

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

struct ha_device_iter_opt
{
	/**
	 * @brief Bitmask of flags telling the last events of the endpoint
	 * should be locked. Bit n is set if the endpoint n last event should be locked.
	 */
	uint8_t ep_lock_last_ev_mask;
};

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

struct ha_event_stats
{
	uint8_t notified; /* Number of times the event has been notified */
	// uint32_t alive_ms; /* Time the event has been alive (ms) */
};

struct ha_room_assoc
{
	ha_room_id_t rid;
	ha_dev_addr_t addr;
};

/**
 * @brief Compare two devices addresses
 * 
 * @param a First device address
 * @param b Second device address
 * @return int 0 if equal, any other value otherwise
 */
int ha_dev_addr_cmp(const ha_dev_addr_t *a,
		    const ha_dev_addr_t *b);

/**
 * @brief Convert a device address to a string
 * 
 * @param addr Address to convert
 * @param buf Buffer to store the string
 * @param buf_len Buffer length
 * @return int 0 if success, any other value otherwise
 */
int ha_dev_addr_to_str(const ha_dev_addr_t *addr,
		       char *buf,
		       size_t buf_len);

/**
 * @brief Get the device by its address
 *
 * @param addr
 * @return ha_dev_t* A pointer to the device or NULL if not found
 */
ha_dev_t *ha_dev_get_by_addr(const ha_dev_addr_t *addr);

/**
 * @brief Propagate a payload issues from a device
 *
 * @param addr Address of the device which issued the payload
 * @param payload Payload buffer and context
 * @return int
 */
int ha_dev_register_data(const ha_dev_addr_t *addr,
			 const struct ha_device_payload *payload);

/**
 * @brief Get device endpoint last event
 * 
 * @param dev 
 * @param ep Endpoint index
 * @return ha_ev_t* Last event received on the endpoint or NULL if none
 */
ha_ev_t *ha_dev_get_last_event(ha_dev_t *dev, uint32_t ep);

/**
 * @brief Get device endpoint last event data
 * 
 * @param dev 
 * @param ep Endpoint index
 * @return const void* Last event data received on the endpoint or NULL if none
 */
const void *ha_dev_get_last_event_data(ha_dev_t *dev, uint32_t ep);

/**
 * @brief Callback for iterating over devices
 * 
 * @param dev Current device
 * @param user_data User data passed to the iterator
 * @return typedef Return true to continue iterating, false to stop
 */
typedef bool ha_dev_iterate_cb_t(ha_dev_t *dev,
				 void *user_data);

/**
 * @brief Iterate over filtered devices
 * 
 * @param callback Callback to call for each device
 * @param filter Filter to apply to the devices to iterate
 * @param options Additional options to manupulate the iteration
 * @param user_data User pointer to pass to the callback
 * @return ssize_t Number of devices iterated, negative on error
 */
ssize_t ha_dev_iterate(ha_dev_iterate_cb_t callback,
		       const ha_dev_filter_t *filter,
		       const ha_dev_iter_opt_t *options,
		       void *user_data);

/**
 * @brief Reference an event, i.e. incrementing its reference counter
 * 
 * Note: 
 * - This function is thread-safe
 * - This function requires ha_ev_unref() to be called when the event is no 
 * longer needed
 * 
 * @param event 
 */
void ha_ev_ref(ha_ev_t *event);

/**
 * @brief Unreference an event, i.e. decrementing its reference counter
 * 
 * Note:
 * - This function is thread-safe
 * - This function requires ha_ev_ref() to be called before
 * 
 * @param event 
 */
void ha_ev_unref(ha_ev_t *event);

/**
 * @brief Get the data associated with an event if any
 * 
 * @param event 
 * @return void* 
 */
void *ha_ev_get_data(const ha_ev_t *event);

/**
 * @brief Macro to cast the data associated with an event to a specific type
 */
#define HA_EV_GET_CAST_DATA(_ev, _type) \
	((_type *)ha_ev_get_data(_ev))

/**
 * @brief Notify an event to all subscribers
 *
 * @param event Event to notify
 * @return int Number of waiters notified, negative on error
 */
int ha_ev_notify_all(ha_ev_t *event);

/**
 * @brief Subscribe to a specific type of event, using given subscription
 *  configuration. If subscription succeeds, the subscription handle is returned in
 *  the provided pointer "subs".
 *
 * @param conf Subscription configuration, must live until unsubscribed
 * @param sub Pointer to event subscription handle
 * @return int 0 on success, negative error code otherwise
 */
int ha_ev_subscribe(const ha_ev_subs_conf_t *conf,
		    struct ha_ev_subs **sub);

/**
 * @brief Unsubscribe from a specific type of event, using given subscription
 * handle.
 *
 * @param sub Subscription handle, set by ha_ev_subscribe()
 * @return int
 */
int ha_ev_unsubscribe(struct ha_ev_subs *sub);

/**
 * @brief Wait for an event to be notified on given subscription handle.
 *
 * @param sub Subscription handle, set by ha_ev_subscribe()
 * @param timeout Timeout to wait for an event
 * @return ha_ev_t*
 */
ha_ev_t *ha_ev_wait(struct ha_ev_subs *sub,
		    k_timeout_t timeout);

/**
 * @brief Get the room associated with a device
 * 
 * @param dev 
 * @return struct ha_room* Room associated with the device, NULL if none
 */
struct ha_room *ha_dev_get_room(ha_dev_t *const dev);

/**
 * @brief Send a command to a device, and wait for the response event
 * 
 * @param addr Address to request
 * @param cmd Command to send
 * @param timeout Timeout to wait for the response
 * @return ha_ev_t* Response event, NULL on error/timeout
 */
ha_ev_t *ha_dev_command(const ha_dev_addr_t *addr,
			struct ha_device_cmd *cmd,
			k_timeout_t timeout);


/**
 * @brief Get device endpoint by index
 * 
 * @param dev 
 * @param ep Index of the endpoint
 * @return struct ha_device_endpoint* 
 */
struct ha_device_endpoint *ha_dev_endpoint_get(ha_dev_t *dev, uint32_t ep);

/**
 * @brief Get device endpoint by its type id
 * 
 * @param dev 
 * @param eid Endpoint type id
 * @return struct ha_device_endpoint* 
 */
struct ha_device_endpoint *ha_dev_endpoint_get_by_id(ha_dev_t *dev, ha_endpoint_id_t eid);

/**
 * @brief Get device endpoint index by its endpoint type id
 * 
 * @param dev 
 * @param eid 
 * @return int 
 */
int ha_dev_endpoint_get_index_by_id(ha_dev_t *dev, ha_endpoint_id_t eid);

/**
 * @brief Tell whether a device has a endpoint at the given index
 * 
 * @param dev 
 * @param endpoint_index 
 * @return true 
 * @return false 
 */
bool ha_dev_endpoint_exists(const ha_dev_t *dev,
			    uint8_t endpoint_index);

/**
 * @brief Tell whether a device has a endpoint at the given index which supports 
 * the given datatype for incoming data events. 
 * @param dev 
 * @param endpoint_index Endpoint index.
 * @param datatype 
 * @return true 
 * @return false 
 */
bool ha_dev_endpoint_has_datatype(const ha_dev_t *dev,
				  uint8_t endpoint_index,
				  const ha_data_type_t datatype);

/**
 * @brief Tell whether a device has a endpoint at the given index which supports
 * data events.
 * 
 * @param dev 
 * @param endpoint_index 
 * @return true 
 * @return false 
 */
bool ha_dev_endpoint_check_data_support(const ha_dev_t *dev,
					uint8_t endpoint_index);

/**
 * @brief Tell whether a device has a endpoint at the given index which supports
 * commands.
 * 
 * @param dev 
 * @param endpoint_index 
 * @return true 
 * @return false 
 */
bool ha_dev_endpoint_check_cmd_support(const ha_dev_t *dev,
				       uint8_t endpoint_index);

/**
 * @brief Iterate over all devices/endpoints and count all data inputs matching 
 * given "assignement".
 * 
 * @param type 
 * @param assignement 
 * @return int 
 */
int ha_dev_count_data_inputs(ha_data_type_t type,
			     ha_data_assignement_t assignement);

/** Iterate over all devices/endpoints and count all control outputs matching
 * given "assignement".
 * @brief 
 * 
 * @param type 
 * @param assignement 
 * @return int 
 */
int ha_dev_count_control_outputs(ha_data_type_t type,
				 ha_data_assignement_t assignement);

/* Debug functions */

/**
 * @brief Return the number of unallocated memory block in the pool for events
 * 
 * Note: Debug function
 * 
 * @return uint32_t 
 */
uint32_t ha_ev_free_count(void);

/**
 * @brief Copy HA statistics to given structure
 * 
 * @param dest 
 * @return int 
 */
int ha_stats_copy(struct ha_stats *dest);

#endif /* _HA_DEVS_H_ */