#include "core/cloud.h"
#include "core/mqttc.h"
#include "ha/core/ha.h"
#include "ha/core/subs_extended.h"
#include "ha/devices/all.h"
#include "ha/json.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/data/json.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mqtt.h>
LOG_MODULE_REGISTER(cloud_app, LOG_LEVEL_WRN);

static struct ha_ev_subs *sub = NULL;

const struct json_obj_descr json_cloud_xiaomi_record_measures_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(
		struct json_xiaomi_record_measures, "rssi", rssi, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_xiaomi_record_measures,
				  "temperature",
				  temperature,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_xiaomi_record_measures,
				  "humidity",
				  humidity,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_xiaomi_record_measures,
				  "battery_level",
				  battery_level,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_xiaomi_record_measures,
				  "battery_voltage",
				  battery_voltage,
				  JSON_TOK_NUMBER),
};

const struct json_obj_descr json_cloud_xiaomi_record_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(
		struct json_xiaomi_record, "bt_mac", bt_mac, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(
		struct json_xiaomi_record, "timestamp", base.timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct json_xiaomi_record,
				    "measures",
				    measures,
				    json_cloud_xiaomi_record_measures_descr),
};

void cloud_on_queued(struct ha_ev_subs *sub, ha_ev_t *event)
{
	ARG_UNUSED(sub);
	ARG_UNUSED(event);

	cloud_notify(0u);
}

static struct ha_subs_ext_lookup_table sub_lt;

int cloud_app_init(void)
{
	static struct ha_ev_subs_conf sub_conf = {
		.flags = HA_EV_SUBS_CONF_DEVICE_DATA | HA_EV_SUBS_CONF_ON_QUEUED_HOOK |
			 HA_EV_SUBS_CONF_DEVICE_TYPE,
		.device_type  = HA_DEV_TYPE_XIAOMI_MIJIA,
		.on_queued_cb = cloud_on_queued,
	};

	ha_subs_ext_conf_set(&sub_conf,
			     &sub_lt,
			     HA_SUBS_EXT_LOOKUP_TYPE_SDEVUID,
			     HA_SUBS_EXT_FILTERING_TYPE_INTERVAL,
			     HA_SUBS_EXT_FILTERING_PARAM_INTERVAL(
				     CONFIG_APP_CLOUD_TELEMETRY_INTERVAL));

	return ha_subscribe(&sub_conf, &sub);
}

int process_event(ha_ev_t *event)
{
	int ret = 0;

	switch (event->dev->addr.type) {
	case HA_DEV_TYPE_XIAOMI_MIJIA: {
		cursor_buffer_t *buf = mqttc_get_payload_buffer();

		struct json_xiaomi_record json_data;

		const struct ha_ds_xiaomi *const data = ha_ev_get_data(event);

		char temp_str[9u];
		char bt_mac_str[BT_ADDR_STR_LEN];

		sprintf(temp_str, "%.2f", data->temperature.value / 100.0);

		bt_addr_to_str(
			&event->dev->addr.mac.addr.ble.a, bt_mac_str, BT_ADDR_LE_STR_LEN);

		json_data.bt_mac	 = bt_mac_str;
		json_data.base.timestamp = event->timestamp;

		json_data.measures.rssi		   = data->rssi.value;
		json_data.measures.temperature	   = temp_str;
		json_data.measures.humidity	   = data->humidity.value;
		json_data.measures.battery_level   = data->battery_level.level;
		json_data.measures.battery_voltage = data->battery_level.voltage;

		json_obj_encode_buf(json_cloud_xiaomi_record_descr,
				    ARRAY_SIZE(json_cloud_xiaomi_record_descr),
				    &json_data,
				    buf->buffer,
				    buf->size);

		ret = mqttc_publish(CONFIG_APP_AWS_THING_NAME "/data",
				    buf->buffer,
				    strlen(buf->buffer),
				    MQTT_QOS_1_AT_LEAST_ONCE);
		if (ret < 0) {
			LOG_ERR("Failed to publish data: %d", ret);
		}

		break;
	}
	default:
		LOG_WRN("Unsupported event type: %d", event->dev->addr.type);
		break;
	}

	return ret;
}

int cloud_app_process(atomic_val_t flags)
{
	int ret = 0;
	ha_ev_t *ev;

	if ((ev = ha_ev_wait(sub, K_NO_WAIT)) != NULL) {
		LOG_INF("Processing event: %p", ev);

		ret = process_event(ev);
		ha_ev_unref(ev);

		/* Maybe more events to process ? */
		cloud_notify(0u);
	}

	return ret;
}

int cloud_app_cleanup(void)
{
	int ret = ha_unsubscribe(sub);
	ha_subs_ext_lt_clear(&sub_lt);

	sub = NULL;
	return ret;
}
