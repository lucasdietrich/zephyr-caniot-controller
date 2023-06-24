#ifndef _HA_DATA_H_
#define _HA_DATA_H_

#include <stdint.h>

#include <zephyr/sys/slist.h>

#if defined(CONFIG_CANIOT_LIB)
#include <caniot/datatype.h>
#endif

typedef enum {
	HA_SUBSYS_UNASSIGNED = 0u,

	HA_SUBSYS_SOC_TEMPERATURE,
	HA_SUBSYS_BOARD_TEMPERATURE,
	HA_SUBSYS_EXTERNAL_TEMPERATURE_SENSOR,

	HA_SUBSYS_BOARD_HUMIDITY,
	HA_SUBSYS_EXTERNAL_HUMIDITY_SENSOR,

	HA_SUBSYS_OPEN_COLLECTOR,
	HA_SUBSYS_RELAY,

	HA_SUBSYS_DIGITAL_IO,
} ha_data_subsystem_t;

#define HA_DATA_STD_TYPE_OFFSET		0x00u
#define HA_DATA_SPECIAL_TYPE_OFFSET 0x80u

typedef enum {
	HA_DATA_UNSPEC = HA_DATA_STD_TYPE_OFFSET,

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
	HA_DATA_XPS = HA_DATA_SPECIAL_TYPE_OFFSET,
	HA_DATA_TS,
	HA_DATA_ONOFF,
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
	ha_dev_sensor_type_t sens_type;
};

struct ha_data_humidity {
	uint16_t value; /* 1e-2 % */
	ha_dev_sensor_type_t sens_type;
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

struct ha_data_onoff {
	caniot_onestate_cmd_t status : 1u;
};
#endif

struct ha_data_descr {
	const char *name;
	ha_data_subsystem_t subsys : 8u;
	ha_data_type_t type : 8u;
	uint32_t offset : 16u;
};

#define HA_DATA_DESCR_NAMED(_struct, _name, _member, _type, _ss)                       \
	{                                                                                    \
		.name = _name, .subsys = _ss, .type = _type,                                  \
		.offset = offsetof(_struct, _member),                                            \
	}

#define HA_DATA_DESCR(_struct, _member, _type, _meas)                                    \
	HA_DATA_DESCR_NAMED(_struct, NULL, _member, _type, _meas)

#define HA_DATA_DESCR_UNASSIGNED(_struct, _member, _type)                                \
	HA_DATA_DESCR(_struct, _member, _type, HA_SUBSYS_UNASSIGNED)

/**
 * @brief Get the value matching the requested type from a data structure using
 * its descriptor.
 *
 * Note: Pointer needs to be casted to the correct type.
 *
 * @param data Data structure to get the value from
 * @param descr Descriptor of the given data
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


typedef union {
	struct ha_data_temperature temperature;
	struct ha_data_humidity humidity;
	struct ha_data_battery_level battery_level;
	struct ha_data_rssi rssi;
	struct ha_data_digital digital;
	struct ha_data_digital_in digital_in;
	struct ha_data_digital_out digital_out;
	struct ha_data_analog analog;
	struct ha_shutter_position shutter_position;
#if defined(CONFIG_CANIOT_LIB)
	struct ha_heater_mode heater_mode;
	struct ha_data_xps xps;
	struct ha_data_ts ts;
	struct ha_data_onoff onoff;
#endif
} ha_data_value_storage_t;

struct ha_data {
	sys_snode_t _node;			/* Use to link data together */
	ha_data_type_t type;		/*	Type of the data represented by this structure */
	ha_data_subsystem_t subsys; /* Subsystem the data belongs to */
	uint8_t occurence;	/* Subsystem index if multiple data of the same type/subsys */
	uint8_t value[];	/* Data buffer (size depends on the type) */
};
typedef struct ha_data ha_data_t;

/* !!! This structure is only used for data interpretation, and should never be allocated directly !!! */
struct ha_data_storage {
	sys_snode_t _node;			/* Use to link data together */
	ha_data_type_t type;		/*	Type of the data represented by this structure */
	ha_data_subsystem_t subsys; /* Subsystem the data belongs to */
	uint8_t occurence;	   		/* Subsystem index if multiple data of the same type/subsys */
	ha_data_value_storage_t value; 		/* Data buffer (size depends on the type) */
};
typedef struct ha_data_storage ha_data_storage_t;

size_t ha_data_get_size_by_type(ha_data_type_t type);

/**
 * @brief Allocate a buffer with the correct size for the given type.
 * 
 * @param type Type of the data to allocate.
 * @return ha_data_t* 
 */
ha_data_t *ha_data_alloc(ha_data_type_t type);

/**
 * @brief Free the memory allocated by ha_data_alloc().
 * 
 * @param data 
 */
void ha_data_free(ha_data_t *data);

int ha_data_alloc_array(ha_data_t **array, uint8_t count, ha_data_type_t type);

int ha_data_free_array(ha_data_t **array, uint8_t count);

/**
 * @brief Convert ha_data_type_t enumeration to a string representation.
 *
 * This function is used to provide a human-readable string of each enumeration of
 * ha_data_type_t. If the type is not recognized, the function will return "Unknown".
 *
 * @param type An enumerated value of ha_data_type_t.
 * @return const char* The string representation of the input type.
 */
const char *ha_data_type_to_str(ha_data_type_t type);

/**
 * @brief Convert ha_dev_sensor_type_t enumeration to a string representation.
 *
 * This function is used to provide a human-readable string of each enumeration of
 * ha_dev_sensor_type_t. If the type is not recognized, the function will return
 * "Unknown".
 *
 * @param assignement An enumerated value of ha_dev_sensor_type_t.
 * @return const char* The string representation of the input assignment.
 */
const char *ha_data_subsystem_to_str(ha_data_subsystem_t subsystem);

#endif /* _HA_DATA_H_ */