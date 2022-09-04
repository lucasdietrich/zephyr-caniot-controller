/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CLOUD_H_
#define _CLOUD_H_

#include <zephyr.h>

struct cloud_platform {
	const char *name;

	void *data;
	void *config;

	int (*init)(void);
	int (*connect)(void);
	int (*publish)(const char *topic, const char *data, size_t len, int qos);
	int (*subscribe)(const char *topic, int qos);
	int (*on_published)(const char *topic, const char *data, size_t len);
};

int cloud_init(void);

#endif /* _CLOUD_H_ */