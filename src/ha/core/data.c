#include "data.h"
#include "ha.h"

#include <zephyr/kernel.h>

size_t get_data_size(ha_data_type_t type)
{
	static const uint32_t std_sizes[] = {
		[HA_DATA_TEMPERATURE]	   = sizeof(struct ha_data_temperature),
		[HA_DATA_HUMIDITY]	   = sizeof(struct ha_data_humidity),
		[HA_DATA_BATTERY_LEVEL]	   = sizeof(struct ha_data_battery_level),
		[HA_DATA_RSSI]		   = sizeof(struct ha_data_rssi),
		[HA_DATA_DIGITAL_INOUT]	   = sizeof(struct ha_data_digital),
		[HA_DATA_ANALOG]	   = sizeof(struct ha_data_analog),
		[HA_DATA_HEATER_MODE]	   = sizeof(struct ha_heater_mode),
		[HA_DATA_SHUTTER_POSITION] = sizeof(struct ha_shutter_position),
	};

	if (type < HA_DATA_SPECIAL_TYPE_OFFSET) {
		return std_sizes[type];
	}

	switch (type) {
	case HA_DATA_XPS:
		return sizeof(struct ha_data_xps);
	case HA_DATA_TS:
		return sizeof(struct ha_data_ts);
	case HA_DATA_ONOFF:
		return sizeof(struct ha_data_onoff);
	default:
		return 0;
	}
}

void *ha_data_get(void *data_structure,
		  const struct ha_data_descr *descr,
		  size_t data_descr_size,
		  ha_data_type_t type,
		  uint8_t occurence)
{
	if (!data_structure || !descr || !data_descr_size) return NULL;

	const struct ha_data_descr *d;

	for (d = descr; d < descr + data_descr_size; d++) {
		if ((d->type == type) && (occurence-- == 0)) {
			return (uint8_t *)data_structure + d->offset;
			break;
		}
	}

	return NULL;
}

bool ha_data_descr_data_type_has(const struct ha_data_descr *descr,
				 size_t data_descr_size,
				 ha_data_type_t type)
{
	if (!descr) {
		return false;
	}

	const struct ha_data_descr *d;

	for (d = descr; d < descr + data_descr_size; d++) {
		if (d->type == type) {
			return true;
		}
	}

	return false;
}

uint32_t ha_data_descr_data_types_mask(const struct ha_data_descr *descr,
				       size_t data_descr_size)
{
	if (!descr) return 0;

	uint32_t mask = 0;
	const struct ha_data_descr *d;

	for (d = descr; d < descr + data_descr_size; d++) {
		mask |= (1 << d->type);
	}

	return mask;
}

int ha_data_descr_extract(const struct ha_data_descr *descr,
			  size_t data_descr_size,
			  void *data_structure,
			  void *destination,
			  size_t index)
{
	if (!descr || !data_structure || !destination) return -EINVAL;

	if (index >= data_descr_size) return -ENOENT;

	memcpy(destination,
	       (uint8_t *)data_structure + descr[index].offset,
	       get_data_size(descr[index].type));

	return 0;
}