/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_UTILS_H
#define _HA_UTILS_H

#include <stdint.h>

#include <zephyr/drivers/can.h>

#if defined(CONFIG_CANIOT_LIB)
#include <caniot/caniot.h>
#endif 

#include "ha.h"

const char *ha_dev_medium_to_str(ha_dev_medium_type_t medium);

const char *ha_dev_type_to_str(ha_dev_type_t type);

int zcan_to_caniot(const struct can_frame *zcan,
		   struct caniot_frame *caniot);


int caniot_to_zcan(struct can_frame *zcan,
		   const struct caniot_frame *caniot);

int ha_parse_ss_command(const char *str);

int ha_parse_xps_command(const char *str);

#endif /* _HA_UTILS_H */