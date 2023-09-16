#include "data.h"
#include "ha.h"

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>

#define HA_DATA_MAX_SIZE 64u

#define _(_x)                                                                            \
	case _x:                                                                             \
		return #_x

size_t ha_data_type_get_value_size(ha_data_type_t type)
{
	static const uint32_t std_sizes[] = {
		[HA_DATA_TEMPERATURE]	   = sizeof(struct ha_data_temperature),
		[HA_DATA_HUMIDITY]		   = sizeof(struct ha_data_humidity),
		[HA_DATA_BATTERY_LEVEL]	   = sizeof(struct ha_data_battery_level),
		[HA_DATA_RSSI]			   = sizeof(struct ha_data_rssi),
		[HA_DATA_DIGITAL_INOUT]	   = sizeof(struct ha_data_digital),
		[HA_DATA_ANALOG]		   = sizeof(struct ha_data_analog),
		[HA_DATA_HEATER_MODE]	   = sizeof(struct ha_heater_mode),
		[HA_DATA_SHUTTER_POSITION] = sizeof(struct ha_shutter_position),
	};

	static const size_t spe_cizes[] = {
		[HA_DATA_XPS - HA_DATA_SPECIAL_TYPE_OFFSET]	  = sizeof(struct ha_data_xps),
		[HA_DATA_TS - HA_DATA_SPECIAL_TYPE_OFFSET]	  = sizeof(struct ha_data_ts),
		[HA_DATA_ONOFF - HA_DATA_SPECIAL_TYPE_OFFSET] = sizeof(struct ha_data_onoff),
	};

	if (type < HA_DATA_SPECIAL_TYPE_OFFSET) {
		return std_sizes[type];
	} else if (type < _HA_DATA_MAX_TYPE) {
		return spe_cizes[type - HA_DATA_SPECIAL_TYPE_OFFSET];
	} else {
		return 0;
	}
}

const char *ha_data_type_to_str_dbg(ha_data_type_t type)
{
	switch (type) {
		_(HA_DATA_UNSPEC);
		_(HA_DATA_TEMPERATURE);
		_(HA_DATA_HUMIDITY);
		_(HA_DATA_BATTERY_LEVEL);
		_(HA_DATA_RSSI);
		_(HA_DATA_DIGITAL_INOUT);
		_(HA_DATA_DIGITAL_IN);
		_(HA_DATA_DIGITAL_OUT);
		_(HA_DATA_ANALOG);
		_(HA_DATA_HEATER_MODE);
		_(HA_DATA_SHUTTER_POSITION);
		_(HA_DATA_XPS);
		_(HA_DATA_TS);
		_(HA_DATA_ONOFF);
	default:
		return "<unknown data type>";
	}
}

const char *ha_data_type_to_str(ha_data_type_t type)
{
	const char *std_types[] = {[HA_DATA_UNSPEC]			  = "NA",
							   [HA_DATA_TEMPERATURE]	  = "temperature",
							   [HA_DATA_HUMIDITY]		  = "humidity",
							   [HA_DATA_BATTERY_LEVEL]	  = "battery level",
							   [HA_DATA_RSSI]			  = "rssi",
							   [HA_DATA_DIGITAL_INOUT]	  = "digital in/out",
							   [HA_DATA_DIGITAL_IN]		  = "digital in",
							   [HA_DATA_DIGITAL_OUT]	  = "digital out",
							   [HA_DATA_ANALOG]			  = "analog",
							   [HA_DATA_HEATER_MODE]	  = "heater mode",
							   [HA_DATA_SHUTTER_POSITION] = "shutter position"};

	const char *spe_types[] = {
		[HA_DATA_XPS - HA_DATA_SPECIAL_TYPE_OFFSET]	  = "xps",
		[HA_DATA_TS - HA_DATA_SPECIAL_TYPE_OFFSET]	  = "ts",
		[HA_DATA_ONOFF - HA_DATA_SPECIAL_TYPE_OFFSET] = "onoff",
	};

	if (type < HA_DATA_SPECIAL_TYPE_OFFSET) {
		return std_types[type];
	} else if (type < _HA_DATA_MAX_TYPE) {
		return spe_types[type - HA_DATA_SPECIAL_TYPE_OFFSET];
	} else {
		return std_types[HA_DATA_UNSPEC];
	}
}

const char *ha_data_subsystem_to_str_dbg(ha_data_subsystem_t subsystem)
{
	switch (subsystem) {
		_(HA_DEV_SENSOR_TYPE_NONE);
		_(HA_DEV_SENSOR_TYPE_EMBEDDED);
		_(HA_DEV_SENSOR_TYPE_EXTERNAL1);
		_(HA_DEV_SENSOR_TYPE_EXTERNAL2);
		_(HA_DEV_SENSOR_TYPE_EXTERNAL3);
	default:
		return "<unknown assignment>";
	}
}

const char *ha_data_subsystem_to_str(ha_data_subsystem_t subsystem)
{
	const char *subsystems[] = {
		[HA_DEV_SENSOR_TYPE_NONE]	   = "NA",
		[HA_DEV_SENSOR_TYPE_EMBEDDED]  = "embedded",
		[HA_DEV_SENSOR_TYPE_EXTERNAL1] = "external1",
		[HA_DEV_SENSOR_TYPE_EXTERNAL2] = "external2",
		[HA_DEV_SENSOR_TYPE_EXTERNAL3] = "external3",
	};

	if (subsystem < _HA_DEV_SENSOR_TYPE_MAX) {
		return subsystems[subsystem];
	} else {
		return subsystems[HA_DEV_SENSOR_TYPE_NONE];
	}
}

void *ha_data_get(void *data_structure,
				  const ha_data_descr_t *descr,
				  size_t data_descr_size,
				  ha_data_type_t type,
				  uint8_t occurence)
{
	if (!data_structure || !descr || !data_descr_size) return NULL;

	const ha_data_descr_t *d;

	for (d = descr; d < descr + data_descr_size; d++) {
		if ((d->type == type) && (occurence-- == 0)) {
			return (uint8_t *)data_structure + d->offset;
			break;
		}
	}

	return NULL;
}

bool ha_data_descr_data_type_has(const ha_data_descr_t *descr,
								 size_t data_descr_size,
								 ha_data_type_t type)
{
	if (!descr) {
		return false;
	}

	const ha_data_descr_t *d;

	for (d = descr; d < descr + data_descr_size; d++) {
		if (d->type == type) {
			return true;
		}
	}

	return false;
}

uint64_t ha_data_descr_data_types_mask(const ha_data_descr_t *descr,
									   size_t data_descr_size)
{
	if (!descr) return 0;

	uint64_t mask = 0llu;
	const ha_data_descr_t *d;

	for (d = descr; d < descr + data_descr_size; d++) {
		mask |= (1llu << d->type);
	}

	return mask;
}

size_t ha_data_descr_calc_data_buf_size(const ha_data_descr_t *descr,
										size_t data_descr_size)
{
	size_t size = 0u;

	if (descr && data_descr_size) {
		const ha_data_descr_t *d = &descr[data_descr_size - 1u];
		size					 = d->offset + ha_data_type_get_value_size(d->type);
	}

	return size;
}

int ha_data_iterate_descr(const void *data,
							 const ha_data_descr_t *descr,
							 size_t descr_size,
							 ha_data_iter_cb_t cb,
							 void *user_data)
{
	if (!descr || !descr || !descr_size || !cb) return -EINVAL;

	ha_data_storage_t storage;

	for (size_t i = 0; i < descr_size; i++) {
		storage._node.next = NULL;
		storage.subsys	   = descr[i].subsys;
		storage.type	   = descr[i].type;
		storage.occurence  = 0xFF; /* TODO: count occurences */
		memcpy(&storage.value, (uint8_t *)data + descr[i].offset,
			   ha_data_type_get_value_size(descr[i].type));

		if (cb(&storage, user_data) == false) {
			break;
		}
	}

	return 0;
}

int ha_data_iterate_slist(sys_slist_t *list, ha_data_iter_cb_t cb, void *user_data)
{
	if (!list || !cb) return -EINVAL;

	ha_data_storage_t *storage;

	SYS_SLIST_FOR_EACH_CONTAINER (list, storage, _node) {
		if (cb(storage, user_data) == false) {
			break;
		}
	}

	return 0;
}

int ha_data_descr_extract(const ha_data_descr_t *descr,
						  size_t data_descr_size,
						  void *data_structure,
						  void *destination,
						  size_t index)
{
	if (!descr || !data_structure || !destination) return -EINVAL;

	if (index >= data_descr_size) return -ENOENT;

	memcpy(destination, (uint8_t *)data_structure + descr[index].offset,
		   ha_data_type_get_value_size(descr[index].type));

	return 0;
}

static void data_buf_clear(ha_data_t *data, size_t size)
{
	__ASSERT_NO_MSG(data != NULL);

	memset(data, 0u, size);
}

ha_data_t *ha_data_alloc(ha_data_type_t type)
{
	size_t size = ha_data_type_get_value_size(type) + sizeof(ha_data_t);
	ha_data_t *data;

	/* Don't allocate if unknown type */
	if (size == 0) {
		return NULL;
	}

	data = (ha_data_t *)k_malloc(size);

	/* Reset structure */
	if (data != NULL) {
		data_buf_clear(data, size);

		/* Set type as it is already known */
		data->type = type;
	}

	return data;
}

void ha_data_free(ha_data_t *data)
{
	k_free(data);
}

int ha_data_alloc_array(ha_data_t **array, uint8_t count, ha_data_type_t type)
{
	size_t size = ha_data_type_get_value_size(type) + sizeof(ha_data_t);
	ha_data_t *p;

	/* Don't allocate if unknown type */
	if (!array || !count || (size == 0)) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < count; i++) {
		p = (ha_data_t *)k_malloc(size);

		if (p != NULL) {
			data_buf_clear(p, size);
			p->type	 = type;
			array[i] = p;
		} else {
			ha_data_free_array(array, i);
			return -ENOMEM;
		}
	}

	return 0;
}

int ha_data_free_array(ha_data_t **array, uint8_t count)
{
	if (!array || !count) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < count; i++) {
		ha_data_free(array[i]);
	}

	return 0;
}

int ha_data_encode_value(char *buf,
						 size_t buf_len,
						 ha_data_type_t type,
						 const ha_data_value_storage_t *value)
{
	int ret;
	size_t size = ha_data_type_get_value_size(type);

	if (!buf || !buf_len || !value || !size) return -EINVAL;

	switch (type) {
	case HA_DATA_TEMPERATURE:
		ret = snprintf(buf, buf_len, "%.2f °C", value->temperature.value / 100.0);
		break;
	case HA_DATA_HUMIDITY:
		ret = snprintf(buf, buf_len, "%.2f %%", value->humidity.value / 100.0);
		break;
	case HA_DATA_BATTERY_LEVEL:
		ret = snprintf(buf, buf_len, "%hhu %% (%hu mV)", value->battery_level.level,
					   value->battery_level.voltage);
		break;
	case HA_DATA_RSSI:
		ret = snprintf(buf, buf_len, "%hhd dBm", value->rssi.value);
		break;
	case HA_DATA_DIGITAL_INOUT:
		ret = snprintf(buf, buf_len, "%x/%x", value->digital.value, value->digital.mask);
		break;
	case HA_DATA_HEATER_MODE:
		ret = snprintf(buf, buf_len, "%s",
					   caniot_heating_status_to_str(value->heater_mode.mode));
		break;
	default:
		ret = -ENOTSUP;
	}

	if ((size_t)ret >= buf_len) {
		ret = -ENOMEM;
	}

	return ret;
}