/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_H_
#define _HA_H_

#include <zephyr.h>

#include <bluetooth/addr.h>
#include <caniot/caniot.h>
#include <caniot/datatype.h>

#define XIAOMI_BT_LE_ADDR_0 0xA4U
#define XIAOMI_BT_LE_ADDR_1 0xC1U
#define XIAOMI_BT_LE_ADDR_2 0x38U

#define HA_CANIOT_MAX_DEVICES 5U
#define HA_XIAOMI_MAX_DEVICES 15U
#define HA_OTHER_MAX_DEVICES 5U

#define HA_MAX_DEVICES (HA_CANIOT_MAX_DEVICES + HA_XIAOMI_MAX_DEVICES + HA_OTHER_MAX_DEVICES)

// TODO move to CANIOT library
#define HA_CANIOT_MAX_TEMPERATURES 4U

typedef enum {
	HA_DEV_MEDIUM_NONE = 0,
	HA_DEV_MEDIUM_BLE,
	HA_DEV_MEDIUM_CAN,
} ha_dev_medium_type_t;

typedef enum {
	HA_DEV_TYPE_NONE = 0,
	HA_DEV_TYPE_XIAOMI_MIJIA, /* Xiaomi Mijia LYWSD03MMC */
	HA_DEV_TYPE_CANIOT,
	HA_DEV_TYPE_NUCLEO_F429ZI,
} ha_dev_type_t;

typedef union {
	bt_addr_le_t ble;
	caniot_did_t caniot;
} ha_dev_mac_addr_t;

typedef struct
{
	ha_dev_medium_type_t medium;
	ha_dev_mac_addr_t addr;
} ha_dev_mac_t;

typedef struct ha_device_addr {
	ha_dev_type_t type;
	ha_dev_mac_t mac;
} ha_dev_addr_t;

typedef enum {
	HA_DEV_SENSOR_TYPE_NONE = 0,
	HA_DEV_SENSOR_TYPE_EMBEDDED,
	HA_DEV_SENSOR_TYPE_EXTERNAL1,
	HA_DEV_SENSOR_TYPE_EXTERNAL2,
	HA_DEV_SENSOR_TYPE_EXTERNAL3,
} ha_dev_sensor_type_t;

#endif /* _HA_H_ */