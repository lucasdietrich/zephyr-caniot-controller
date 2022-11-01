#include <zephyr/kernel.h>

#include <stdint.h>
#include <stdbool.h>

#include "caniot.h"

#define GARAGE_DOOR_CONTROLLER_DEV CANIOT_DID(CANIOT_DEVICE_CLASS0, 0x1)
#define ALARM_CONTROLLER_DID CANIOT_DID(CANIOT_DEVICE_CLASS0, 0x3)
#define DEV_BOARD_DID CANIOT_DID(CANIOT_DEVICE_CLASS0, 0x4)
#define HEATING_CONTROLLER_DID CANIOT_DID(CANIOT_DEVICE_CLASS1, 0x0)

/* TODO remove one of the duplicates (see devices/caniot.c) */
static int save_caniot_temperature(struct ha_ds_caniot_blc0 *blt,
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

/* feed a board level telemetry dataset from a received CAN buffer */
void ha_dev_caniot_blc_cls0_to_blt(struct ha_ds_caniot_blc0 *blt,
				   const struct caniot_blc0_telemetry *can_buf)
{
	save_caniot_temperature(blt, 0U, can_buf->int_temperature,
				HA_DEV_SENSOR_TYPE_EMBEDDED);
	save_caniot_temperature(blt, 1U, can_buf->ext_temperature,
				HA_DEV_SENSOR_TYPE_EXTERNAL1);
	save_caniot_temperature(blt, 2U, can_buf->ext_temperature2,
				HA_DEV_SENSOR_TYPE_EXTERNAL2);
	save_caniot_temperature(blt, 3U, can_buf->ext_temperature3,
				HA_DEV_SENSOR_TYPE_EXTERNAL3);

	blt->dio.value = can_buf->dio;
	blt->dio.mask = 0xFFU;

	blt->pdio.value = can_buf->pdio;
	blt->pdio.mask = 0xFFU;
}

static int blc0_ingest(struct ha_event *ev,
		       struct ha_dev_payload *pl)
{
	ha_dev_caniot_blc_cls0_to_blt(ev->data,
				      (const struct caniot_blc0_telemetry *)pl->buffer);

	return 0;
}

static int blc1_ingest(struct ha_event *ev,
		       struct ha_dev_payload *pl)
{
	return 0;
}

static int ep_heating_control_ingest(struct ha_event *ev,
				     struct ha_dev_payload *pl)
{
	return 0;
}

static int select_endpoint(const ha_dev_addr_t *addr,
			   const struct ha_dev_payload *pl)
{
	__ASSERT_NO_MSG(addr->type == HA_DEV_TYPE_CANIOT);

	caniot_id_t *const id = pl->y;

	if (id->endpoint == CANIOT_ENDPOINT_BOARD_CONTROL) {
		return HA_ENDPOINT_INDEX(0);
	} else if (id->endpoint == CANIOT_ENDPOINT_APP) {
		return HA_ENDPOINT_INDEX(1);
	}
	
	// ha_dev_get_endpoint_idx_by_id()

	return -ENOENT;
}

/* TODO reference endpoint instead of allocating two for EACH device */

static const struct ha_data_descr ha_ds_caniot_blc0_descr[] = {
	HA_DATA_DESCR(struct ha_ds_caniot_blc0, temperatures[0u], 
		HA_DATA_TEMPERATURE, HA_ASSIGN_BOARD_TEMPERATURE),
	HA_DATA_DESCR(struct ha_ds_caniot_blc0, temperatures[1u], 
		HA_DATA_TEMPERATURE, HA_ASSIGN_EXTERNAL_TEMPERATURE_SENSOR),
	HA_DATA_DESCR(struct ha_ds_caniot_blc0, temperatures[2u], 
		HA_DATA_TEMPERATURE, HA_ASSIGN_EXTERNAL_TEMPERATURE_SENSOR),
	HA_DATA_DESCR(struct ha_ds_caniot_blc0, temperatures[3u], 
		HA_DATA_TEMPERATURE, HA_ASSIGN_EXTERNAL_TEMPERATURE_SENSOR),
	HA_DATA_DESCR(struct ha_ds_caniot_blc0, dio, HA_DATA_DIGITAL_INOUT, HA_ASSIGN_DIGITAL_IO),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_blc0, pdio, HA_DATA_DIGITAL_INOUT),
};

static const struct ha_data_descr ha_cmd_caniot_blc0_descr[] = {
	HA_DATA_DESCR(struct ha_cmd_caniot_blc0, oc1, HA_DATA_XPS, HA_ASSIGN_OPEN_COLLECTOR),
	HA_DATA_DESCR(struct ha_cmd_caniot_blc0, oc2, HA_DATA_XPS, HA_ASSIGN_OPEN_COLLECTOR),
	HA_DATA_DESCR(struct ha_cmd_caniot_blc0, rl1, HA_DATA_XPS, HA_ASSIGN_RELAY),
	HA_DATA_DESCR(struct ha_cmd_caniot_blc0, rl2, HA_DATA_XPS, HA_ASSIGN_RELAY),
};

static const struct ha_device_endpoint_api ep_blc0 = {
	.eid = HA_DEV_ENDPOINT_CANIOT_BLC0,
	.data_size = sizeof(struct ha_ds_caniot_blc0),
	.expected_payload_size = 8u,
	.data_descr = ha_ds_caniot_blc0_descr,
	.data_descr_size = ARRAY_SIZE(ha_ds_caniot_blc0_descr),
	.cmd_descr = ha_cmd_caniot_blc0_descr,
	.cmd_descr_size = ARRAY_SIZE(ha_cmd_caniot_blc0_descr),
	.ingest = blc0_ingest,
	.command = NULL
};

static const struct ha_data_descr ha_ds_caniot_blc1_descr[] = {
	// ha_ds_caniot_blc1
};

static const struct ha_device_endpoint_api ep_blc1 = {
	.eid = HA_DEV_ENDPOINT_CANIOT_BLC1,
	.data_size = 0u,
	.expected_payload_size = 8u,
	.data_descr_size = ARRAY_SIZE(ha_ds_caniot_blc1_descr),
	.data_descr = ha_ds_caniot_blc1_descr,
	.ingest = blc1_ingest,
	.command = NULL
};

static const struct ha_data_descr ha_ds_caniot_ep_heating_control_descr[] = {
	HA_DATA_DESCR(struct ha_ds_caniot_heating_control, temperatures[0u], 
		HA_DATA_TEMPERATURE, HA_ASSIGN_BOARD_TEMPERATURE),
	HA_DATA_DESCR(struct ha_ds_caniot_heating_control, temperatures[1u], 
		HA_DATA_TEMPERATURE, HA_ASSIGN_EXTERNAL_TEMPERATURE_SENSOR),
	HA_DATA_DESCR(struct ha_ds_caniot_heating_control, temperatures[2u], 
		HA_DATA_TEMPERATURE, HA_ASSIGN_EXTERNAL_TEMPERATURE_SENSOR),
	HA_DATA_DESCR(struct ha_ds_caniot_heating_control, temperatures[3u], 
		HA_DATA_TEMPERATURE, HA_ASSIGN_EXTERNAL_TEMPERATURE_SENSOR),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, heaters[0u], 
		HA_DATA_HEATER_MODE),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, heaters[1u], 
		HA_DATA_HEATER_MODE),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, heaters[2u], 
		HA_DATA_HEATER_MODE),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, heaters[3u], 
		HA_DATA_HEATER_MODE),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, shutters[0u], 
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, shutters[1u], 
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, shutters[2u], 
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, shutters[3u], 
		HA_DATA_SHUTTER_POSITION),
};

static const struct ha_data_descr ha_cmd_caniot_ep_heating_control_descr[] = {
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_heating_control, heaters[0u], 
		HA_DATA_HEATER_MODE),
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_heating_control, heaters[1u], 
		HA_DATA_HEATER_MODE),
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_heating_control, heaters[2u], 
		HA_DATA_HEATER_MODE),
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_heating_control, heaters[3u], 
		HA_DATA_HEATER_MODE),
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_heating_control, shutters[0u], 
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_heating_control, shutters[1u], 
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_heating_control, shutters[2u], 
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_heating_control, shutters[3u], 
		HA_DATA_SHUTTER_POSITION),
};

static const struct ha_device_endpoint_api ep_heating_control = {
	.eid = HA_DEV_ENDPOINT_CANIOT_HEATING,
	.data_size = sizeof(struct ha_ds_caniot_heating_control),
	.expected_payload_size = 8u,
	.data_descr = ha_ds_caniot_ep_heating_control_descr,
	.data_descr_size = ARRAY_SIZE(ha_ds_caniot_ep_heating_control_descr),
	.cmd_descr = ha_cmd_caniot_ep_heating_control_descr,
	.cmd_descr_size = ARRAY_SIZE(ha_cmd_caniot_ep_heating_control_descr),
	.ingest = ep_heating_control_ingest,
	.command = NULL
};

static int init_endpoints(const ha_dev_addr_t *addr,
			  struct ha_device_endpoint *endpoints,
			  uint8_t *endpoints_count)
{
	if (CANIOT_DID_CLS(addr->mac.addr.caniot) == CANIOT_DEVICE_CLASS0) {
		endpoints[0].api = &ep_blc0;
		*endpoints_count = 1U;
	} else if (CANIOT_DID_CLS(addr->mac.addr.caniot) == CANIOT_DEVICE_CLASS1) {
		endpoints[0].api = &ep_blc1;
		*endpoints_count = 1U;
	} else {
		return -ENOTSUP;
	}

	if (caniot_deviceid_equal(addr->mac.addr.caniot, HEATING_CONTROLLER_DID)) {
		endpoints[1u].api = &ep_heating_control;
		*endpoints_count = 2u;
	}

	return 0;
}

const struct ha_device_api ha_device_api_caniot = {
	.init_endpoints = init_endpoints,
	.select_endpoint = select_endpoint,
};

int ha_dev_register_caniot_telemetry(uint32_t timestamp,
				     caniot_did_t did,
				     char buf[8u],
				     caniot_id_t *id)
{
	/* check if device already exists */
	const ha_dev_addr_t addr = {
		.type = HA_DEV_TYPE_CANIOT,
		.mac = {
			.medium = HA_DEV_MEDIUM_CAN,
			.addr.caniot = did
		}
	};

	return ha_dev_register_data(&addr, buf, 8u, timestamp, (void *)id);
}

const struct caniot_blc0_telemetry *ha_ev_get_caniot_telemetry(const ha_ev_t *ev)
{
	return HA_EV_GET_CAST_DATA(ev, struct caniot_blc0_telemetry);
}