#include <stdint.h>
#include <stdbool.h>

#include "xiaomi.h"

#include "ha/devices.h"
#include "ble/xiaomi_record.h"

static bool on_registration(const ha_dev_addr_t *addr)
{
	/* Check if BLE address is valid */
	return true;
}

static size_t get_internal_format_size(struct ha_device *dev, 
				       const void *idata,
				       size_t data_len)
{
	return sizeof(struct ha_xiaomi_dataset);
}

static void ble_record_to_xiaomi(struct ha_xiaomi_dataset *xiaomi,
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

static bool convert_data(struct ha_device *dev,
			 const void *idata,
			 size_t ilen,
			 void *odata,
			 size_t olen,
			 uint32_t *timestamp)
{
	__ASSERT_NO_MSG(dev->addr.type == HA_DEV_TYPE_XIAOMI_MIJIA);

	ble_record_to_xiaomi(odata, idata, timestamp);

	return true;
}

const struct ha_device_api ha_device_api_xiaomi = {
	.on_registration = on_registration,
	.get_internal_format_size = get_internal_format_size,
	.convert_data = convert_data
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
				    sizeof(xiaomi_record_t), record->time);
}

const struct ha_xiaomi_dataset *ha_ev_get_xiaomi_data(const ha_ev_t *ev)
{	
	return HA_EV_GET_CAST_DATA(ev, const struct ha_xiaomi_dataset);
}