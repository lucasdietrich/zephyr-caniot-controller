/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>

#include "data.h"

void ha_data_ble_to_xiaomi(struct ha_xiaomi_dataset *xiaomi,
			   const xiaomi_record_t *rec)
{
	xiaomi->rssi = rec->measurements.rssi;
	xiaomi->temperature.type = HA_DEV_SENSOR_TYPE_EMBEDDED;
	xiaomi->temperature.value = rec->measurements.temperature;
	xiaomi->humidity = rec->measurements.humidity;
	xiaomi->battery_mv = rec->measurements.battery_mv;
	xiaomi->battery_level = rec->measurements.battery_level;
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

/* feed a board level telemetry dataset from a received CAN buffer */
void ha_data_can_to_blt(struct ha_caniot_blt_dataset *blt,
			const struct caniot_board_control_telemetry *can_buf)
{
	save_caniot_temperature(blt, 0U, can_buf->int_temperature,
				HA_DEV_SENSOR_TYPE_EMBEDDED);
	save_caniot_temperature(blt, 1U, can_buf->ext_temperature,
				HA_DEV_SENSOR_TYPE_EXTERNAL1);
	save_caniot_temperature(blt, 2U, can_buf->ext_temperature2,
				HA_DEV_SENSOR_TYPE_EXTERNAL2);
	save_caniot_temperature(blt, 3U, can_buf->ext_temperature3,
				HA_DEV_SENSOR_TYPE_EXTERNAL3);

	blt->dio = can_buf->dio;
	blt->pdio = can_buf->pdio;
}