/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* HA Specific header
 * TODO refactor in order to get rid of it in the future */

#ifndef _HA_SPE_H_
#define _HA_SPE_H_

#include <zephyr/bluetooth/addr.h>

#if defined(CONFIG_CANIOT_LIB)
#include <caniot/caniot.h>
#include <caniot/datatype.h>
#endif

#if defined(CONFIG_APP_HA_EMULATED_DEVICES)
#define HA_CANIOT_MAX_DEVICES 63u
#define HA_XIAOMI_MAX_DEVICES 40u
#else
#define HA_CANIOT_MAX_DEVICES 5U
#define HA_XIAOMI_MAX_DEVICES 15U
#endif

#define HA_DEV_FILTER_CAN _HA_DEV_FILTER_BY_DEVICE_MEDIUM(HA_DEV_MEDIUM_CAN)
#define HA_DEV_FILTER_BLE _HA_DEV_FILTER_BY_DEVICE_MEDIUM(HA_DEV_MEDIUM_BLE)

#define HA_DEV_FILTER_CANIOT	   _HA_DEV_FILTER_BY_DEVICE_TYPE(HA_DEV_TYPE_CANIOT)
#define HA_DEV_FILTER_XIAOMI_MIJIA _HA_DEV_FILTER_BY_DEVICE_TYPE(HA_DEV_TYPE_XIAOMI_MIJIA)
#define HA_DEV_FILTER_NUCLEO_F429ZI                                                      \
	_HA_DEV_FILTER_BY_DEVICE_TYPE(HA_DEV_TYPE_NUCLEO_F429ZI)

typedef enum {
	HA_DEV_MEDIUM_NONE = 0,
	HA_DEV_MEDIUM_BLE, /* MAC address e.g. 23:45:67:89:AB:CD */
	HA_DEV_MEDIUM_CAN, /* CAN ID e.g. 0x123 or extended e.g. 0x12345678 */
} ha_dev_medium_type_t;

typedef enum {
	HA_DEV_TYPE_NONE = 0,
	HA_DEV_TYPE_XIAOMI_MIJIA, /* Xiaomi Mijia LYWSD03MMC */
	HA_DEV_TYPE_CANIOT,	  /* CANIOT device addr e.g. 18 */
	HA_DEV_TYPE_NUCLEO_F429ZI,
} ha_dev_type_t;

typedef struct {
	uint32_t id : 29u;
	uint32_t ext : 1u;
	uint32_t bus : 2u;
} can_addr_t;

typedef union {
	bt_addr_le_t ble;
#if defined(CONFIG_CANIOT_LIB)
	caniot_did_t caniot;
#endif
	can_addr_t can;
} ha_dev_mac_addr_t;

typedef enum {
	HA_DEV_ENDPOINT_NONE, /* Mean either: UNDEFINED endpoint or ANY endpoint
			       */
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
	HA_DEV_ENDPOINT_CANIOT_SHUTTERS,
} ha_endpoint_id_t;

#endif /* _HA_SPE_H_ */