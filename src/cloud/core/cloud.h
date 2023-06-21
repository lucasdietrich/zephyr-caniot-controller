/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CLOUD_CLOUD_H_
#define _CLOUD_CLOUD_H_

#include <zephyr/kernel.h>

struct cloud_platform *cloud_platform_get(void);

void cloud_publish(const char *topic, const char *data, size_t len);

void cloud_subscribe(const char *topic);

void cloud_unsubscribe(const char *topic);

void cloud_msg_callback_register(void (*callback)(const char *topic,
												  const char *data,
												  size_t len));

void cloud_set_process_cb(void (*callback)(void));

void cloud_notify(atomic_val_t flags);

#endif /* _CLOUD_CLOUD_H_ */