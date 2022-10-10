#ifndef _HA_DATA_H_
#define _HA_DATA_H_

#include <sys/slist.h>

#include <stdint.h>

#include <caniot/datatype.h>

typedef enum {
	HA_DATA_TEMPERATURE,
	HA_DATA_HUMIDITY,
	HA_DATA_BATTERY_LEVEL,
	HA_DATA_RSSI,
	HA_DATA_DIGITAL,
	HA_DATA_ANALOG,
	HA_DATA_HEATER_MODE,
	HA_DATA_SHUTTER_POSITION,
} ha_data_type_t;

typedef enum {
	HA_DEV_SENSOR_TYPE_NONE = 0,
	HA_DEV_SENSOR_TYPE_EMBEDDED,
	HA_DEV_SENSOR_TYPE_EXTERNAL1,
	HA_DEV_SENSOR_TYPE_EXTERNAL2,
	HA_DEV_SENSOR_TYPE_EXTERNAL3,
} ha_dev_sensor_type_t;

struct ha_data_temperature {
	int16_t value; /* 1e-2 °C */
	ha_dev_sensor_type_t type;
};

struct ha_data_humidity {
	uint16_t value; /* 1e-2 % */
	ha_dev_sensor_type_t type;
};

struct ha_data_battery_level {
	uint8_t level; /* % */
	uint16_t voltage; /* 1e-3 V */
};

struct ha_data_rssi {
	int8_t value; /* dBm */
};

struct ha_data_digital {
	uint32_t value;
	uint32_t mask;
};

struct ha_data_analog {
	uint32_t value; /* 1e-6 V */
};

struct ha_heater_mode {
	caniot_heating_status_t mode;
};

struct ha_shutter_position {
	uint8_t position; /* % */
	uint8_t moving; /* 0: stopped, 1: moving */
};

struct ha_data_descr
{
	ha_data_type_t type;
	uint16_t offset;
};

#define HA_DATA_DESCR(_struct, _member, _type) \
	{ \
		.type = _type, \
		.offset = offsetof(_struct, _member), \
	}

void *ha_data_get(void *data,
		  const struct ha_data_descr *descr,
		  size_t descr_size,
		  ha_data_type_t type,
		  uint8_t index);

bool ha_data_descr_data_type_has(const struct ha_data_descr *descr,
				 size_t descr_size,
				 ha_data_type_t type);

uint32_t ha_data_descr_data_types_mask(const struct ha_data_descr *descr,
				       size_t descr_size);

#endif /* _HA_DATA_H_ */