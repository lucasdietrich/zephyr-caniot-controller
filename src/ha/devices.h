/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ha_devs.h
 * @brief
 * @version 0.1
 * @date 2022-03-31
 
 *
 * TODO: shorten "ha_dev" to "mydev" or "myd"
 *
 */

#ifndef _HA_DEVS_H_
#define _HA_DEVS_H_

#include <zephyr.h>

#include "data.h"
#include "room.h"

/* Forward declaration */
struct ha_event;

struct ha_dev_stats
{
	uint32_t rx; /* number of received packets */
	uint32_t rx_bytes; /* number of received bytes */

	uint32_t tx; /* number of transmitted packets */
	uint32_t tx_bytes; /* number of transmitted bytes */

	uint32_t max_inactivity; /* number of seconds without any activity */
};

struct ha_event_stats
{
	uint8_t notified; /* Number of times the event has been notified */
	uint32_t alive_ms; /* Time the event has been alive (ms) */
};

struct ha_device;

struct ha_device_api {
	/**
	 * @brief Called when a new device is registered.
	 * 
	 * @param addr Device address
	 * @return true to accept the device, false to refuse the registration
	 */
	bool (*on_registration)(const ha_dev_addr_t *addr);

	/**
	 * @brief Called when a new data is received, in order to know what size
	 * should be allocated for the data.
	 */
	size_t(*get_internal_format_size)(struct ha_device *dev,
					  const void *idata,
					  size_t ilen);

	/**
	 * @brief Called when a new data is received from the device, 
	 * this function should convert the data to the internal format if needed.
	 * 
	 * Or at least should just copy the data to the internal buffer.
	 * 
	 * The timestamp variable allow to adjust the timestamp of the data.
	 */
	bool (*convert_data)(struct ha_device *dev,
			     const void *idata,
			     size_t ilen,
			     void *odata,
			     size_t olen,
			     uint32_t *timestamp);
};

struct ha_device {
	/* Addr which uniquely identifies the device */
	ha_dev_addr_t addr;

	/* UNIX timestamps in seconds */
	uint32_t registered_timestamp;

	/* Device API */
	const struct ha_device_api *api;

	/* Device statistics */
	struct ha_dev_stats stats;

	/* Device last data event item */
	struct ha_event *last_data_event;

	/* Room where the device is */
	struct ha_room *room;
};

typedef struct ha_device ha_dev_t;

ha_dev_t *ha_dev_get_by_addr(const ha_dev_addr_t *addr);

// ha_dev_t *ha_dev_register(const ha_dev_addr_t *addr);

int ha_dev_register_data(const ha_dev_addr_t *addr,
			 const void *data,
			 size_t data_len,
			 uint32_t timestamp);

const void *ha_dev_get_last_data(ha_dev_t *dev);

#define HA_DEV_GET_CAST_LAST_DATA(_dev, _type) \
	((_type *)ha_dev_get_last_data(_dev))



typedef void ha_dev_iterate_cb_t(ha_dev_t *dev,
				 void *user_data);

int ha_dev_addr_cmp(const ha_dev_addr_t *a,
		    const ha_dev_addr_t *b);

size_t ha_dev_iterate(ha_dev_iterate_cb_t callback,
		      const ha_dev_filter_t *filter,
		      void *user_data);

static inline size_t ha_dev_xiaomi_iterate_data(ha_dev_iterate_cb_t callback,
					   void *user_data)
{
	const ha_dev_filter_t filter = {
		.flags =
			HA_DEV_FILTER_DATA_EXIST |
			HA_DEV_FILTER_DEVICE_TYPE,
		.device_type = HA_DEV_TYPE_XIAOMI_MIJIA,
	};

	return ha_dev_iterate(callback, &filter, user_data);
}


static inline size_t ha_dev_caniot_iterate_data(ha_dev_iterate_cb_t callback,
					   void *user_data)
{
	const ha_dev_filter_t filter = {
		.flags =
			HA_DEV_FILTER_DATA_EXIST |
			HA_DEV_FILTER_DEVICE_TYPE,
		.device_type = HA_DEV_TYPE_CANIOT,
	};

	return ha_dev_iterate(callback, &filter, user_data);
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



/* move to specific header */
struct ha_dev_garage_cmd
{
	uint8_t actuate_left: 1;
	uint8_t actuate_right: 1;
};

void ha_dev_garage_cmd_init(struct ha_dev_garage_cmd *cmd);

int ha_dev_garage_cmd_send(const struct ha_dev_garage_cmd *cmd);




typedef enum
{
	HA_EV_TYPE_DATA = 0u,
	// HA_EV_TYPE_CONTROL = 1,
	HA_EV_TYPE_COMMAND = 2u,
	HA_EV_TYPE_ERROR = 3u,
} ha_ev_type_t;

typedef struct ha_event {

	/* Event time */
	uint32_t time;

	/* Event type */
	ha_ev_type_t type;

	/* Number of times the event is referenced
	 * If ref_count is 0, the event data can be freed */
	atomic_val_t ref_count;

	/* Flags */
	uint32_t isbroadcast: 1u;

	/* Device the event is related to */
	ha_dev_t *dev;

	/* Event data */
	void *data;
} ha_ev_t;

uint32_t ha_ev_free_count(void);

void ha_ev_ref(ha_ev_t *event);

const void *ha_ev_get_data(const ha_ev_t *event);

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

	/* Flag describing on which event should the waiter be notified */
	atomic_t flags;

	/* Depends on filter */
	event_subs_filter_func_t func;
	ha_dev_mac_t device_addr;
	ha_dev_type_t device_type;

} ha_ev_subs_t;

#define HA_EV_SUBS_FLAG_SUBSCRIBED_BIT 0u
#define HA_EV_SUBS_FLAG_SUBSCRIBED BIT(HA_EV_SUBS_FLAG_SUBSCRIBED_BIT)

#define HA_EV_SUBS_DEVICE_TYPE BIT(1u)
#define HA_EV_SUBS_DEVICE_ADDR BIT(2u)
#define HA_EV_SUBS_DEVICE_DATA BIT(3u)
#define HA_EV_SUBS_DEVICE_COMMAND BIT(4u)
#define HA_EV_SUBS_DEVICE_ERROR BIT(5u)
#define HA_EV_SUBS_FUNCTION BIT(6u)

// #define HA_EV_SUBS_PARAMS 7u

#define HA_EV_SUBS_SUBSCRIBED(_subs) (atomic_test_bit(&_subs->flags, HA_EV_SUBS_FLAG_SUBSCRIBED_BIT))

// #define HA_EV_SUBS_FLAG_ALL BIT(0u)

typedef struct ha_ev_subs_conf
{
	uint32_t flags;

	/* Depends on filter */
	event_subs_filter_func_t func;
	const ha_dev_mac_t *device_addr;
	ha_dev_type_t device_type;
} ha_ev_subs_conf_t;

/**
 * @brief Notify waiters of an event
 * 
 * @param event 
 * @return int 
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




extern const struct ha_device_api ha_device_api_xiaomi;
extern const struct ha_device_api ha_device_api_caniot;
extern const struct ha_device_api ha_device_api_f429zi;

int ha_dev_register_xiaomi_record(const xiaomi_record_t *record);
const struct ha_xiaomi_dataset *ha_ev_get_xiaomi_data(const ha_ev_t *ev);

int ha_dev_register_die_temperature(uint32_t timestamp, float die_temperature);
float ha_ev_get_die_temperature(const ha_ev_t *ev);

int ha_dev_register_caniot_telemetry(uint32_t timestamp,
				     caniot_did_t did,
				     const struct caniot_board_control_telemetry *data);
const struct caniot_board_control_telemetry *ha_ev_get_caniot_telemetry(const ha_ev_t *ev);

struct ha_room_assoc
{	
	ha_room_id_t rid;
	ha_dev_addr_t addr;
};

struct ha_room *ha_dev_get_room(ha_dev_t *const dev);

#endif /* _HA_DEVS_H_ */