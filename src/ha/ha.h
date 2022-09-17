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

#define HA_DEV_ADDR_STR_MAX_LEN MAX(BT_ADDR_LE_STR_LEN, sizeof("0x1FFFFFFF"))
#define HA_DEV_ADDR_TYPE_STR_MAX_LEN 16u
#define HA_DEV_ADDR_MEDIUM_STR_MAX_LEN 10u

// TODO move to CANIOT library
#define HA_CANIOT_MAX_TEMPERATURES 4U

typedef enum {
	HA_DEV_MEDIUM_NONE = 0,
	HA_DEV_MEDIUM_BLE, 	/* MAC address e.g. 23:45:67:89:AB:CD */
	HA_DEV_MEDIUM_CAN, 	/* CAN ID e.g. 0x123 or extended e.g. 0x12345678 */
} ha_dev_medium_type_t;

typedef enum {
	HA_DEV_TYPE_NONE = 0,
	HA_DEV_TYPE_XIAOMI_MIJIA, /* Xiaomi Mijia LYWSD03MMC */
	HA_DEV_TYPE_CANIOT, 	/* CANIOT device addr e.g. 18 */
	HA_DEV_TYPE_NUCLEO_F429ZI,
} ha_dev_type_t;

typedef struct
{
	uint32_t id : 29u;
	uint32_t ext : 1u;
	uint32_t bus: 2u;
} can_addr_t;

typedef union {
	bt_addr_le_t ble;
	caniot_did_t caniot;
	can_addr_t can;
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