/**
 * @file mydevices.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-03-31
 * 
 * @copyright Copyright (c) 2022
 * 
 * TODO: shorten "mydevice" to "mydev" or "myd"
 * 
 */

#ifndef _MYDEVICES_H_
#define _MYDEVICES_H_

#include <stddef.h>

#include "ble/xiaomi_record.h"

typedef enum {
	MYDEVICE_MEDIUM_TYPE_NONE = 0,
	MYDEVICE_MEDIUM_TYPE_BLE,
	MYDEVICE_MEDIUM_TYPE_CAN,
} mydevice_medium_type_t;

typedef enum {
	MYDEVICE_TYPE_NONE = 0,
	MYDEVICE_TYPE_XIAOMI_MIJIA, /* Xiaomi Mijia LYWSD03MMC */
	MYDEVICE_TYPE_CANIOT,
} mydevice_type_t;

typedef struct {
	mydevice_medium_type_t medium;
	union {
		bt_addr_le_t ble;
		uint8_t caniot;
	} addr;
} mydevice_phys_addr_t;

typedef enum {
	MYDEVICE_SENSOR_TYPE_NONE = 0,
	MYDEVICE_SENSOR_TYPE_EMBEDDED,
	MYDEVICE_SENSOR_TYPE_EXTERNAL,
} mydevice_sensor_type_t;

typedef enum
{
	MYDEVICE_FILTER_NONE = 0,
	MYDEVICE_FILTER_TYPE_MEDIUM, /* filter by medium */
	MYDEVICE_FILTER_TYPE_DEVICE_TYPE, /* filter by device type */
	// MYDEVICE_FILTER_TYPE_SENSOR_TYPE, /* filter by temperature sensor type */
	MYDEVICE_FILTER_TYPE_MEASUREMENTS_TIMESTAMP, /* filter only devices with recent measurements */
	// MYDEVICE_FILTER_TYPE_REGISTERED_TIMESTAMP, /* filter only recent devices */
	MYDEVICE_FILTER_TYPE_HAS_TEMPERATURE, /* filter only devices with temperature sensor */
} mydevice_filter_type_t;

typedef struct
{
	mydevice_filter_type_t type;
	union {
		mydevice_medium_type_t medium;
		mydevice_type_t device_type;
		// mydevice_sensor_type_t sensor_type;
		uint32_t timestamp;
	} data;
} mydevice_filter_t;

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
				mydevice_sensor_type_t type;
			} temperature;
		} xiaomi;
		struct {
			struct {
				int16_t value; /* 1e-2 °C */
				mydevice_sensor_type_t type;
			} temperatures[2];

			uint8_t rl1 : 1;
			uint8_t rl2 : 1;
			uint8_t oc1 : 1;
			uint8_t oc2 : 1;
			uint8_t in1 : 1;
			uint8_t in2 : 1;
			uint8_t in3 : 1;
			uint8_t in4 : 1;
		} caniot;
	};
} mydevice_data_t;

struct mydevice {
	uint32_t registered_timestamp;

	mydevice_phys_addr_t addr;
	mydevice_type_t type;
	mydevice_data_t data;
};

size_t mydevice_iterate(void (*callback)(struct mydevice *dev,
					 void *user_data),
			mydevice_filter_t *filter,
			void *user_data);

size_t mydevice_xiaomi_iterate(void (*callback)(struct mydevice *dev,
						void *user_data),
			       void *user_data);

#endif /* _MYDEVICES_H_ */