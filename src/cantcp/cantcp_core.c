/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cantcp.h"

#include <stddef.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <sys/types.h>
LOG_MODULE_REGISTER(cantcp_core, LOG_LEVEL_WRN);

static int sendall(int sock, const uint8_t *buf, size_t len)
{
	ssize_t ret = -EINVAL;

	while (len > 0U) {
		ret = zsock_send(sock, buf, len, 0);
		if (ret > 0) {
			buf += ret;
			len -= ret;
			LOG_HEXDUMP_DBG(buf - ret, ret, "sent");
		} else if (ret == 0) {
			LOG_WRN("(%d) connection closed", sock);
			break;
		} else if (ret < 0) {
			// handle EAGAIN case if non-blocking option is enabled
			LOG_ERR("(%d) failed to send = %d", sock, ret);
			break;
		}
	}

	return ret;
}

static int recvall(int sock, uint8_t *buf, size_t len)
{
	ssize_t ret = -EINVAL;

	while (len > 0U) {
		ret = zsock_recv(sock, buf, len, 0);
		if (ret > 0) {
			buf += ret;
			len -= ret;
			LOG_HEXDUMP_DBG(buf - ret, ret, "received");
		} else if (ret == 0) {
			LOG_WRN("(%d) connection closed", sock);
			break;
		} else if (ret < 0) {
			// handle EAGAIN case if non-blocking option is enabled
			LOG_ERR("(%d) failed to recv = %d", sock, ret);
			break;
		}
	}

	return ret;
}

int cantcp_core_tunnel_init(cantcp_tunnel_t *tunnel)
{
	memset(tunnel, 0U, sizeof(cantcp_tunnel_t));

	tunnel->sock				= -1;
	tunnel->flags.secure		= CANTCP_UNSECURE;
	tunnel->flags.blocking_mode = CANTCP_BLOCKING;
	tunnel->flags.bus			= CANTCP_BUS_DEFAULT;

	tunnel->keep_alive_timeout = CANTCP_DEFAULT_KEEP_ALIVE_TIMEOUT;
	tunnel->max_retries		   = CANTCP_DEFAULT_MAX_RETRIES;
	tunnel->retry_delay		   = CANTCP_DEFAULT_RETRY_DELAY;

	tunnel->server.port = CANTCP_DEFAULT_PORT;

	return 0U;
}

static size_t get_frame_length(struct can_frame *msg)
{
	return sizeof(struct can_frame);
}

int cantcp_core_send_frame(cantcp_tunnel_t *tunnel, struct can_frame *msg)
{
	int ret;
	size_t sent = 0U;

	/* send header */
	struct {
		uint16_t length;
	} header = {.length = get_frame_length(msg)};

	ret = sendall(tunnel->sock, (const void *)&header, sizeof(header));
	LOG_DBG("sent = %d", ret);
	if (ret <= 0) {
		goto exit;
	}

	sent += ret;

	/* send frame */
	ret = sendall(tunnel->sock, (const void *)msg, header.length);
	LOG_DBG("sent = %d", ret);
	if (ret <= 0) {
		goto exit;
	}

	sent += ret;

	return sent;
exit:
	return ret;
}

int cantcp_core_recv_frame(cantcp_tunnel_t *tunnel, struct can_frame *msg)
{
	int ret;
	size_t recv = 0U;

	/* recv header */
	struct {
		uint16_t length;
	} header = {.length = 0U};

	ret = recvall(tunnel->sock, (void *)&header, sizeof(header));
	if (ret <= 0) {
		goto exit;
	}

	if (header.length != sizeof(struct can_frame)) {
		LOG_ERR("invalid frame length = %hu, expected %u", header.length,
				sizeof(struct can_frame));
		ret = -1;
		goto exit;
	}

	recv += ret;

	/* recv frame */
	ret = recvall(tunnel->sock, (void *)msg, header.length);
	if (ret <= 0) {
		goto exit;
	}

	recv += ret;

	return recv - 2U; /* remove header size */
exit:
	return ret;
}

typedef enum {
	DATA_FRAME	  = 0,
	FILTER_FRAME  = 1,
	CONTROL_FRAME = 2,
	ERROR_FRAME	  = 3,
} cantcp_frame_type_t;

struct cantcp_control_frame {
};

struct cantcp_error_frame {
	int error;
};

struct cantcp_header {
	cantcp_frame_type_t frame_type : 2;
};

struct cantcp_frame {
	struct cantcp_header header;

	union {
		struct can_frame frame;
		struct can_filter filter;
		struct cantcp_control_frame control;
		struct cantcp_error_frame error;
	};
};

// typedef struct cantcp_frame
// {
// 	uint16_t length;
// 	uint8_t *buffer;
// };