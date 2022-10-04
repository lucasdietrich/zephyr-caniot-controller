#include <zephyr.h>

#include <stdint.h>
#include <stdbool.h>

#include "caniot.h"

/* TODO remove one of the duplicates (see devices/caniot.c) */
static int save_caniot_temperature(struct ha_ds_caniot_blc0_telemetry *blt,
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
void ha_dev_caniot_blc_cls0_to_blt(struct ha_ds_caniot_blc0_telemetry *blt,
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

static int blc_ingest(struct ha_event *ev,
		      struct ha_dev_payload *pl)
{
	if (CANIOT_DID_CLS(ev->dev->addr.mac.addr.caniot) == CANIOT_DEVICE_CLASS0) {
		ha_dev_caniot_blc_cls0_to_blt(
			ev->data,
			(const struct caniot_blc0_telemetry *)pl->buffer);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int select_endpoint(const ha_dev_addr_t *addr,
			   const struct ha_dev_payload *pl)
{
	__ASSERT_NO_MSG(addr->type == HA_DEV_TYPE_CANIOT);

	caniot_id_t *const id = pl->y;

	if (id->endpoint == CANIOT_ENDPOINT_BOARD_CONTROL) {
		return HA_ENDPOINT_INDEX(0);
	}

	return -ENOTSUP;
}

/* TODO reference endpoint instead of allocating two for EACH device */

static struct ha_device_endpoint ep_blc = HA_DEV_ENDPOINT_INIT(
	HA_DEV_ENDPOINT_CANIOT_BLC,
	sizeof(struct ha_ds_caniot_blc0_telemetry),
	sizeof(struct caniot_blc0_telemetry),
	blc_ingest,
	NULL
);

static int init_endpoints(const ha_dev_addr_t *addr,
			  struct ha_device_endpoint **endpoints,
			  uint8_t *endpoints_count)
{
	endpoints[0] = &ep_blc;
	*endpoints_count = 1U;

	return 0;
}

const struct ha_device_api ha_device_api_caniot = {
	.init_endpoints = init_endpoints,
	.select_endpoint = select_endpoint,
};

int ha_dev_register_caniot_telemetry(uint32_t timestamp,
				     caniot_did_t did,
				     const struct caniot_blc0_telemetry *data,
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

	return ha_dev_register_data(&addr,
				    data,
				    CANIOT_BLT_SIZE,
				    timestamp,
				    (void *)id);
}

const struct caniot_blc0_telemetry *ha_ev_get_caniot_telemetry(const ha_ev_t *ev)
{
	return HA_EV_GET_CAST_DATA(ev, struct caniot_blc0_telemetry);
}