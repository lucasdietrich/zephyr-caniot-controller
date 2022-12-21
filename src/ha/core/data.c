#include <zephyr/kernel.h>

#include "data.h"

#include "devices.h"

size_t get_data_size(ha_data_type_t type)
{
	switch (type) {
	case HA_DATA_TEMPERATURE:
		return sizeof(struct ha_data_temperature);
	case HA_DATA_HUMIDITY:
		return sizeof(struct ha_data_humidity);
	case HA_DATA_BATTERY_LEVEL:
		return sizeof(struct ha_data_battery_level);
	case HA_DATA_RSSI:
		return sizeof(struct ha_data_rssi);
	case HA_DATA_DIGITAL_INOUT:
		return sizeof(struct ha_data_digital);
	case HA_DATA_ANALOG:
		return sizeof(struct ha_data_analog);
	case HA_DATA_HEATER_MODE:
		return sizeof(struct ha_heater_mode);
	case HA_DATA_SHUTTER_POSITION:
		return sizeof(struct ha_shutter_position);
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
	if (!data_structure || !descr || !data_descr_size)
		return NULL;

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
	if (!descr)
		return 0;

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
	if (!descr || !data_structure || !destination)
		return -EINVAL;

	if (index >= data_descr_size)
		return -ENOENT;

	memcpy(destination,
	       (uint8_t *)data_structure + descr[index].offset,
	       get_data_size(descr[index].type));

	return 0;
}