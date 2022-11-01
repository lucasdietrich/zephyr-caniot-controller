/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>


controller_status_t controller_status = {
    .has_ipv4_addr = 0,
    .valid_system_time = 0,
};