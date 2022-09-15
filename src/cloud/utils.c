/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"

#include <net/socket.h>
#include <net/net_core.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(cloud_utils, LOG_LEVEL_DBG);

const char *mqtt_evt_get_str(enum mqtt_evt_type evt_type)
{
	static const char *mqtt_evt_str[] = {
		"MQTT_EVT_CONNACK",
		"MQTT_EVT_DISCONNECT",
		"MQTT_EVT_PUBLISH",
		"MQTT_EVT_PUBACK",
		"MQTT_EVT_PUBREC",
		"MQTT_EVT_PUBREL",
		"MQTT_EVT_PUBCOMP",
		"MQTT_EVT_SUBACK",
		"MQTT_EVT_UNSUBACK",
		"MQTT_EVT_PINGRESP",
		"<UNKNOWN mqtt_evt_type>"
	};

	return mqtt_evt_str[MIN(evt_type, ARRAY_SIZE(mqtt_evt_str) - 1)];
}

int resolve_hostname(struct sockaddr_in *addr,
		     const char *hostname,
		     uint16_t port)
{
	int ret = 0U;
	struct zsock_addrinfo *ai = NULL;

	const struct zsock_addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0
	};
	ret = zsock_getaddrinfo(hostname, NULL, &hints, &ai);
	if (ret != 0) {
		LOG_ERR("failed to resolve hostname err = %d (errno = %d)",
			ret, errno);
	} else {
		memcpy(addr, ai->ai_addr,
		       MIN(ai->ai_addrlen,
			   sizeof(struct sockaddr_storage)));

		addr->sin_port = htons(port);

		char addr_str[INET_ADDRSTRLEN];
		zsock_inet_ntop(AF_INET,
				&addr->sin_addr,
				addr_str,
				sizeof(addr_str));
		LOG_INF("Resolved %s -> %s", log_strdup(hostname), log_strdup(addr_str));
	}

	zsock_freeaddrinfo(ai);

	return ret;
}