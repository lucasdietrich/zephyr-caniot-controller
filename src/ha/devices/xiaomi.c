#include "ha/core/ha.h"
#include "xiaomi.h"

#include <stdbool.h>
#include <stdint.h>

static void ble_record_to_xiaomi(struct ha_ds_xiaomi *xiaomi,
				 const xiaomi_record_t *rec,
				 uint32_t *timestamp)
{
	xiaomi->rssi.value	      = rec->measurements.rssi;
	xiaomi->temperature.type      = HA_DEV_SENSOR_TYPE_EMBEDDED;
	xiaomi->temperature.value     = rec->measurements.temperature;
	xiaomi->humidity.type	      = HA_DEV_SENSOR_TYPE_EMBEDDED;
	xiaomi->humidity.value	      = rec->measurements.humidity;
	xiaomi->battery_level.level   = rec->measurements.battery_level;
	xiaomi->battery_level.voltage = rec->measurements.battery_mv;

	*timestamp = rec->time;
}

static int ingest(struct ha_event *ev, const struct ha_device_payload *pl)
{

	ble_record_to_xiaomi(
		ev->data, (const xiaomi_record_t *)pl->buffer, &ev->timestamp);

	return 0;
}

static const struct ha_data_descr ha_ds_xiaomi_descr[] = {
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_xiaomi, rssi, HA_DATA_RSSI),
	HA_DATA_DESCR(struct ha_ds_xiaomi,
		      humidity,
		      HA_DATA_HUMIDITY,
		      HA_ASSIGN_BOARD_HUMIDITY),
	HA_DATA_DESCR(struct ha_ds_xiaomi,
		      temperature,
		      HA_DATA_TEMPERATURE,
		      HA_ASSIGN_BOARD_TEMPERATURE),
	HA_DATA_DESCR_UNASSIGNED(
		struct ha_ds_xiaomi, battery_level, HA_DATA_BATTERY_LEVEL),
};

static struct ha_device_endpoint_api ep = {
	.eid		       = HA_DEV_EP_XIAOMI_MIJIA,
	.data_size	       = sizeof(struct ha_ds_xiaomi),
	.expected_payload_size = sizeof(xiaomi_record_t),
	.flags		       = HA_DEV_EP_FLAG_DEFAULT,
	.data_descr_size       = ARRAY_SIZE(ha_ds_xiaomi_descr),
	.data_descr	       = ha_ds_xiaomi_descr,
	.ingest		       = ingest,
	.command	       = NULL,
};

static int init_endpoints(const ha_dev_addr_t *addr,
			  struct ha_device_endpoint *endpoints,
			  uint8_t *endpoints_count)
{
	endpoints[0].api = &ep;
	*endpoints_count = 1U;

	return 0;
}

const struct ha_device_api ha_device_api_xiaomi = {
	.init_endpoints = init_endpoints, .select_endpoint = HA_DEV_EP_SELECT_0_CB};

int ha_dev_xiaomi_register_record(const xiaomi_record_t *record)
{
	const ha_dev_addr_t addr = {
		.type = HA_DEV_TYPE_XIAOMI_MIJIA,
		.mac =
			{
				.medium	  = HA_DEV_MEDIUM_BLE,
				.addr.ble = record->addr,
			},
	};

	const struct ha_device_payload pl = {
		.buffer	   = (const char *)record,
		.len	   = sizeof(xiaomi_record_t),
		.timestamp = record->time,
		.y	   = NULL,
	};

	return ha_dev_register_data(&addr, &pl);
}

void ha_dev_xiaomi_record_init(xiaomi_record_t *record)
{
	record->measurements.battery_level = 0xFF;
	record->measurements.battery_mv	   = 0xFFFF;
	record->measurements.humidity	   = 0xFFFF;
	record->measurements.temperature   = 0xFFFF;
	record->measurements.rssi	   = 0xFF;
	record->time			   = 0u;

	record->valid = false;
}

const struct ha_ds_xiaomi *ha_ev_get_xiaomi_data(const ha_ev_t *ev)
{
	return HA_EV_GET_CAST_DATA(ev, const struct ha_ds_xiaomi);
}