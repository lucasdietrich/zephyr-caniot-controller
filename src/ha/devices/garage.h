/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_DEVICES_GARAGE_H
#define _HA_DEVICES_GARAGE_H

#include "../ha.h"
#include "../devices.h"

/* move to specific header */
struct ha_dev_garage_cmd
{
	uint8_t actuate_left: 1;
	uint8_t actuate_right: 1;
};

void ha_dev_garage_cmd_init(struct ha_dev_garage_cmd *cmd);

int ha_dev_garage_cmd_send(const struct ha_dev_garage_cmd *cmd);

#endif /* _HA_DEVICES_GARAGE_H */