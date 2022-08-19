/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_DATA_H_
#define _HA_DATA_H_

#include <stddef.h>
#include <stdint.h>

#include "../ble/xiaomi_record.h"
#include <caniot/caniot.h>
#include <caniot/datatype.h>

#define HA_CANIOT_MAX_DEVICES 5U
#define HA_XIAOMI_MAX_DEVICES 15U
#define HA_OTHER_MAX_DEVICES 5U

#define HA_MAX_DEVICES (HA_CANIOT_MAX_DEVICES + HA_XIAOMI_MAX_DEVICES + HA_OTHER_MAX_DEVICES)

// TODO move to CANIOT library
#define HA_CANIOT_MAX_TEMPERATURES 4U

/* TODO: Move these definitions to devices.h header */
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

typedef enum
{
	HA_DEV_FILTER_MEDIUM = BIT(0), /* filter by medium */
	HA_DEV_FILTER_DEVICE_TYPE = BIT(1), /* filter by device type */
	HA_DEV_FILTER_DATA_EXIST = BIT(2), /* Filter only by existing data */
	// HA_DEV_FILTER_SENSOR_TYPE, /* filter by temperature sensor type */
	HA_DEV_FILTER_DATA_TIMESTAMP = BIT(3), /* filter only devices with recent measurements */
	// HA_DEV_FILTER_REGISTERED_TIMESTAMP, /* filter only recent devices */
	HA_DEV_FILTER_HAS_TEMPERATURE = BIT(4), /* filter only devices with temperature sensor */
} ha_dev_filter_flags_t;

/* TODO Incompatible masks */

typedef struct
{
	ha_dev_filter_flags_t flags;

	/* filters values */
	ha_dev_medium_type_t medium;
	ha_dev_type_t device_type;
	uint32_t data_timestamp;
} ha_dev_filter_t;

struct ha_xiaomi_dataset
{
	int8_t rssi;

	uint16_t humidity; /* 1e-2 % */

	struct {
		int16_t value; /* 1e-2 °C */
		ha_dev_sensor_type_t type;
	} temperature;

	uint16_t battery_mv; /* mV */

	uint8_t battery_level; /* % */
};

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

struct ha_f429zi_dataset
{
	float die_temperature; /* °C */
};

/* feed a xiaomi dataset with a received ble frame */
void ha_data_ble_to_xiaomi(struct ha_xiaomi_dataset *xiaomi,
			   const xiaomi_record_t *rec);

/* feed a board level telemetry dataset from a received CAN BLT/BCT buffer */
void ha_data_can_to_blt(struct ha_caniot_blt_dataset *blt,
			const struct caniot_board_control_telemetry *can_buf);


#endif /* _HA_DATA_H_ */