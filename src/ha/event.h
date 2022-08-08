/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_EVENT_H_
#define _HA_EVENT_H_

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

#define HA_EV_TRIG_DEVICE_REGISTERED 1u
#define HA_EV_TRIG_DEVICE_UNREGISTERED 2u
#define HA_EV_TRIG_DEVICE_DATA 3u
#define HA_EV_TRIG_DEVICE_COMMAND 4u
#define HA_EV_TRIG_DEVICE_ERROR 5u
#define HA_EV_TRIG_FUNCTION 6u
#define HA_EV_TRIG_PARAMS 7u

typedef bool (*ha_event_trigger_func_t)(ha_event_t *event);

typedef struct ha_event_trigger
{
	/* Queue of events to be notified to the waiter */
	struct k_fifo evq;

	/* Flag describing on which event should the waiter be notified */
	atomic_val_t flags;

	union {
		ha_event_trigger_func_t func;
		void *params;
	};

} ha_ev_trig_t;

#endif /* _HA_EVENT_H_ */