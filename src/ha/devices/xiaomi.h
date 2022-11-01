/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_DEVICES_XIAOMI_H
#define _HA_DEVICES_XIAOMI_H

#include <zephyr/kernel.h>

#include "../ha.h"
#include "../devices.h"
#include "../ble/xiaomi_record.h"

struct ha_ds_xiaomi
{
	struct ha_data_rssi rssi;
	struct ha_data_humidity humidity;
	struct ha_data_temperature temperature;
	struct ha_data_battery_level battery_level;
};

#define HA_BT_ADDR_LE_PUBLIC_INIT(_b0, _b1, _b2, _b3, _b4, _b5) \
	(bt_addr_le_t) { \
		.type = BT_ADDR_LE_PUBLIC, \
		.a = { \
			.val = { \
				_b0, \
				_b1, \
				_b2, \
				_b3, \
				_b4, \
				_b5, \
			} \
		} \
	}

#define HA_DEV_BLE_MAC_INIT(_b0, _b1, _b2, _b3, _b4, _b5) \
	{ \
		.medium = HA_DEV_MEDIUM_CAN, \
		.addr = { \
			.ble = HA_BT_ADDR_LE_PUBLIC_INIT(_b0, _b1, _b2, _b3, _b4, _b5) \
		} \
	}

#define HA_DEV_XIAOMI_ADDR_INIT(_b3, _b4, _b5) \
	{ \
		.type = HA_DEV_TYPE_XIAOMI_MIJIA, \
		.mac = HA_DEV_BLE_MAC_INIT( \
			XIAOMI_BT_LE_ADDR_0, \
			XIAOMI_BT_LE_ADDR_1, \
			XIAOMI_BT_LE_ADDR_2, \
			_b3, \
			_b4, \
			_b5 \
		)\
	}

int ha_dev_register_xiaomi_record(const xiaomi_record_t *record);

const struct ha_ds_xiaomi *ha_ev_get_xiaomi_data(const ha_ev_t *ev);

#endif /* _HA_DEVICES_XIAOMI_H */