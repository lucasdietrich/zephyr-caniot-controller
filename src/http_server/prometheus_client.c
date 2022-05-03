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

#include "utils.h"

#include <caniot/caniot.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(prom, LOG_LEVEL_DBG);

typedef enum {
	VALUE_ENCODING_TYPE_UINT32,
	VALUE_ENCODING_TYPE_FLOAT,
	VALUE_ENCODING_TYPE_FLOAT_DIGITS,
	VALUE_ENCODING_TYPE_EXP,
	VALUE_ENCODING_TYPE_EXP_DIGITS,
} metric_encoding_type_t;

struct metric_value {
	/* The value of the metric */
        float value;

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

const struct metric_definition mdef_device_temperature =
	METRIC_DEF_TAGS("device_temperature", GAUGE, device_measurement_tags,
	"Device temperature (in °C)");

const struct metric_definition mdef_device_humidity =
	METRIC_DEF_TAGS("device_humidity", GAUGE, device_measurement_tags, 
	"Device humidity (in %)");

const struct metric_definition mdef_device_battery_level =
	METRIC_DEF_TAGS("device_battery_level", GAUGE, device_measurement_tags, 
	"Device battery level (in V)");

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
	case VALUE_ENCODING_TYPE_UINT32:
		ret = snprintf(buf, buf_size, "%u", (uint32_t)value->value);
		break;
	case VALUE_ENCODING_TYPE_FLOAT_DIGITS:
		ret = snprintf(buf, buf_size, "%.*f",
			       (int)value->encoding.digits, value->value);
		break;
	case VALUE_ENCODING_TYPE_EXP:
		ret = snprintf(buf, buf_size, "%e", value->value);
		break;
	case VALUE_ENCODING_TYPE_EXP_DIGITS:
		ret = snprintf(buf, buf_size, "%.*e",
			       (int)value->encoding.digits, value->value);
		break;
	case VALUE_ENCODING_TYPE_FLOAT:
	default:
		ret = snprintf(buf, buf_size, "%f", value->value);
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

/*___________________________________________________________________________*/

const char *prom_myd_medium_to_str(ha_dev_medium_type_t medium)
{
	switch (medium) {
	case HA_DEV_MEDIUM_TYPE_BLE:
		return "BLE";
	case HA_DEV_MEDIUM_TYPE_CAN:
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
	switch(sensor_type) {
	case HA_DEV_SENSOR_TYPE_EMBEDDED:
		return "EMBEDDED";
	case HA_DEV_SENSOR_TYPE_EXTERNAL:
		return "EXTERNAL";
	default:
		return "";
	}
}

/*___________________________________________________________________________*/

int prometheus_metrics_demo(struct http_request *req,
			    struct http_response *resp)
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
		.value = 24.723,
		.encoding = {
			.type = VALUE_ENCODING_TYPE_FLOAT,
		},
		.tags_values = tags1,
		.tags_values_count = ARRAY_SIZE(tags1)
	};

	struct metric_value val2 = {
		.value = -17.234,
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

static void prom_metric_feed_xiaomi_temperature(struct ha_dev *dev,
					 struct metric_value *val)
{
	val->encoding.type = VALUE_ENCODING_TYPE_FLOAT_DIGITS;
	val->encoding.digits = 2;
	val->value = dev->data.xiaomi.temperature.value / 100.0;
}

static void prom_metric_feed_xiaomi_humidity(struct ha_dev *dev,
				      struct metric_value *val)
{
	val->encoding.type = VALUE_ENCODING_TYPE_UINT32;
	val->value = dev->data.xiaomi.humidity;
}

static void prom_metric_feed_xiaomi_battery_level(struct ha_dev *dev,
				     struct metric_value *val)
{
	val->encoding.type = VALUE_ENCODING_TYPE_FLOAT_DIGITS;
	val->encoding.digits = 3;
	val->value = dev->data.xiaomi.battery_level / 1000.0;
}

static void prom_metric_feed_xiaomi_measurement_timestamp(struct ha_dev *dev,
							  struct metric_value *val)
{
	val->encoding.type = VALUE_ENCODING_TYPE_UINT32;
	val->value = dev->data.measurements_timestamp;
}

static void prom_ha_devs_iterate_cb(struct ha_dev *dev,
				      void *user_data)
{
	buffer_t *const buffer = (buffer_t *)user_data;

	if (dev->type == HA_DEV_TYPE_XIAOMI_MIJIA) {

		char mac_addr[BT_ADDR_STR_LEN];

		bt_addr_to_str(&dev->addr.addr.ble.a,
			       mac_addr, sizeof(mac_addr));

		union measurements_tags_values tags_values = {
			.medium = prom_myd_medium_to_str(dev->addr.medium),
			.mac = mac_addr,
			.device = prom_myd_device_type_to_str(dev->type),
			.sensor = prom_myd_sensor_type_to_str(
				dev->data.xiaomi.temperature.type),
			.room = "",
			.collector = "f429",
		};

		struct metric_value val = {
			.tags_values = tags_values.list,
			.tags_values_count = ARRAY_SIZE(tags_values.list)
		};

		prom_metric_feed_xiaomi_temperature(dev, &val);
		encode_metric(buffer, &val, &mdef_device_temperature, false);

		prom_metric_feed_xiaomi_humidity(dev, &val);
		encode_metric(buffer, &val, &mdef_device_humidity, false);

		prom_metric_feed_xiaomi_battery_level(dev, &val);
		encode_metric(buffer, &val, &mdef_device_battery_level, false);

		prom_metric_feed_xiaomi_measurement_timestamp(dev, &val);
		encode_metric(buffer, &val, &mdef_device_measurements_last_timestamp, false);

	} else if (dev->type == HA_DEV_TYPE_CANIOT) {
		char caniot_addr_str[CANIOT_ADDR_LEN];

		caniot_encode_deviceid(dev->addr.addr.caniot,
				       caniot_addr_str,
				       sizeof(caniot_addr_str));

		union measurements_tags_values tags_values = {
			.medium = prom_myd_medium_to_str(dev->addr.medium),
			.mac = caniot_addr_str, /* can device id */
			.device = prom_myd_device_type_to_str(dev->type),
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

		for (size_t i = 0U; i < ARRAY_SIZE(dev->data.caniot.temperatures); i++) {
			ha_dev_sensor_type_t sensor_type =
				dev->data.caniot.temperatures[i].type;
			if (sensor_type != HA_DEV_SENSOR_TYPE_NONE) {
				val.value = dev->data.caniot.temperatures[i].value / 100.0;
				tags_values.sensor = prom_myd_sensor_type_to_str(sensor_type);
				encode_metric(buffer, &val, &mdef_device_temperature, false);
			}
		}
		
	} else if (dev->type == HA_DEV_TYPE_NUCLEO_F429ZI) {
		union measurements_tags_values tags_values = {
			.medium = "",
			.mac = "",
			.device = prom_myd_device_type_to_str(dev->type),
			.sensor = prom_myd_sensor_type_to_str(
				HA_DEV_SENSOR_TYPE_EMBEDDED),
			.room = "",
			.collector = "f429",
		};

		struct metric_value val = {
			.tags_values = tags_values.list,
			.tags_values_count = ARRAY_SIZE(tags_values.list),
			.value = dev->data.nucleo_f429zi.die_temperature,
			.encoding = {
				.type = VALUE_ENCODING_TYPE_FLOAT,
				.digits = 1
			}
		};

		encode_metric(buffer, &val, &mdef_device_temperature, false);
	}
}

int prometheus_metrics(struct http_request *req,
		       struct http_response *resp)
{
	ha_dev_filter_t filter = {
		.type = HA_DEV_FILTER_NONE,
	};

	ha_dev_iterate(prom_ha_devs_iterate_cb,
			 &filter,
			 (void *)&resp->buffer);

	resp->status_code = 200;

	return 0;
}