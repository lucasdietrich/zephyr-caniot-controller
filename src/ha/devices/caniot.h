/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_DEVICES_CANIOT_H
#define _HA_DEVICES_CANIOT_H

#include "../ha.h"
#include "../devices.h"
#include "../ble/xiaomi_record.h"

/* blt stands for board level telemetry */
struct ha_caniot_blt_dataset
{
	struct {
		int16_t value; /* 1e-2 °C */
		ha_dev_sensor_type_t type;
	} temperatures[HA_CANIOT_MAX_TEMPERATURES];

	uint8_t dio;

	uint8_t pdio;
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
void ha_data_can_to_blt(struct ha_caniot_blt_dataset *blt,
			const struct caniot_board_control_telemetry *can_buf);

int ha_dev_register_caniot_telemetry(uint32_t timestamp,
				     caniot_did_t did,
				     const struct caniot_board_control_telemetry *data);

const struct caniot_board_control_telemetry *ha_ev_get_caniot_telemetry(const ha_ev_t *ev);

#endif /* _HA_DEVICES_CANIOT_H */