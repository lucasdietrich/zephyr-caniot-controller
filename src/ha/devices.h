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

#include <zephyr.h>

#include <stddef.h>

#include "../ble/xiaomi_record.h"
#include <caniot/caniot.h>
#include <caniot/datatype.h>

#include "config.h"

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

typedef struct {
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

struct ha_caniot_dataset
{
	struct {
		int16_t value; /* 1e-2 °C */
		ha_dev_sensor_type_t type;
	} temperatures[HA_CANIOT_MAX_TEMPERATURES];

	union {
		struct {
			uint8_t rl1 : 1;
			uint8_t rl2 : 1;
			uint8_t oc1 : 1;
			uint8_t oc2 : 1;
			uint8_t in1 : 1;
			uint8_t in2 : 1;
			uint8_t in3 : 1;
			uint8_t in4 : 1;
		};
		uint8_t dio;
	};
};

struct ha_f429zi_dataset
{
	float die_temperature; /* °C */
};

struct ha_dev_stats_t
{
	uint32_t rx; /* number of received packets */
	uint32_t rx_bytes; /* number of received bytes */

	uint32_t tx; /* number of transmitted packets */
	uint32_t tx_bytes; /* number of transmitted bytes */

	uint32_t max_inactivity; /* number of seconds without any activity */
};

typedef struct {
	ha_dev_addr_t addr;

	/* UNIX timestamps in seconds */
	uint32_t registered_timestamp;
	struct {
		uint32_t measurements_timestamp;

		union {
			struct ha_xiaomi_dataset xiaomi;
			struct ha_caniot_dataset caniot;
			struct ha_f429zi_dataset nucleo_f429zi;
		};
	} data;

	/* TODO handle */
	struct ha_dev_stats_t stats;
} ha_dev_t;

bool ha_dev_valid(ha_dev_t *const dev);

typedef void ha_dev_iterate_cb_t(ha_dev_t *dev,
				 void *user_data);

int ha_dev_addr_cmp(const ha_dev_addr_t *a,
		    const ha_dev_addr_t *b);

size_t ha_dev_iterate(ha_dev_iterate_cb_t callback,
		      ha_dev_filter_t *filter,
		      void *user_data);

size_t ha_dev_iterate_filter_by_type(ha_dev_iterate_cb_t callback,
				  void *user_data,
				  ha_dev_type_t type);

static inline size_t ha_dev_xiaomi_iterate(ha_dev_iterate_cb_t callback,
					   void *user_data)
{
	return ha_dev_iterate_filter_by_type(callback,
					  user_data,
					  HA_DEV_TYPE_XIAOMI_MIJIA);
}


static inline size_t ha_dev_caniot_iterate(ha_dev_iterate_cb_t callback,
					   void *user_data)
{
	return ha_dev_iterate_filter_by_type(callback,
					     user_data,
					     HA_DEV_TYPE_CANIOT);
}

static inline void ha_dev_inc_stats_rx(ha_dev_t *dev, uint32_t rx_bytes)
{
	__ASSERT(dev != NULL, "dev is NULL");

	dev->stats.rx_bytes += rx_bytes;
	dev->stats.rx++;
}

static inline void ha_dev_inc_stats_tx(ha_dev_t *dev, uint32_t tx_bytes)
{
	__ASSERT(dev != NULL, "dev is NULL");

	dev->stats.tx_bytes += tx_bytes;
	dev->stats.tx++;
}

int ha_register_xiaomi_from_dataframe(xiaomi_dataframe_t *frame);

int ha_dev_register_die_temperature(uint32_t timestamp,
				    float die_temperature);

int ha_dev_register_caniot_telemetry(uint32_t timestamp,
				     caniot_did_t did,
				     struct caniot_board_control_telemetry *data);

#endif /* _HA_DEVS_H_ */