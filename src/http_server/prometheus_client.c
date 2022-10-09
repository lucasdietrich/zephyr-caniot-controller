/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file prometheus_client.c
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Prometheus exporter
 * @version 0.1
 * @date 2022-03-30
 *
 * @copyright Copyright (c) 2022
 *
 * Note: Check metrics format with command `curl -s http://192.168.10.240/metrics | promtool check metrics`
 *
 * TODO:
 * - Add support for tag default value
 * - Check metric and tag names
 * - Do show the measurement if it was not updated in the last 5 minutes, or since the last update
 *
 *
 * Expected output (per device):
 *  device_temperature{medium="BLE",mac="A4:C1:38:68:05:63",device="LYWSD03MMC",sensor="EMBEDDED",room="",collector="f429"} 23.63
 *  device_humidity{medium="BLE",mac="A4:C1:38:68:05:63",device="LYWSD03MMC",sensor="EMBEDDED",room="",collector="f429"} 39
 *  device_battery_level{medium="BLE",mac="A4:C1:38:68:05:63",device="LYWSD03MMC",sensor="EMBEDDED",room="",collector="f429"} 2.899
 *  device_measurements_last_timestamp{medium="BLE",mac="A4:C1:38:68:05:63",device="LYWSD03MMC",sensor="EMBEDDED",room="",collector="f429"} 1648764416
 *
 */

#include "prometheus_client.h"

#include "ha/devices.h"
#include "ha/devices/caniot.h"
#include "ha/devices/xiaomi.h"
#include "ha/devices/garage.h"
#include "ha/devices/f429zi.h"

#include "utils/buffers.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(prom, LOG_LEVEL_DBG);

/* Number of metrics to encode between each HTTP response buffer flush */
#define CONFIG_PROMETHEUS_METRICS_PER_FLUSH 4u

typedef enum {
	VALUE_ENCODING_TYPE_INT32,
	VALUE_ENCODING_TYPE_UINT32,
	VALUE_ENCODING_TYPE_FLOAT,
	VALUE_ENCODING_TYPE_FLOAT_DIGITS,
	VALUE_ENCODING_TYPE_EXP,
	VALUE_ENCODING_TYPE_EXP_DIGITS,
} metric_encoding_type_t;

struct metric_value {
	/* The value of the metric */
	union {
		float fvalue; /* store value as float */
		uint32_t uvalue; /* store value as unsigned */
		int32_t svalue; /* store value as signed */
	};

	/* Encoding type in the case of a float value */
	struct {
		/* Float encoding type: float, exp ...*/
		metric_encoding_type_t type;

		/* additionnal arg, e.g. digits for precision */
		union {
			uint8_t digits;
		};
	} encoding;

	/* Values for tags */
	const char **tags_values;

	/* Number of tags values */
	uint8_t tags_values_count;
};

typedef enum {
	COUNTER = 0,
	GAUGE,
	HISTOGRAM,
	SUMMARY,
} metric_type_t;

static const char *get_metric_type_str(metric_type_t type)
{
	switch (type) {
	case COUNTER:
		return "counter";
	case GAUGE:
		return "gauge";
	case HISTOGRAM:
		return "histogram";
	case SUMMARY:
		return "summary";
	default:
		return "";
	}
}

struct metric_tag {
	/* tag name */
	char *name;

	/* TODO tag default value */
	// char *def;
};

struct metric_definition
{
	/* metric name */
	const char *name;

	/* metric type */
	metric_type_t type;

	/* metric tags list */
	const struct metric_tag *tags;

	/* metric tags count */
	uint8_t tags_count;

	/* metric help */
	const char *help;
};

#define METRIC_TAG(n) { .name = n }

#define METRIC_DEF_TAGS(n, t, tags_list, h) \
{ \
        .name = n, \
        .type = t, \
        .tags = tags_list, \
        .tags_count = ARRAY_SIZE(tags_list), \
        .help = h \
}

#define METRIC_DEF(n, t, h) \
{ \
        .name = n, \
        .type = t, \
        .tags = NULL, \
        .tags_count = 0U, \
        .help = h \
}

static const struct metric_tag device_measurement_tags[] = {
	/* medium type : ble, can */
	METRIC_TAG("medium"),

	/* mac address: BLE MAC address (if ble), DeviceID (if caniot device) */
	METRIC_TAG("mac"),

	/* device type : Xioami, caniot, ... */
	METRIC_TAG("device"),

	/* sensor type : embedded, extern */
	METRIC_TAG("sensor"),

	/* device room: where the device is located */
	METRIC_TAG("room"),

	/* which device collected the data */
	METRIC_TAG("collector"),
};

const struct metric_definition mdef_device_rssi =
METRIC_DEF_TAGS("device_rssi", GAUGE, device_measurement_tags,
		"Device rssi (in dBm)");
		
const struct metric_definition mdef_device_temperature =
METRIC_DEF_TAGS("device_temperature", GAUGE, device_measurement_tags,
		"Device temperature (in °C)");

const struct metric_definition mdef_device_humidity =
METRIC_DEF_TAGS("device_humidity", GAUGE, device_measurement_tags,
		"Device humidity (in %)");

const struct metric_definition mdef_device_battery_level =
METRIC_DEF_TAGS("device_battery_level", GAUGE, device_measurement_tags,
		"Device battery level (in %)");

const struct metric_definition mdef_device_battery_voltage =
METRIC_DEF_TAGS("device_battery_voltage", GAUGE, device_measurement_tags,
		"Device battery voltage (in V)");

const struct metric_definition mdef_device_measurements_last_timestamp =
METRIC_DEF_TAGS("device_measurements_last_timestamp", GAUGE, device_measurement_tags,
		"Timestamp of the last device measurement (UTC time)");


static bool validate_metric_value(struct metric_value *value)
{
	bool success = value != NULL;

	success &= (value->tags_values_count == 0) || (value->tags_values != NULL);

	// success &= (value->encoding.type >= VALUE_ENCODING_TYPE_UINT32) &&
	// 	(value->encoding.type <= VALUE_ENCODING_TYPE_EXP);

	return success;
}

static bool validate_metric_definition(const struct metric_definition *definition)
{
	bool success = definition != NULL;

	success &= definition->name != NULL;

	success &= (definition->type >= COUNTER) && (definition->type <= SUMMARY);

	success &= (definition->tags_count == 0) || (definition->tags != NULL);

	return success;
}

static ssize_t encode_value(char *buf, size_t buf_size, struct metric_value *value)
{
	ssize_t ret = -EINVAL;

	switch (value->encoding.type) {
	case VALUE_ENCODING_TYPE_INT32:
		ret = snprintf(buf, buf_size, "%d", value->svalue);
		break;
	case VALUE_ENCODING_TYPE_UINT32:
		ret = snprintf(buf, buf_size, "%u", value->uvalue);
		break;
	case VALUE_ENCODING_TYPE_FLOAT_DIGITS:
		ret = snprintf(buf, buf_size, "%.*f",
			       (int)value->encoding.digits, value->fvalue);
		break;
	case VALUE_ENCODING_TYPE_EXP:
		ret = snprintf(buf, buf_size, "%e", value->fvalue);
		break;
	case VALUE_ENCODING_TYPE_EXP_DIGITS:
		ret = snprintf(buf, buf_size, "%.*e",
			       (int)value->encoding.digits, value->fvalue);
		break;
	case VALUE_ENCODING_TYPE_FLOAT:
	default:
		ret = snprintf(buf, buf_size, "%f", value->fvalue);
		break;
	}

	return ret;
}

/**
 * @brief Encode metric value using the metric definition
 *
 * @param buf Buffer to store the encoded value
 * @param len Buffer length
 * @param value Metric value and tags values
 * @param metric Metric definition
 * @param meta Tells if meta data TYPE and HELP should be prepended to the metric value
 * @return ssize_t
 */
static ssize_t encode_metric(buffer_t *buffer,
			     struct metric_value *value,
			     const struct metric_definition *metric,
			     bool meta)
{
	if ((validate_metric_value(value) == false) ||
	    (validate_metric_definition(metric) == false)) {
		return -EINVAL;
	}

	int ret;
	ssize_t appended = 0U;

	const char *const metric_name = metric->name;

	/* encode meta information HELP and TYPE */
	if (meta) {
		if (metric->help != NULL) {
			const char *strings[] = {
				"# HELP ",
				metric_name,
				" ",
				metric->help,
				"\n",
			};
			ret = buffer_append_strings(buffer, strings,
						    ARRAY_SIZE(strings));
			if (ret < 0) {
				return ret;
			}
			appended += ret;
		}

		const char *strings[] = {
			"# TYPE ",
			metric_name,
			" ",
			get_metric_type_str(metric->type),
			"\n",
		};

		ret = buffer_append_strings(buffer, strings,
					    ARRAY_SIZE(strings));
		if (ret < 0) {
			return ret;
		}

		appended += ret;
	}

	/* check for tags */
	const uint8_t tags_count = metric->tags_count;
	const bool has_tags = tags_count > 0;

	/* build strings address array */
	const size_t count =
		4U + (has_tags ? 1U + 4U * tags_count : 0U);
	const char *strings[count];

	/* encode value to float string */
	char value_str[20];
	ret = encode_value(value_str, sizeof(value_str), value);
	if (ret < 0) {
		LOG_ERR("Failed to encode metric value, buffer too small %u",
			sizeof(value_str));
		return ret;
	}

	/* prepare strings array */
	strings[0] = metric_name;
	strings[count - 3] = " ";
	strings[count - 2] = value_str;
	strings[count - 1] = "\n";

	/* prepare tags */
	if (has_tags) {
		strings[1] = "{";

		for (uint8_t i = 0U; i < tags_count; i++) {
			const struct metric_tag *const tag = &metric->tags[i];
			const char *tag_name = tag->name;
			const char *tag_value = NULL;
			if (value->tags_values_count > i) {
				tag_value = value->tags_values[i];
			}
			if (tag_value == NULL) {
				tag_value = "";
			}

			strings[4 * i + 2] = tag_name;
			strings[4 * i + 3] = "=\"";
			strings[4 * i + 4] = tag_value;
			strings[4 * i + 5] = "\",";
		}

		/* overwrite last comma */
		strings[count - 4] = "\"}";
	}

	/* encode actual metric */
	ret = buffer_append_strings(buffer, strings, count);
	if (ret < 0) {
		return ret;
	}

	return appended + ret;
}




/**
 * @brief @see "struct json_obj_descr" in :
 * https://github.com/zephyrproject-rtos/zephyr/blob/main/include/zephyr/data/json.h
 */
struct prom_metric_descr
{
	const struct metric_definition *def;
	
	const char *metric_name;

	uint32_t align_shift : 2;
	uint32_t metric_name_len : 7;
	metric_encoding_type_t type : 7;
	uint32_t offset : 16;
};

#define Z_ALIGN_SHIFT(type)	(__alignof__(type) == 1 ? 0 : \
				 __alignof__(type) == 2 ? 1 : \
				 __alignof__(type) == 4 ? 2 : 3)

#define PROM_METRIC_DESCR(struct_, field_name_, type_, def_) \
	{ \
		.def = def_, \
		.metric_name = (#field_name_), \
		.align_shift = Z_ALIGN_SHIFT(struct_), \
		.metric_name_len = sizeof(#field_name_) - 1, \
		.type = type_, \
		.offset = offsetof(struct_, field_name_), \
	}
	
#define PROM_METRIC_DESCR_NAMED(struct_, metric_field_name_, \
				struct_field_name_, type_, def_) \
	{ \
		.def = def_, \
		.metric_name = (metric_field_name_), \
		.align_shift = Z_ALIGN_SHIFT(struct_), \
		.metric_name_len = sizeof(metric_field_name_) - 1, \
		.type = type_, \
		.offset = offsetof(struct_, struct_field_name_), \
	}



const char *prom_myd_medium_to_str(ha_dev_medium_type_t medium)
{
	switch (medium) {
	case HA_DEV_MEDIUM_BLE:
		return "BLE";
	case HA_DEV_MEDIUM_CAN:
		return "CAN";
	default:
		return "";
	}
}

const char *prom_myd_device_type_to_str(ha_dev_type_t device_type)
{
	switch (device_type) {
	case HA_DEV_TYPE_CANIOT:
		return "CANIOT";
	case HA_DEV_TYPE_XIAOMI_MIJIA:
		return "LYWSD03MMC";
	case HA_DEV_TYPE_NUCLEO_F429ZI:
		return "NUCLEO_F429ZI";
	default:
		return "";
	}
}

const char *prom_myd_sensor_type_to_str(ha_dev_sensor_type_t sensor_type)
{
	switch (sensor_type) {
	case HA_DEV_SENSOR_TYPE_EMBEDDED:
		return "EMBEDDED";
	case HA_DEV_SENSOR_TYPE_EXTERNAL1:
		return "EXTERNAL";
	case HA_DEV_SENSOR_TYPE_EXTERNAL2:
		return "EXTERNAL2";
	case HA_DEV_SENSOR_TYPE_EXTERNAL3:
		return "EXTERNAL3";
	default:
		return "";
	}
}




struct prom_demo_struct {
	uint32_t a;
	float b;
	int32_t c;
};

/* iterator for this struct */
__attribute__((used)) static const struct prom_metric_descr demo_descr[] = {
	PROM_METRIC_DESCR_NAMED(struct prom_demo_struct, "vara", a, VALUE_ENCODING_TYPE_UINT32, NULL),
	PROM_METRIC_DESCR_NAMED(struct prom_demo_struct, "varb", b, VALUE_ENCODING_TYPE_FLOAT, NULL),
	PROM_METRIC_DESCR(struct prom_demo_struct, c, VALUE_ENCODING_TYPE_INT32, NULL),
};

int prometheus_metrics_demo(http_request_t *req,
			    http_response_t *resp)
{
	const char *tags1[] = {
		"BLE"
		"00:00:00:00:00:00",
		"Xioami",
		"EMB",
		"Lucas' Bedroom",
		"f429"
	};

	const char *tags2[] = {
		"BLE",
		"11:11:11:11:11:11",
		"Xioami"
		"EMB",
		"Kitchen",
		"f429"
	};

	struct metric_value val1 = {
		.fvalue = 24.723,
		.encoding = {
			.type = VALUE_ENCODING_TYPE_FLOAT,
		},
		.tags_values = tags1,
		.tags_values_count = ARRAY_SIZE(tags1)
	};

	struct metric_value val2 = {
		.fvalue = -17.234,
		.encoding = {
			.type = VALUE_ENCODING_TYPE_EXP_DIGITS,
			.digits = 3,
		},
		.tags_values = tags2,
		.tags_values_count = ARRAY_SIZE(tags2)
	};

	encode_metric(&resp->buffer, &val1,
		      &mdef_device_temperature, true);

	encode_metric(&resp->buffer, &val2,
		      &mdef_device_temperature, false);

	resp->status_code = 200;

	return 0;
}

union measurements_tags_values
{
	struct {
		const char *medium;
		const char *mac;
		const char *device;
		const char *sensor;
		const char *room;
		const char *collector;
	};
	const char *list[6];
};



static void prom_metric_feed_dev_measurement_timestamp(uint32_t timestamp,
						       struct metric_value *val)
{
	val->encoding.type = VALUE_ENCODING_TYPE_UINT32;
	val->uvalue = timestamp;
}

static void prom_metric_feed_xiaomi_temperature(const struct ha_ds_xiaomi *dt,
						struct metric_value *val)
{
	val->encoding.type = VALUE_ENCODING_TYPE_FLOAT_DIGITS;
	val->encoding.digits = 2U;
	val->fvalue = dt->temperature.value / 100.0;
}

static void prom_metric_feed_xiaomi_humidity(const struct ha_ds_xiaomi *dt,
					     struct metric_value *val)
{
	val->encoding.type = VALUE_ENCODING_TYPE_FLOAT_DIGITS;
	val->encoding.digits = 3U;
	val->fvalue = dt->humidity / 100.0;
}

static void prom_metric_feed_xiaomi_battery_level(const struct ha_ds_xiaomi *dt,
						  struct metric_value *val)
{
	val->encoding.type = VALUE_ENCODING_TYPE_UINT32;
	val->uvalue = dt->battery_level;
}

static void prom_metric_feed_xiaomi_rssi(const struct ha_ds_xiaomi *dt,
					 struct metric_value *val)
{
	val->encoding.type = VALUE_ENCODING_TYPE_INT32;
	val->svalue = (float) dt->rssi;
}

static void prom_metric_feed_xiaomi_battery_voltage(const struct ha_ds_xiaomi *dt,
						  struct metric_value *val)
{
	val->encoding.type = VALUE_ENCODING_TYPE_FLOAT_DIGITS;
	val->encoding.digits = 3U;
	val->fvalue = dt->battery_mv / 1000.0;
}

static bool prom_ha_devs_iterate_cb(ha_dev_t *dev,
				    void *user_data)
{
	buffer_t *const buffer = (buffer_t *)user_data;

	if (dev->addr.type == HA_DEV_TYPE_XIAOMI_MIJIA) {

		char mac_addr[BT_ADDR_STR_LEN];

		bt_addr_to_str(&dev->addr.mac.addr.ble.a,
			       mac_addr, sizeof(mac_addr));

		const struct ha_ds_xiaomi *const dt = 
			HA_DEV_EP0_GET_CAST_LAST_DATA(dev, const struct ha_ds_xiaomi);

		union measurements_tags_values tags_values = {
			.medium = prom_myd_medium_to_str(dev->addr.mac.medium),
			.mac = mac_addr,
			.device = prom_myd_device_type_to_str(dev->addr.type),
			.sensor = prom_myd_sensor_type_to_str(dt->temperature.type),
			.room = "",
			.collector = "f429",
		};

		struct metric_value val = {
			.tags_values = tags_values.list,
			.tags_values_count = ARRAY_SIZE(tags_values.list)
		};

		prom_metric_feed_xiaomi_rssi(dt, &val);
		encode_metric(buffer, &val, &mdef_device_rssi, false);

		prom_metric_feed_xiaomi_temperature(dt, &val);
		encode_metric(buffer, &val, &mdef_device_temperature, false);

		prom_metric_feed_xiaomi_humidity(dt, &val);
		encode_metric(buffer, &val, &mdef_device_humidity, false);

		prom_metric_feed_xiaomi_battery_level(dt, &val);
		encode_metric(buffer, &val, &mdef_device_battery_level, false);

		prom_metric_feed_xiaomi_battery_voltage(dt, &val);
		encode_metric(buffer, &val, &mdef_device_battery_voltage, false);

		ha_ev_t *last_ev = ha_dev_get_last_event(dev, 0u);
		prom_metric_feed_dev_measurement_timestamp(last_ev->timestamp, &val);
		encode_metric(buffer, &val, &mdef_device_measurements_last_timestamp, false);

	} else if (dev->addr.type == HA_DEV_TYPE_CANIOT) {
		char caniot_addr_str[CANIOT_ADDR_LEN];

		caniot_encode_deviceid(dev->addr.mac.addr.caniot,
				       caniot_addr_str,
				       sizeof(caniot_addr_str));

		const struct ha_ds_caniot_blc0_telemetry *const dt = 
			HA_DEV_EP0_GET_CAST_LAST_DATA(dev, const struct ha_ds_caniot_blc0_telemetry);

		union measurements_tags_values tags_values = {
			.medium = prom_myd_medium_to_str(dev->addr.mac.medium),
			.mac = caniot_addr_str, /* can device id */
			.device = prom_myd_device_type_to_str(dev->addr.type),
			.sensor = prom_myd_sensor_type_to_str(
				HA_DEV_SENSOR_TYPE_EMBEDDED),
			.room = "",
			.collector = "f429",
		};

		struct metric_value val = {
			.tags_values = tags_values.list,
			.tags_values_count = ARRAY_SIZE(tags_values.list),
			.encoding = {
				.type = VALUE_ENCODING_TYPE_FLOAT,
				.digits = 2,
			}
		};
	
		for (size_t i = 0U; i < ARRAY_SIZE(dt->temperatures); i++) {
			ha_dev_sensor_type_t sensor_type =
				dt->temperatures[i].type;
			if (sensor_type != HA_DEV_SENSOR_TYPE_NONE) {
				val.fvalue = dt->temperatures[i].value / 100.0;
				tags_values.sensor = prom_myd_sensor_type_to_str(sensor_type);
				encode_metric(buffer, &val, &mdef_device_temperature, false);
			}
		}
		
		ha_ev_t *last_ev = ha_dev_get_last_event(dev, 0u);
		prom_metric_feed_dev_measurement_timestamp(last_ev->timestamp, &val);
		encode_metric(buffer, &val, &mdef_device_measurements_last_timestamp, false);

	} else if (dev->addr.type == HA_DEV_TYPE_NUCLEO_F429ZI) {
		const struct ha_ds_f429zi *const dt =
			HA_DEV_EP0_GET_CAST_LAST_DATA(dev, const struct ha_ds_f429zi);

		
		union measurements_tags_values tags_values = {
			.medium = "",
			.mac = "",
			.device = prom_myd_device_type_to_str(dev->addr.type),
			.sensor = prom_myd_sensor_type_to_str(
				HA_DEV_SENSOR_TYPE_EMBEDDED),
			.room = "",
			.collector = "f429",
		};

		struct metric_value val = {
			.tags_values = tags_values.list,
			.tags_values_count = ARRAY_SIZE(tags_values.list),
			.fvalue = dt->die_temperature,
			.encoding = {
				.type = VALUE_ENCODING_TYPE_FLOAT,
				.digits = 1
			}
		};

		encode_metric(buffer, &val, &mdef_device_temperature, false);

		ha_ev_t *last_ev = ha_dev_get_last_event(dev, 0u);
		prom_metric_feed_dev_measurement_timestamp(last_ev->timestamp, &val);
		encode_metric(buffer, &val, &mdef_device_measurements_last_timestamp, false);
	}

	return true;
}

int prometheus_metrics(http_request_t *req,
		       http_response_t *resp)
{
	/* Next index to encode metric into buffer */
	static uint32_t next_index;

	if (http_response_is_first_call(resp)) {
		/* Reset index */
		next_index = 0u;

		/* Enable chunked transfer encoding, because we don't know
		 * the size of the response in advance */
		http_response_enable_chunk_encoding(resp);
	}

	const ha_dev_filter_t filter = {
		.flags =
			HA_DEV_FILTER_DATA_EXIST |
			HA_DEV_FILTER_FROM_INDEX |
			HA_DEV_FILTER_TO_INDEX |
			HA_DEV_FILTER_DEVICE_TYPE,
		.from_index = next_index,
		.to_index = next_index + CONFIG_PROMETHEUS_METRICS_PER_FLUSH,
		.device_type = HA_DEV_TYPE_XIAOMI_MIJIA,
	};

	size_t count = ha_dev_iterate(prom_ha_devs_iterate_cb, &filter, 
				      &HA_DEV_ITER_OPT_LOCK_ALL(),
				      (void *)&resp->buffer);

	/* Check wether there are more metrics to encode */
	if (count == CONFIG_PROMETHEUS_METRICS_PER_FLUSH) {
		http_response_more_data(resp);
		
		next_index += count;
	}

	resp->status_code = 200;

	return 0;
}

int prometheus_metrics_controller(http_request_t *req,
				  http_response_t *resp)
{
	return -EINVAL;
}