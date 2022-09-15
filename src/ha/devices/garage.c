/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "garage.h"

#include <caniot/caniot.h>
#include <caniot/datatype.h>

static const caniot_did_t garage_did = CANIOT_DID(CANIOT_DEVICE_CLASS0, CANIOT_DEVICE_SID4);

void ha_dev_garage_cmd_init(struct ha_dev_garage_cmd *cmd)
{
	if (cmd != NULL) {
		memset(cmd, 0, sizeof(struct ha_dev_garage_cmd));
	}
}

static void ha_dev_garage_payload_build(struct caniot_board_control_command *payload,
					const struct ha_dev_garage_cmd *cmd)
{
	__ASSERT_NO_MSG(payload != NULL);
	__ASSERT_NO_MSG(cmd != NULL);

	caniot_board_control_command_init(payload);

	if (cmd->actuate_left == 1U) {
		payload->crl1 = CANIOT_XPS_TOGGLE;
	}

	if (cmd->actuate_right == 1U) {
		payload->crl2 = CANIOT_XPS_TOGGLE;
	}
}

int ha_dev_garage_cmd_send(const struct ha_dev_garage_cmd *cmd)
{
	struct caniot_frame frame;
	struct caniot_board_control_command payload;

	ha_dev_garage_payload_build(&payload, cmd);

	caniot_build_query_command(&frame, CANIOT_ENDPOINT_BOARD_CONTROL,
				   (uint8_t *)&payload, sizeof(payload));

	return ha_ciot_ctrl_send(&frame, garage_did);
}