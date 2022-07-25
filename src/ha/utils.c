/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"


int zcan_to_caniot(const struct zcan_frame *zcan,
		   struct caniot_frame *caniot)
{
	if ((zcan == NULL) || (caniot == NULL)) {
		return -EINVAL;
	}

	caniot_clear_frame(caniot);
	caniot->id = caniot_canid_to_id((uint16_t)zcan->id);
	caniot->len = MIN(zcan->dlc, 8U);
	memcpy(caniot->buf, zcan->data, caniot->len);

	return 0U;
}

// static
int caniot_to_zcan(struct zcan_frame *zcan,
		   const struct caniot_frame *caniot)
{
	if ((zcan == NULL) || (caniot == NULL)) {
		return -EINVAL;
	}

	memset(zcan, 0x00U, sizeof(struct zcan_frame));

	zcan->id = caniot_id_to_canid(caniot->id);
	zcan->dlc = MIN(caniot->len, 8U);
	memcpy(zcan->data, caniot->buf, zcan->dlc);

	return 0U;
}

static int string_get_index_in_list(const char *str, const char *const *list)
{
	int ret = -1;

	if (str != NULL) {
		for (size_t i = 0; list[i] != NULL; i++) {
			if (strcmp(str, list[i]) == 0) {
				ret = i;
				break;
			}
		}
	}

	return ret;
}

int ha_parse_ss_command(const char *str)
{
	static const char *const cmds[] = {
		"none",
		"set",
		NULL
	};
	return MAX(0, string_get_index_in_list(str, cmds));
}

int ha_parse_xps_command(const char *str)
{
	static const char *const cmds[] = {
		"none",
		"set_on",
		"set_off",
		"toggle",
		"reset",
		"pulse_on",
		"pulse_off",
		"pulse_cancel",
		NULL
	};
	return MAX(0, string_get_index_in_list(str, cmds));
}