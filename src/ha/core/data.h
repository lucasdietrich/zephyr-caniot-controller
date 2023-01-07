#ifndef _HA_DATA_H_
#define _HA_DATA_H_

#include <stdint.h>

#include <zephyr/sys/slist.h>

#if defined(CONFIG_CANIOT_LIB)
#include <caniot/datatype.h>
#endif

typedef enum {
	HA_ASSIGN_UNASSIGNED = 0u,

	HA_ASSIGN_SOC_TEMPERATURE,
	HA_ASSIGN_BOARD_TEMPERATURE,
	HA_ASSIGN_EXTERNAL_TEMPERATURE_SENSOR,

	HA_ASSIGN_BOARD_HUMIDITY,
	HA_ASSIGN_EXTERNAL_HUMIDITY_SENSOR,

	HA_ASSIGN_OPEN_COLLECTOR,
	HA_ASSIGN_RELAY,

	HA_ASSIGN_DIGITAL_IO,

} ha_data_assignement_t;

typedef enum {
	HA_DATA_UNSPEC = 0u,

	/* Standard types */
	HA_DATA_TEMPERATURE,
	HA_DATA_HUMIDITY,
	HA_DATA_BATTERY_LEVEL,
	HA_DATA_RSSI,
	HA_DATA_DIGITAL_INOUT, /* Mix of in and out */
	HA_DATA_DIGITAL_IN,
	HA_DATA_DIGITAL_OUT,
	HA_DATA_ANALOG,
	HA_DATA_HEATER_MODE,
	HA_DATA_SHUTTER_POSITION,

	/* Special types */
	HA_DATA_XPS = 0x80u,
	HA_DATA_TS,
	HA_DATA_SS,
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
	uint8_t level;	  /* % */
	uint16_t voltage; /* 1e-3 V */
};

struct ha_data_rssi {
	int8_t value; /* dBm */
};

struct ha_data_digital {
	uint32_t value; /* 1 bit per pin */
	uint32_t mask;	/* 1 if active*/
};

struct ha_data_digital_in {
	uint32_t value; /* 1 bit per pin */
	uint32_t mask;	/* 1 if active*/
};

struct ha_data_digital_out {
	uint32_t value; /* 1 bit per pin */
	uint32_t mask;	/* 1 if active*/
};

struct ha_data_analog {
	uint32_t value; /* 1e-6 V */
};

struct ha_shutter_position {
	uint8_t position; /* % */
	uint8_t moving;	  /* 0: stopped, 1: moving */
};

#if defined(CONFIG_CANIOT_LIB)
struct ha_heater_mode {
	caniot_heating_status_t mode : 8u;
};

struct ha_data_xps {
	caniot_complex_digital_cmd_t cmd : 3u;
};

struct ha_data_ts {
	caniot_twostate_cmd_t cmd : 2u;
};

struct ha_data_ss {
	caniot_onestate_cmd_t cmd : 1u;
};
#endif

struct ha_data_descr {
	const char *name;
	ha_data_assignement_t measure : 8u;
	ha_data_type_t type : 8u;
	uint32_t offset : 16u;
};

#define HA_DATA_DESCR_NAMED(_struct, _name, _member, _type, _meas)                       \
	{                                                                                \
		.name = _name, .measure = _meas, .type = _type,                          \
		.offset = offsetof(_struct, _member),                                    \
	}

#define HA_DATA_DESCR(_struct, _member, _type, _meas)                                    \
	HA_DATA_DESCR_NAMED(_struct, NULL, _member, _type, _meas)

#define HA_DATA_DESCR_UNASSIGNED(_struct, _member, _type)                                \
	HA_DATA_DESCR(_struct, _member, _type, HA_ASSIGN_UNASSIGNED)

/**
 * @brief Get the value matching the requested type from a data structure using
 * its descriptor.
 *
 * Note: Pointer needs to be casted to the correct type.
 *
 * @param data Data structure to get the value from
 * @param descr Descriptor of the data structure
 * @param data_descr_size Descriptor size
 * @param type Type of the data to get
 * @param occurence Occurence of the data type in the descriptor
 * @return void* Pointer to the data value
 */
void *ha_data_get(void *data,
		  const struct ha_data_descr *descr,
		  size_t data_descr_size,
		  ha_data_type_t type,
		  uint8_t occurence);

/**
 * @brief Tell whether a descriptor contains a given data type.
 *
 * @param descr Descriptor to check
 * @param data_descr_size Descriptor size
 * @param type Type to search for
 * @return true
 * @return false
 */
bool ha_data_descr_data_type_has(const struct ha_data_descr *descr,
				 size_t data_descr_size,
				 ha_data_type_t type);

/**
 * @brief Calculate a mask of all data types supported in a descriptor.
 *
 * @param descr Descriptor to compute the mask from
 * @param data_descr_size Descriptor size
 * @return uint32_t Mask of data types
 */
uint32_t ha_data_descr_data_types_mask(const struct ha_data_descr *descr,
				       size_t data_descr_size);

/**
 * @brief Extract data at given index from a data structure using a descriptor
 *
 * @param descr
 * @param data_descr_size
 * @param data_structure
 * @param data_extract
 * @param index
 * @return int 0 on success, negative value on error
 */
int ha_data_descr_extract(const struct ha_data_descr *descr,
			  size_t data_descr_size,
			  void *data_structure,
			  void *data_extract,
			  size_t index);

#endif /* _HA_DATA_H_ */