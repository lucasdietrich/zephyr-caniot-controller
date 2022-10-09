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
	uint8_t value; /* % */
	uint16_t mvoltage; /* 1e-3 V */
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

typedef struct ha_data
{
	sys_snode_t _node;

	ha_data_type_t type;
	
	union {
		void *data;
		// struct ha_data_temperature *temp;
		// struct ha_data_humidity *hum;
		// struct ha_data_battery_level *bat;
		// struct ha_data_rssi *rssi;
		// struct ha_data_digital *dig;
		// struct ha_data_analog *analog;
	};
} ha_data_t;

ha_data_t *ha_data_alloc(ha_data_type_t type);

void ha_data_free(ha_data_t *data);

void ha_data_append(sys_slist_t *list, ha_data_t *data);

void ha_data_free_list(sys_slist_t *list);

struct ha_event;

ha_data_t *ha_ev_data_alloc_queue(struct ha_event *ev,
				  ha_data_type_t type);

#endif /* _HA_DATA_H_ */