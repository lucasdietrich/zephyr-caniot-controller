#include <zephyr/kernel.h>

#include <stdint.h>
#include <stdbool.h>

#include "caniot.h"

#define GARAGE_DOOR_CONTROLLER_DEV 	CANIOT_DID(CANIOT_DEVICE_CLASS0, 0x1u)
#define ALARM_CONTROLLER_DID 		CANIOT_DID(CANIOT_DEVICE_CLASS0, 0x3u)
#define DEV_BOARD_DID 			CANIOT_DID(CANIOT_DEVICE_CLASS0, 0x4u)

#define HEATING_CONTROLLER_DID 		CANIOT_DID(CANIOT_DEVICE_CLASS1, 0x0u)
#define SHUTTERS_CONTROLLER_DID 	CANIOT_DID(CANIOT_DEVICE_CLASS1, 0x2u)

/* TODO remove one of the duplicates (see devices/caniot.c) */
static int save_caniot_temperature(struct ha_data_temperature *temp_buf_array,
				   size_t array_size,
				   uint8_t temp_index,
				   uint16_t temperature,
				   ha_dev_sensor_type_t sens_type)
{
	int ret = -EINVAL;

	if (temp_index < array_size) {
		if (CANIOT_DT_VALID_T10_TEMP(temperature)) {
			temp_buf_array[temp_index].type = sens_type;
			temp_buf_array[temp_index].value =
				caniot_dt_T10_to_T16(temperature);
			ret = 1U;
		} else {
			temp_buf_array[temp_index].type =
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
	save_caniot_temperature(blt->temperatures, ARRAY_SIZE(blt->temperatures),
				0U, can_buf->int_temperature,
				HA_DEV_SENSOR_TYPE_EMBEDDED);
	save_caniot_temperature(blt->temperatures, ARRAY_SIZE(blt->temperatures),
				1U, can_buf->ext_temperature,
				HA_DEV_SENSOR_TYPE_EXTERNAL1);
	save_caniot_temperature(blt->temperatures, ARRAY_SIZE(blt->temperatures),
				2U, can_buf->ext_temperature2,
				HA_DEV_SENSOR_TYPE_EXTERNAL2);
	save_caniot_temperature(blt->temperatures, ARRAY_SIZE(blt->temperatures),
				3U, can_buf->ext_temperature3,
				HA_DEV_SENSOR_TYPE_EXTERNAL3);

	blt->dio.value = can_buf->dio;
	blt->dio.mask = 0xFFU;

	blt->pdio.value = can_buf->pdio;
	blt->pdio.mask = 0xFFU;
}

static int blc0_ingest(struct ha_event *ev,
		       const struct ha_device_payload *pl)
{
	ha_dev_caniot_blc_cls0_to_blt(ev->data,
				      (const struct caniot_blc0_telemetry *)pl->buffer);

	return 0;
}

void ha_dev_caniot_blc_cls1_to_blt(struct ha_ds_caniot_blc1 *blt,
				   const struct caniot_blc1_telemetry *can_buf)
{
	save_caniot_temperature(blt->temperatures, ARRAY_SIZE(blt->temperatures),
				0U, can_buf->int_temperature,
				HA_DEV_SENSOR_TYPE_EMBEDDED);
	save_caniot_temperature(blt->temperatures, ARRAY_SIZE(blt->temperatures),
				1U, can_buf->ext_temperature,
				HA_DEV_SENSOR_TYPE_EXTERNAL1);
	save_caniot_temperature(blt->temperatures, ARRAY_SIZE(blt->temperatures),
				2U, can_buf->ext_temperature2,
				HA_DEV_SENSOR_TYPE_EXTERNAL2);
	save_caniot_temperature(blt->temperatures, ARRAY_SIZE(blt->temperatures),
				3U, can_buf->ext_temperature3,
				HA_DEV_SENSOR_TYPE_EXTERNAL3);

	blt->dio.value = can_buf->pcpd |
		(can_buf->eio << 8) |
		(can_buf->pb0 << 16) |
		(can_buf->pe0 << 17) |
		(can_buf->pe1 << 18);
	blt->dio.mask = 0x7FFFFU;
}

static int blc1_ingest(struct ha_event *ev,
		       const struct ha_device_payload *pl)
{
	ha_dev_caniot_blc_cls1_to_blt(ev->data,
				      (const struct caniot_blc1_telemetry *)pl->buffer);
	return 0;
}

static int ep_heating_control_ingest(struct ha_event *ev,
				     const struct ha_device_payload *pl)
{
	struct caniot_heating_control *can_buf =
		(struct caniot_heating_control *)pl->buffer;
	struct ha_ds_caniot_heating_control *ds = ev->data;

	ds->heaters[0u].mode = can_buf->heater1_cmd;
	ds->heaters[1u].mode = can_buf->heater2_cmd;
	ds->heaters[2u].mode = can_buf->heater3_cmd;
	ds->heaters[3u].mode = can_buf->heater4_cmd;

	return 0;
}

static int ep_shutters_control_ingest(struct ha_event *ev,
				      const struct ha_device_payload *pl)
{
	struct caniot_shutters_control *can_buf =
		(struct caniot_shutters_control *)pl->buffer;
	struct ha_ds_caniot_shutters_control *ds = ev->data;

	ds->shutters[0u].position = can_buf->shutters_openness[0u];
	ds->shutters[1u].position = can_buf->shutters_openness[1u];
	ds->shutters[2u].position = can_buf->shutters_openness[2u];
	ds->shutters[3u].position = can_buf->shutters_openness[3u];

	return 0;
}

static int select_endpoint(const ha_dev_addr_t *addr,
			   const struct ha_device_payload *pl)
{
	__ASSERT_NO_MSG(addr->type == HA_DEV_TYPE_CANIOT);

	caniot_id_t *const id = pl->y;

	switch (id->endpoint) {
	case CANIOT_ENDPOINT_BOARD_CONTROL:
		return HA_DEV_ENDPOINT_INDEX(0);
	case CANIOT_ENDPOINT_APP:
		return HA_DEV_ENDPOINT_INDEX(1);
	default:
		return -ENOENT;
	}
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
	HA_DATA_DESCR(struct ha_ds_caniot_blc1, temperatures[0u],
		HA_DATA_TEMPERATURE, HA_ASSIGN_BOARD_TEMPERATURE),
	HA_DATA_DESCR(struct ha_ds_caniot_blc1, temperatures[1u],
		HA_DATA_TEMPERATURE, HA_ASSIGN_EXTERNAL_TEMPERATURE_SENSOR),
	HA_DATA_DESCR(struct ha_ds_caniot_blc1, temperatures[2u],
		HA_DATA_TEMPERATURE, HA_ASSIGN_EXTERNAL_TEMPERATURE_SENSOR),
	HA_DATA_DESCR(struct ha_ds_caniot_blc1, temperatures[3u],
		HA_DATA_TEMPERATURE, HA_ASSIGN_EXTERNAL_TEMPERATURE_SENSOR),
	HA_DATA_DESCR(struct ha_ds_caniot_blc1, dio, HA_DATA_DIGITAL_INOUT, HA_ASSIGN_DIGITAL_IO),
};

static const struct ha_device_endpoint_api ep_blc1 = {
	.eid = HA_DEV_ENDPOINT_CANIOT_BLC1,
	.data_size = sizeof(struct ha_ds_caniot_blc1),
	.expected_payload_size = 8u,
	.data_descr_size = ARRAY_SIZE(ha_ds_caniot_blc1_descr),
	.data_descr = ha_ds_caniot_blc1_descr,
	.cmd_descr = NULL,
	.cmd_descr_size = 0u,
	.ingest = blc1_ingest,
	.command = NULL
};

static const struct ha_data_descr ha_ds_caniot_ep_heating_control_descr[] = {
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, heaters[0u],
		HA_DATA_HEATER_MODE),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, heaters[1u],
		HA_DATA_HEATER_MODE),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, heaters[2u],
		HA_DATA_HEATER_MODE),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_heating_control, heaters[3u],
		HA_DATA_HEATER_MODE),
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

static const struct ha_data_descr ha_ds_caniot_ep_shutters_control_descr[] = {
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_shutters_control, shutters[0u],
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_shutters_control, shutters[1u],
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_shutters_control, shutters[2u],
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_ds_caniot_shutters_control, shutters[3u],
		HA_DATA_SHUTTER_POSITION),
};

static const struct ha_data_descr ha_cmd_caniot_ep_shutters_control_descr[] = {
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_shutters_control, shutters[0u],
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_shutters_control, shutters[1u],
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_shutters_control, shutters[2u],
		HA_DATA_SHUTTER_POSITION),
	HA_DATA_DESCR_UNASSIGNED(struct ha_cmd_caniot_shutters_control, shutters[3u],
		HA_DATA_SHUTTER_POSITION),
};

static const struct ha_device_endpoint_api ep_shutters_control = {
	.eid = HA_DEV_ENDPOINT_CANIOT_SHUTTERS,
	.data_size = sizeof(struct ha_ds_caniot_shutters_control),
	.expected_payload_size = 8u,
	.data_descr = ha_ds_caniot_ep_shutters_control_descr,
	.data_descr_size = ARRAY_SIZE(ha_ds_caniot_ep_shutters_control_descr),
	.cmd_descr = ha_cmd_caniot_ep_shutters_control_descr,
	.cmd_descr_size = ARRAY_SIZE(ha_cmd_caniot_ep_shutters_control_descr),
	.ingest = ep_shutters_control_ingest,
	.command = NULL
};

static int init_endpoints(const ha_dev_addr_t *addr,
			  struct ha_device_endpoint *endpoints,
			  uint8_t *endpoints_count)
{
	/* Get class endpoint */
	switch (CANIOT_DID_CLS(addr->mac.addr.caniot)) {
	case CANIOT_DEVICE_CLASS0:
		endpoints[0u].api = &ep_blc0;
		*endpoints_count = 1U;
		break;
	case CANIOT_DEVICE_CLASS1:
		endpoints[0u].api = &ep_blc1;
		*endpoints_count = 1U;
		break;
	default:
		return -ENOTSUP;
	}

	/* Get device application endpoint */
	switch (addr->mac.addr.caniot) {
	case HEATING_CONTROLLER_DID:
		endpoints[1u].api = &ep_heating_control;
		*endpoints_count = 2u;
		break;
	case SHUTTERS_CONTROLLER_DID:
		endpoints[1u].api = &ep_shutters_control;
		*endpoints_count = 2u;
		break;
	default:
		break;
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

	const struct ha_device_payload pl =
	{
		.buffer = buf,
		.len = 8u,
		.timestamp = timestamp,
		.y = (void *)id
	};

	return ha_dev_register_data(&addr, &pl);
}

const struct caniot_blc0_telemetry *ha_ev_get_caniot_telemetry(const ha_ev_t *ev)
{
	return HA_EV_GET_CAST_DATA(ev, struct caniot_blc0_telemetry);
}