/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CLOUD_CLOUD_INTERNAL_H_
#define _CLOUD_CLOUD_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/tls_credentials.h>

#define CONFIG_APP_CLOUD_ENDPOINT_MAX_LEN  64u
#define CONFIG_APP_CLOUD_CLIENT_ID_MAX_LEN 32u
#define CONFIG_APP_CLOUD_TOPIC_MAX_LEN	   128u

struct cloud_platform_config {
	char *endpoint;
	char *clientid;
	uint16_t port;

	const char *user;
	const char *password;

	sec_tag_t *sec_tag_list;
	uint32_t sec_tag_count;
};

struct cloud_platform {
	const char *name;

	struct cloud_platform_config config;

	struct sockaddr_in broker;

	struct mqtt_client *mqtt;

	/* Api */
	int (*init)(struct cloud_platform_config *p);
	int (*provision)(struct cloud_platform_config *p);
	int (*deinit)(struct cloud_platform_config *p);
};

#endif /* #define _CLOUD_CLOUD_INTERNAL_H_ */