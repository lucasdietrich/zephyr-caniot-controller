#include <stdint.h>
#include <stdbool.h>

#include "ha/devices.h"
#include "ble/xiaomi_record.h"
#include "ha/events.h"
#include "ha/data.h"

static bool on_registration(const ha_dev_addr_t *addr)
{
	/* Check if BLE address is valid */
	return true;
}

static size_t get_data_size(struct ha_device *dev, const void *idata)
{
	return sizeof(struct ha_xiaomi_dataset);
}

static void ble_record_to_xiaomi(struct ha_xiaomi_dataset *xiaomi,
				 const xiaomi_record_t *rec)
{
	xiaomi->rssi = rec->measurements.rssi;
	xiaomi->temperature.type = HA_DEV_SENSOR_TYPE_EMBEDDED;
	xiaomi->temperature.value = rec->measurements.temperature;
	xiaomi->humidity = rec->measurements.humidity;
	xiaomi->battery_mv = rec->measurements.battery_mv;
	xiaomi->battery_level = rec->measurements.battery_level;
}

static bool convert_data(struct ha_device *dev, const void *idata, void *odata)
{
	__ASSERT_NO_MSG(dev->addr.type == HA_DEV_TYPE_XIAOMI_MIJIA);

	ble_record_to_xiaomi(odata, idata);

	return true;
}

const struct device_api ha_device_api_xiaomi = {
	.on_registration = on_registration,
	.get_data_size = get_data_size,
	.convert_data = convert_data
};