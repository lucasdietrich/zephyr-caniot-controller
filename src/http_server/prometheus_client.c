/**
 * @file prometheus_client.c
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Prometheus exporter 
 * @version 0.1
 * @date 2022-03-30
 * 
 * @copyright Copyright (c) 2022
 * 
 * TODO:
 * - Add support for tag default value
 * - Check metric and tag names
 * - Do show the measurement if it was not updated in the last 5 minutes, or since the last update
 * 
 * Expected output:
 * # HELP device_temperature Device temperature
 * # TYPE device_temperature gauge
 * device_temperature{device_type="BLE",sensor_type="EMB",mac="00:00:00:00:00:00",room="Lucas' Bedroom",collector="f429"} 24.723000
 * device_temperature{device_type="BLE",sensor_type="EMB",mac="11:11:11:11:11:11",room="Kitchen",collector="f429"} -1.723e+01
 * 
 */

#include "prometheus_client.h"

#include "utils.h"

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
        /* device type : ble, can */
        METRIC_TAG("device_type"),

        /* sensor type : embedded, extern */
        METRIC_TAG("sensor_type"),

        /* mac address: BLE MAC address (if ble), DeviceID (if caniot device) */
        METRIC_TAG("mac"),

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

const struct metric_definition mdef_device_battery =
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
static ssize_t encode_metric(uint8_t *buf,
			     size_t len,
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

			ret = mem_append_strings(buf + appended, len - appended,
						 strings, ARRAY_SIZE(strings));
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

		ret = mem_append_strings(buf + appended, len - appended,
					 strings, ARRAY_SIZE(strings));
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
	ret = mem_append_strings(buf + appended, len - appended, strings, count);
	if (ret < 0) {
		return ret;
	}
	
	return appended + ret;
}

int prometheus_metrics(struct http_request *req,
		       struct http_response *resp)
{
	const char *tags1[] = {
		"BLE",
		"EMB",
		"00:00:00:00:00:00",
		"Lucas' Bedroom",
		"f429"
	};

	const char *tags2[] = {
		"BLE",
		"EMB",
		"11:11:11:11:11:11",
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

	resp->content_len += encode_metric(resp->buf, resp->buf_size, &val1,
					   &mdef_device_temperature, true);

	resp->content_len += encode_metric(resp->buf + resp->content_len,
					   resp->buf_size - resp->content_len,
					   &val2, &mdef_device_temperature,
					   false);

	resp->status_code = 200;

	return 0;
}