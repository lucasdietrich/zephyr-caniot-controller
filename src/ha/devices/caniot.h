/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_DEVICES_CANIOT_H
#define _HA_DEVICES_CANIOT_H

#include "ha/core/ha.h"
#include "ha/core/data.h"

#define HA_CANIOT_CLS0_MAX_TEMPERATURES 		4u
#define HA_CANIOT_CLS1_MAX_TEMPERATURES 		4u
#define HA_CANIOT_MAX_TEMPERATURES 4U

/* blt stands for board level telemetry */
struct ha_ds_caniot_blc0
{
	struct ha_data_temperature temperatures[HA_CANIOT_MAX_TEMPERATURES];
	struct ha_data_digital dio;
	struct ha_data_digital pdio;
};

struct ha_cmd_caniot_blc0
{
	struct ha_data_xps oc1;
	struct ha_data_xps oc2;
	struct ha_data_xps rl1;
	struct ha_data_xps rl2;
};

struct ha_ds_caniot_blc1
{
	struct ha_data_temperature temperatures[HA_CANIOT_MAX_TEMPERATURES];
	struct ha_data_digital dio;
};

struct ha_cmd_caniot_blc1
{
	union {
		struct ha_data_xps pc0;
		struct ha_data_xps pc1;
		struct ha_data_xps pc2;
		struct ha_data_xps pc3;

		struct ha_data_xps pd4;
		struct ha_data_xps pd5;
		struct ha_data_xps pd6;
		struct ha_data_xps pd7;

		struct ha_data_xps eio0;
		struct ha_data_xps eio1;
		struct ha_data_xps eio2;
		struct ha_data_xps eio3;
		struct ha_data_xps eio4;
		struct ha_data_xps eio5;
		struct ha_data_xps eio6;
		struct ha_data_xps eio7;

		struct ha_data_xps pb0;
		struct ha_data_xps pe0;
		struct ha_data_xps pe1;
	};
	struct ha_data_xps io[19u];
};

struct ha_ds_caniot_heating_control
{
	struct ha_heater_mode heaters[4u];
};

struct ha_cmd_caniot_heating_control
{
	struct ha_heater_mode heaters[4u];
};

struct ha_ds_caniot_shutters_control
{
	struct ha_shutter_position shutters[4u];
};

struct ha_cmd_caniot_shutters_control
{
	struct ha_shutter_position shutters[4u];
};


#define HA_DEV_CANIOT_MAC_INIT(_did) \
	(ha_dev_mac_t) { \
		.medium = HA_DEV_MEDIUM_CAN, \
		.addr = { .caniot = _did }, \
	}

#define HA_DEV_CANIOT_ADDR_INIT(_did) \
	{ \
		.type = HA_DEV_TYPE_CANIOT, \
		.mac = HA_DEV_CANIOT_MAC_INIT(_did)\
	}

/* feed a board level telemetry dataset from a received CAN BLT/BCT buffer */
void ha_dev_caniot_blc_cls0_to_blt(struct ha_ds_caniot_blc0 *blt,
				   const struct caniot_blc0_telemetry *can_buf);

void ha_dev_caniot_blc_cls1_to_blt(struct ha_ds_caniot_blc1 *blt,
				   const struct caniot_blc1_telemetry *can_buf);

int ha_dev_register_caniot_telemetry(uint32_t timestamp,
				     caniot_did_t did,
				     char buf[8u],
				     caniot_id_t *id);

const struct caniot_blc0_telemetry *ha_ev_get_caniot_telemetry(const ha_ev_t *ev);

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

#endif /* _HA_DEVICES_CANIOT_H */