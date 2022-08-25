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
	return sizeof(struct ha_caniot_blt_dataset);
}

static bool convert_data(struct ha_device *dev,
			 const void *idata,
			 size_t ilen,
			 void *odata,
			 size_t olen,
			 uint32_t *timestamp)
{
	__ASSERT_NO_MSG(dev->addr.type == HA_DEV_TYPE_CANIOT);

	ARG_UNUSED(timestamp);

	ha_data_can_to_blt(odata, idata);

	return true;
}

const struct ha_device_api ha_device_api_caniot = {
	.on_registration = on_registration,
	.get_internal_format_size = get_internal_format_size,
	.convert_data = convert_data
};

int ha_dev_register_caniot_telemetry(uint32_t timestamp,
				     caniot_did_t did,
				     const struct caniot_board_control_telemetry *data)
{
	/* check if device already exists */
	const ha_dev_addr_t addr = {
		.type = HA_DEV_TYPE_CANIOT,
		.mac = {
			.medium = HA_DEV_MEDIUM_CAN,
			.addr.caniot = did
		}
	};

	return ha_dev_register_data(&addr,
				    data,
				    CANIOT_BLT_SIZE,
				    timestamp);
}

const struct caniot_board_control_telemetry *ha_ev_get_caniot_telemetry(const ha_ev_t *ev)
{
	return HA_EV_GET_CAST_DATA(ev, struct caniot_board_control_telemetry);
}