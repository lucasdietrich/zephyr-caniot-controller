#include <stdint.h>
#include <stdbool.h>

#include "ha/devices.h"
#include "ble/xiaomi_record.h"
#include "ha/data.h"

static bool on_registration(const ha_dev_addr_t *addr)
{
	/* Check if BLE address is valid */
	return true;
}

static size_t get_internal_format_size(struct ha_device *dev, 
				       const void *idata,
				       size_t data_len)
{
	return sizeof(struct ha_f429zi_dataset);
}

static void f429zi_temp_convert(struct ha_f429zi_dataset *dt,
				const float *die_temp)
{
	dt->die_temperature = *die_temp;
}

static bool convert_data(struct ha_device *dev,
			 const void *idata,
			 size_t ilen,
			 void *odata,
			 size_t olen,
			 uint32_t *timestamp)
{
	__ASSERT_NO_MSG(dev->addr.type == HA_DEV_TYPE_NUCLEO_F429ZI);

	ARG_UNUSED(timestamp);

	f429zi_temp_convert(odata, idata);

	return true;
}

const struct ha_device_api ha_device_api_f429zi = {
	.on_registration = on_registration,
	.get_internal_format_size = get_internal_format_size,
	.convert_data = convert_data
};

int ha_dev_register_die_temperature(uint32_t timestamp, float die_temperature)
{
	const ha_dev_addr_t addr = {
		.type = HA_DEV_TYPE_NUCLEO_F429ZI,
		.mac = {
			.medium = HA_DEV_MEDIUM_NONE,
		}
	};

	return ha_dev_register_data(&addr, &die_temperature,
				    sizeof(die_temperature), timestamp);
}

float ha_ev_get_die_temperature(const ha_ev_t *ev)
{
	const struct ha_f429zi_dataset *dt = 
		HA_EV_GET_CAST_DATA(ev, struct ha_f429zi_dataset);

	return dt->die_temperature;
}