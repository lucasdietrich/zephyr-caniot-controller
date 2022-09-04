/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CLOUD_UTILS_H_
#define _CLOUD_UTILS_H_

#include <zephyr.h>
#include <net/mqtt.h>

const char *mqtt_evt_get_str(enum mqtt_evt_type evt_type);

#endif /* _CLOUD_UTILS_H_ */