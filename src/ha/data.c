#include <zephyr.h>

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
	case HA_DATA_DIGITAL:
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

void *ha_data_get(void *data,
		  const struct ha_data_descr *descr,
		  size_t data_descr_size,
		  ha_data_type_t type,
		  uint8_t index)
{
	if (!data || !descr || !data_descr_size)
		return NULL;

	const struct ha_data_descr *d;

	for (d = descr; d < descr + data_descr_size; d++) {
		if ((d->type == type) && (index-- == 0)) {
			return (uint8_t *)data + d->offset;
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