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

static int save_caniot_temperature(struct ha_caniot_blt_dataset *blt,
				   uint8_t temp_index,
				   uint16_t temperature,
				   ha_dev_sensor_type_t sens_type)
{
	int ret = -EINVAL;

	if (temp_index < ARRAY_SIZE(blt->temperatures)) {
		if (CANIOT_DT_VALID_T10_TEMP(temperature)) {
			blt->temperatures[temp_index].type = sens_type;
			blt->temperatures[temp_index].value =
				caniot_dt_T10_to_T16(temperature);
			ret = 1U;
		} else {
			blt->temperatures[temp_index].type =
				HA_DEV_SENSOR_TYPE_NONE;
			ret = 0U;
		}
		ret = 0;
	}

	return ret;
}

static void convert(const struct caniot_board_control_telemetry *in,
		    struct ha_caniot_blt_dataset *out)
{
	save_caniot_temperature(out, 0U, in->int_temperature,
				HA_DEV_SENSOR_TYPE_EMBEDDED);
	save_caniot_temperature(out, 1U, in->ext_temperature,
				HA_DEV_SENSOR_TYPE_EXTERNAL1);
	save_caniot_temperature(out, 2U, in->ext_temperature2,
				HA_DEV_SENSOR_TYPE_EXTERNAL2);
	save_caniot_temperature(out, 3U, in->ext_temperature3,
				HA_DEV_SENSOR_TYPE_EXTERNAL3);

	out->dio = in->dio;
	out->pdio = in->pdio;
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

	convert(idata, odata);

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