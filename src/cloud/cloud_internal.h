/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CLOUD_CLOUD_INTERNAL_H_
#define _CLOUD_CLOUD_INTERNAL_H_

#include <zephyr.h>

struct cloud_platform {
	const char *name;

	void *data;
	void *config;
	
	struct mqtt_client *mqtt;
	
	/* Api */
	int (*init)(struct cloud_platform *p);
	int (*provision)(struct cloud_platform *p);
	int (*deinit)(struct cloud_platform *p);
};

#endif /* #define _CLOUD_CLOUD_INTERNAL_H_ */