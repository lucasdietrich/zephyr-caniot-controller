#include <stdint.h>
#include <stdbool.h>

#include "xiaomi.h"

#include "ha/devices.h"
#include "ble/xiaomi_record.h"

static void ble_record_to_xiaomi(struct ha_ds_xiaomi *xiaomi,
				 const xiaomi_record_t *rec,
				 uint32_t *timestamp)
{
	xiaomi->rssi = rec->measurements.rssi;
	xiaomi->temperature.type = HA_DEV_SENSOR_TYPE_EMBEDDED;
	xiaomi->temperature.value = rec->measurements.temperature;
	xiaomi->humidity = rec->measurements.humidity;
	xiaomi->battery_mv = rec->measurements.battery_mv;
	xiaomi->battery_level = rec->measurements.battery_level;

	*timestamp = rec->time;
}

static int ingest(struct ha_event *ev,
	   struct ha_dev_payload *pl)
{

	ble_record_to_xiaomi(ev->data,
			     (const xiaomi_record_t *)pl->buffer,
			     &ev->timestamp);

	return 0;
}

static struct ha_device_endpoint_api ep = HA_DEV_ENDPOINT_API_INIT(
	HA_DEV_ENDPOINT_XIAOMI_MIJIA,
	sizeof(struct ha_ds_xiaomi),
	sizeof(xiaomi_record_t),
	ingest,
	NULL
);

static int init_endpoints(const ha_dev_addr_t *addr,
			  struct ha_device_endpoint *endpoints,
			  uint8_t *endpoints_count)
{
	endpoints[0].api = &ep;
	*endpoints_count = 1U;

	return 0;
}

const struct ha_device_api ha_device_api_xiaomi = {
	.init_endpoints = init_endpoints,
	.select_endpoint = HA_DEV_API_SELECT_ENDPOINT_0_CB
};

int ha_dev_register_xiaomi_record(const xiaomi_record_t *record)
{
	const ha_dev_addr_t addr = {
		.type = HA_DEV_TYPE_XIAOMI_MIJIA,
		.mac = {
			.medium = HA_DEV_MEDIUM_BLE,
			.addr.ble = record->addr
		}
	};

	return ha_dev_register_data(&addr, (void *)record,
				    sizeof(xiaomi_record_t), record->time, NULL);
}

const struct ha_ds_xiaomi *ha_ev_get_xiaomi_data(const ha_ev_t *ev)
{
	return HA_EV_GET_CAST_DATA(ev, const struct ha_ds_xiaomi);
}