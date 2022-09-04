/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CLOUD_INTERNAL_H_
#define _CLOUD_INTERNAL_H_

#include <zephyr.h>

#define CLOUD_MQTT_PORT 8883

char *cloud_get_mqtt_buf(void);

#endif /* _CLOUD_H_ */