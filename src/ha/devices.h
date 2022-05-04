/**
 * @file ha_devs.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-03-31
 *
 * @copyright Copyright (c) 2022
 *
 * TODO: shorten "ha_dev" to "mydev" or "myd"
 *
 */

#ifndef _HA_DEVS_H_
#define _HA_DEVS_H_

#include <stddef.h>

#include "ble/xiaomi_record.h"
#include <caniot/caniot.h>
#include <caniot/datatype.h>

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

typedef struct {
	ha_dev_medium_type_t medium;
	union {
		bt_addr_le_t ble;
		union deviceid caniot;
	} mac;
} ha_dev_addr_t;

typedef enum {
	HA_DEV_SENSOR_TYPE_NONE = 0,
	HA_DEV_SENSOR_TYPE_EMBEDDED,
	HA_DEV_SENSOR_TYPE_EXTERNAL,
} ha_dev_sensor_type_t;

typedef enum
{
	HA_DEV_FILTER_NONE = 0,
	HA_DEV_FILTER_MEDIUM, /* filter by medium */
	HA_DEV_FILTER_DEVICE_TYPE, /* filter by device type */
	// HA_DEV_FILTER_SENSOR_TYPE, /* filter by temperature sensor type */
	HA_DEV_FILTER_MEASUREMENTS_TIMESTAMP, /* filter only devices with recent measurements */
	// HA_DEV_FILTER_REGISTERED_TIMESTAMP, /* filter only recent devices */
	HA_DEV_FILTER_HAS_TEMPERATURE, /* filter only devices with temperature sensor */
} ha_dev_filter_type_t;

typedef struct
{
	ha_dev_filter_type_t type;
	union {
		ha_dev_medium_type_t medium;
		ha_dev_type_t device_type;
		// ha_dev_sensor_type_t sensor_type;
		uint32_t timestamp;
	} data;
} ha_dev_filter_t;

typedef struct
{
	/* UNIX timestamps in seconds */
	uint32_t measurements_timestamp;

	union {
		struct {
			uint8_t humidity; /* % */
			uint16_t battery_level; /* mV */
			struct {
				int16_t value; /* 1e-2 °C */
				ha_dev_sensor_type_t type;
			} temperature;
		} xiaomi;
		struct {
			struct {
				int16_t value; /* 1e-2 °C */
				ha_dev_sensor_type_t type;
			} temperatures[3];

			uint8_t rl1 : 1;
			uint8_t rl2 : 1;
			uint8_t oc1 : 1;
			uint8_t oc2 : 1;
			uint8_t in1 : 1;
			uint8_t in2 : 1;
			uint8_t in3 : 1;
			uint8_t in4 : 1;
		} caniot;

		struct {
			float die_temperature; /* °C */
		} nucleo_f429zi;
	};
} ha_dev_data_t;

typedef struct {
	uint32_t registered_timestamp;

	ha_dev_addr_t addr;
	ha_dev_type_t type;
	ha_dev_data_t data;
} ha_dev_t;

bool ha_dev_valid(ha_dev_t *const dev);

size_t ha_dev_iterate(void (*callback)(ha_dev_t *dev,
				       void *user_data),
		      ha_dev_filter_t *filter,
		      void *user_data);

size_t ha_dev_xiaomi_iterate(void (*callback)(ha_dev_t *dev,
					      void *user_data),
			     void *user_data);

int ha_register_xiaomi_from_dataframe(xiaomi_dataframe_t *frame);

int ha_dev_register_die_temperature(uint32_t timestamp,
				    float die_temperature);

int ha_dev_register_caniot_telemetry(uint32_t timestamp,
				     union deviceid did,
				     struct caniot_board_control_telemetry *data);

int ha_dev_init(void);


#endif /* _HA_DEVS_H_ */