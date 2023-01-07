/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cantcp.h"
#include "cantcp_core.h"
#include "utils.h"

#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

#include <fcntl.h>
LOG_MODULE_REGISTER(cantcp_client, LOG_LEVEL_DBG);

void cantcp_client_tunnel_init(cantcp_tunnel_t *tunnel)
{
	cantcp_core_tunnel_init(tunnel);

	tunnel->flags.mode = CANTCP_CLIENT;
}

static bool host_is_resolved(cantcp_tunnel_t *tunnel)
{
	return (tunnel->server.addr4.sin_port != 0U) &&
	       (tunnel->server.addr4.sin_addr.s_addr != INADDR_ANY) &&
	       (tunnel->server.addr4.sin_family != AF_UNSPEC);
}

static bool host_is_resolvable(cantcp_tunnel_t *tunnel)
{
	return (tunnel->server.hostname != NULL) && (tunnel->server.hostname[0] != '\0');
}

static int host_resolve(cantcp_tunnel_t *tunnel)
{
	int ret			  = 0U;
	struct zsock_addrinfo *ai = NULL;

	if (host_is_resolved(tunnel) == true) {
		goto exit;
	}

	if (host_is_resolvable(tunnel) == false) {
		ret = -EINVAL;
		goto exit;
	}

	const struct zsock_addrinfo hints = {.ai_family = AF_INET};
	ret = zsock_getaddrinfo(tunnel->server.hostname, NULL, &hints, &ai);
	if (ret != 0) {
		LOG_ERR("(%x) failed to resolve hostname (%s) err = %d",
			(uint32_t)tunnel,
			tunnel->server.hostname,
			ret);
		goto exit;
	}

	tunnel->server.addr4.sin_addr	= ((struct sockaddr_in *)ai->ai_addr)->sin_addr;
	tunnel->server.addr4.sin_family = AF_INET;

exit:
	zsock_freeaddrinfo(ai);

	return ret;
}

int cantcp_connect(cantcp_tunnel_t *tunnel)
{
	int ret = -EINVAL;

	/* checks */
	if (tunnel->flags.mode != CANTCP_CLIENT) {
		LOG_ERR("(%x) Tunnel is not in client mode", (uint32_t)tunnel);
		goto exit;
	}

	if (cantcp_connected(tunnel)) {
		LOG_ERR("(%x) Tunnel is already connected", (uint32_t)tunnel);
		goto exit;
	}

	if (tunnel->flags.secure != CANTCP_UNSECURE) {
		tunnel->flags.secure = CANTCP_SECURE;
		LOG_WRN("(%x) secure mode is not supported fow now, ignoring",
			(uint32_t)tunnel);
	}

	if (tunnel->flags.blocking_mode != CANTCP_BLOCKING) {
		tunnel->flags.blocking_mode = CANTCP_BLOCKING;
		LOG_WRN("(%x) non blocking mode is not supported fow now, "
			"ignoring",
			(uint32_t)tunnel);
	}

	/* resolve hostname  and prepare addr */
	ret = host_resolve(tunnel);
	if (ret != 0U) {
		LOG_ERR("(%x) Failed to resolve hostname", (uint32_t)tunnel);
		goto exit;
	}

	if (tunnel->server.port == 0U) {
		tunnel->server.port = CANTCP_DEFAULT_PORT;
		LOG_WRN("(%x) port is not set, using default port (%d)",
			(uint32_t)tunnel,
			CANTCP_DEFAULT_PORT);
	}

	tunnel->server.addr4.sin_port = htons(tunnel->server.port);

	/* create socket and connect */
	int sock =
		zsock_socket(tunnel->server.addr4.sin_family, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		LOG_ERR("(%x) Failed to create socket = %d", (uint32_t)tunnel, sock);
		ret = sock;
		goto exit;
	}

	/* set socket non-blocking */
	if (tunnel->flags.blocking_mode == CANTCP_BLOCKING_MODE) {
		ret = zsock_fcntl(sock, F_SETFL, O_NONBLOCK);
		if (ret < 0) {
			LOG_ERR("(%x) Failed to set socket non-blocking = %d",
				(uint32_t)tunnel,
				ret);
			goto exit;
		}
	}

	char ipv4_str[NET_IPV4_ADDR_LEN];
	ipv4_to_str(&tunnel->server.addr4.sin_addr, ipv4_str, sizeof(ipv4_str));

	ret = zsock_connect(sock, &tunnel->server.addr, sizeof(tunnel->server.addr));
	if (ret < 0) {
		LOG_ERR("(%x) Failed to connect to server %s:%d = %d",
			(uint32_t)tunnel,
			ipv4_str,
			tunnel->server.port,
			ret);

		/* close file descriptor */
		zsock_close(sock);

		goto exit;
	}

	LOG_INF("(%x) Tunnel opened with %s [%s:%d]",
		(uint32_t)tunnel,
		tunnel->server.hostname,
		ipv4_str,
		tunnel->server.port);

	/* set socket once connected */
	tunnel->sock = sock;

	ret = 0U;

exit:
	return ret;
}

int cantcp_disconnect(cantcp_tunnel_t *tunnel)
{
	if (cantcp_connected(tunnel)) {
		zsock_close(tunnel->sock);
		tunnel->sock = -1;

		LOG_INF("(%x) Tunnel closed", (uint32_t)tunnel);
	} else {
		LOG_WRN("(%x) Tunnel is not connected", (uint32_t)tunnel);
	}

	return 0U;
}

bool cantcp_connected(cantcp_tunnel_t *tunnel)
{
	return tunnel->sock >= 0;
}

int cantcp_send(cantcp_tunnel_t *tunnel, struct can_frame *msg)
{
	return cantcp_core_send_frame(tunnel, msg);
}

int cantcp_recv(cantcp_tunnel_t *tunnel, struct can_frame *msg)
{
	return cantcp_core_recv_frame(tunnel, msg);
}
