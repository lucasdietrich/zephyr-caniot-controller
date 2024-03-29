/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_DEVICES_XIAOMI_H
#define _HA_DEVICES_XIAOMI_H

#include "ha/core/ha.h"

#include <zephyr/kernel.h>

#define XIAOMI_BT_LE_ADDR_0 0xA4U
#define XIAOMI_BT_LE_ADDR_1 0xC1U
#define XIAOMI_BT_LE_ADDR_2 0x38U

/* size is 7B */
typedef struct {
	/**
	 * @brief RSSI
	 */
	int8_t rssi;
	/**
	 * @brief Device measured temperature, base unit : 1e-2 °C
	 */
	int16_t temperature;

	/**
	 * @brief Device measured humidity, base unit : 1e-2 %
	 */
	uint16_t humidity; /* 1e-2 % */

	/**
	 * @brief Device measured battery voltage, base unit: 1 mV
	 */
	uint16_t battery_mv;

	/**
	 * @brief Device measured battery level, base unit:  %
	 * Measurement is valid if battery_level > 0
	 */
	uint8_t battery_level;
} xiaomi_measurements_t;

typedef struct {
	/**
	 * @brief Record device address
	 */
	bt_addr_le_t addr;

	/**
	 * @brief Time when the measurements were retrieved
	 */
	uint32_t time;

	/**
	 * @brief Measurements
	 */
	xiaomi_measurements_t measurements;

	/**
	 * @brief Tells whether record is valid
	 */
	uint32_t valid;
} xiaomi_record_t;

struct ha_ds_xiaomi {
	struct ha_data_rssi rssi;
	struct ha_data_humidity humidity;
	struct ha_data_temperature temperature;
	struct ha_data_battery_level battery_level;
};

#define HA_BT_ADDR_LE_PUBLIC_INIT(_b0, _b1, _b2, _b3, _b4, _b5)                          \
	(bt_addr_le_t)                                                                       \
	{                                                                                    \
		.type = BT_ADDR_LE_PUBLIC, .a =                                                  \
		{.val = {                                                                        \
			 _b0,                                                                        \
			 _b1,                                                                        \
			 _b2,                                                                        \
			 _b3,                                                                        \
			 _b4,                                                                        \
			 _b5,                                                                        \
		 } }                                                                             \
	}

#define HA_DEV_BLE_MAC_INIT(_b0, _b1, _b2, _b3, _b4, _b5)                                \
	{                                                                                    \
		.medium = HA_DEV_MEDIUM_CAN, .addr = {                                           \
			.ble = HA_BT_ADDR_LE_PUBLIC_INIT(_b0, _b1, _b2, _b3, _b4, _b5)               \
		}                                                                                \
	}

#define HA_DEV_XIAOMI_ADDR_INIT(_b3, _b4, _b5)                                           \
	{                                                                                    \
		.type = HA_DEV_TYPE_XIAOMI_MIJIA,                                                \
		.mac  = HA_DEV_BLE_MAC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1,            \
									XIAOMI_BT_LE_ADDR_2, _b3, _b4, _b5)                  \
	}

void ha_dev_xiaomi_record_init(xiaomi_record_t *record);

int ha_dev_xiaomi_register_record(const xiaomi_record_t *record);

const struct ha_ds_xiaomi *ha_ev_get_xiaomi_data(const ha_ev_t *ev);

static inline ssize_t ha_dev_xiaomi_iterate_data(ha_dev_iterate_cb_t callback,
												 void *user_data)
{
	const ha_dev_filter_t filter = {
		.flags		 = HA_DEV_FILTER_DATA_EXIST | HA_DEV_FILTER_DEVICE_TYPE,
		.device_type = HA_DEV_TYPE_XIAOMI_MIJIA,
	};

	return ha_dev_iterate(callback, &filter, NULL, user_data);
}

#endif /* _HA_DEVICES_XIAOMI_H */