/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_server.h"

#include <stdio.h>
#include <stddef.h>

#include <poll.h>
#include <net/socket.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_config.h>
#include <net/tls_credentials.h>

#include <net/http_parser.h>

#include "app_sections.h"
#include "http_response.h"
#include "http_request.h"
#include "http_utils.h"
#include "http_conn.h"
#include "routes.h"
#include "utils.h"

#include "creds/credentials.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_server, LOG_LEVEL_WRN); /* INF */

/*____________________________________________________________________________*/

#define HTTP_PORT       80
#define HTTPS_PORT      443

#define HTTPS_SERVER_SEC_TAG   1

#if CONFIG_HTTP_SERVER_NONSECURE
#       define SERVER_FD_COUNT        2
#else
#       define SERVER_FD_COUNT        1
#endif 

static const sec_tag_t sec_tag_list[] = {
	HTTPS_SERVER_SEC_TAG
};

#define KEEP_ALIVE_DEFAULT_TIMEOUT_MS  (30*1000)

/*____________________________________________________________________________*/

static void http_srv_thread(void *_a, void *_b, void *_c);

#define HTTP_SERVER_THREAD_STACK_SIZE (0x1000U)
K_THREAD_DEFINE(http_thread,
		HTTP_SERVER_THREAD_STACK_SIZE,
		http_srv_thread, NULL, NULL, NULL,
		K_PRIO_PREEMPT(8),
		0, 0); 


/*____________________________________________________________________________*/


/* We use the same buffer for all connections,
 * each HTTP request should be parsed and processed immediately.
 *
 * Same buffer for HTTP request and HTTP response
 */
__buf_noinit_section char buffer[0x3000];
__buf_noinit_section char buffer_internal[0x800]; /* For encoding response headers */

/**
 * @brief
 * - 1 TCP socket for HTTP
 * - 1 TLS socket for HTTPS
 * - 3 client sockets
 */
static union
{
	struct pollfd array[CONFIG_HTTP_MAX_CONNECTIONS + SERVER_FD_COUNT];
	struct {
#if CONFIG_HTTP_SERVER_NONSECURE
		struct pollfd srv;      /* unsecure server socket */
#endif
		struct pollfd sec;      /* secure server socket */
		struct pollfd cli[CONFIG_HTTP_MAX_CONNECTIONS];
	};
} fds;

static int servers_count = 0;
static int clients_count = 0;

extern const struct http_parser_settings parser_settings;

/* debug functions */
static void show_pfd(void)
{
	LOG_DBG("servers_count=%d clients_count=%d", servers_count, clients_count);
	for (struct pollfd *pfd = fds.array;
	     pfd < fds.array + ARRAY_SIZE(fds.array); pfd++) {
		LOG_DBG("\tfd=%d ev=%d", pfd->fd, (int)pfd->events);
	}
}

/*____________________________________________________________________________*/

// forward declarations 
static bool process_request(http_connection_t *conn);

/*____________________________________________________________________________*/

static int setup_socket(struct pollfd *pfd, bool secure)
{
	int sock, ret;
	struct sockaddr_in local = {
		.sin_family = AF_INET,
		.sin_port = htons(secure ? HTTPS_PORT : HTTP_PORT),
		.sin_addr = {
			.s_addr = INADDR_ANY
		}
	};

	sock = zsock_socket(AF_INET, SOCK_STREAM, secure ?
			    IPPROTO_TLS_1_2 : IPPROTO_TCP);
	if (sock < 0) {
		ret = sock;
		LOG_ERR("Failed to create socket = %d", ret);
		goto exit;
	}

	/* set secure tag */
	if (secure) {
		ret = zsock_setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST,
				       sec_tag_list, sizeof(sec_tag_list));
		if (ret < 0) {
			LOG_ERR("(%d) Failed to set TCP secure option : %d",
				sock, ret);
			goto exit;
		}
	}

	ret = zsock_bind(sock, (const struct sockaddr *)&local,
			 sizeof(struct sockaddr_in));
	if (ret < 0) {
		LOG_ERR("(%d) Failed to bind socket = %d", sock, ret);
		goto exit;
	}

	/* TODO adjust the backlog value */
	ret = zsock_listen(sock, 3);
	if (ret < 0) {
		LOG_ERR("(%d) Failed to listen socket = %d", sock, ret);
		goto exit;
	}

	pfd->fd = sock;
	pfd->events = POLLIN;

	servers_count++;

	ret = sock;
exit:
	return ret;
}

int setup_sockets(void)
{
	/* setup non-secure HTTP socket (port 80) */
#if CONFIG_HTTP_SERVER_NONSECURE
	if (setup_socket(&fds.srv, false) < 0) {
		goto exit;
	}
#endif /* CONFIG_HTTP_SERVER_NONSECURE */

	/* setup secure HTTPS socket (port 443) */

	/* include this PR : https://github.com/zephyrproject-rtos/zephyr/pull/40255
	 * related issue : https://github.com/zephyrproject-rtos/zephyr/issues/40267
	 */
	tls_credential_add(
		HTTPS_SERVER_SEC_TAG,
		TLS_CREDENTIAL_SERVER_CERTIFICATE,
		x509_public_certificate_rsa1024_der,
		sizeof(x509_public_certificate_rsa1024_der));
	tls_credential_add(
		HTTPS_SERVER_SEC_TAG,
		TLS_CREDENTIAL_PRIVATE_KEY,
		rsa_private_key_rsa1024_der,
		sizeof(rsa_private_key_rsa1024_der));

	if (setup_socket(&fds.sec, true) < 0) {
		goto exit;
	}

	clients_count = 0;
exit:
	return -1;
}

static void remove_pollfd_by_index(uint_fast8_t index)
{
	LOG_DBG("Compress fds count = %u", clients_count);

	if (index >= clients_count) {
		return;
	}

	int move_count = clients_count - index;
	if (move_count > 0) {
		memmove(&fds.cli[index],
			&fds.cli[index + 1],
			move_count * sizeof(struct pollfd));
	}

	memset(&fds.cli[clients_count], 0U,
	       sizeof(struct pollfd));

	clients_count--;

	show_pfd();
}

static int srv_accept(int serv_sock)
{
	int ret, sock;
	struct sockaddr_in addr;
	http_connection_t *conn;
	socklen_t len = sizeof(struct sockaddr_in);

	uint32_t a = k_uptime_get();

	sock = zsock_accept(serv_sock, (struct sockaddr *)&addr, &len);
	if (sock < 0) {
		LOG_ERR("(%d) Accept failed = %d", serv_sock, sock);
		ret = sock;
		goto exit;
	}

	char ipv4_str[NET_IPV4_ADDR_LEN];
	ipv4_to_str(&addr.sin_addr, ipv4_str, sizeof(ipv4_str));

	LOG_DBG("(%d) Accepted connection, allocating connection context, cli sock = %d",
		serv_sock, sock);

	conn = http_conn_alloc();
	if (conn == NULL) {
		LOG_WRN("(%d) Connection refused from %s:%d, cli sock = %d", serv_sock,
			log_strdup(ipv4_str), htons(addr.sin_port), sock);

		zsock_close(sock);

		ret = -1;
		goto exit;
	} else {
		LOG_INF("(%d) Connection accepted from %s:%d, cli sock = %d", serv_sock,
			log_strdup(ipv4_str), htons(addr.sin_port), sock);

		__ASSERT_NO_MSG(clients_count < CONFIG_HTTP_MAX_CONNECTIONS);

		struct pollfd *pfd = &fds.cli[clients_count++];

		pfd->fd = sock;
		pfd->events = POLLIN;

		/* reference connection socket */
		conn->sock = sock;

		/* initialize keep-alive context */
		conn->keep_alive.timeout = KEEP_ALIVE_DEFAULT_TIMEOUT_MS;
		conn->keep_alive.last_activity = k_uptime_get_32();
	}

	show_pfd();

	uint32_t b = k_uptime_get();

	LOG_DBG("Accept delay %u ms", b - a);

	return 0;
exit:
	return ret;
}

static void http_srv_thread(void *_a, void *_b, void *_c)
{
	ARG_UNUSED(_a);
	ARG_UNUSED(_b);
	ARG_UNUSED(_c);

	int ret, timeout;

	http_conn_init();

	setup_sockets();

	for (;;) {
		show_pfd();

		timeout = http_conn_time_to_next_outdated();
		LOG_DBG("zsock_poll timeout: %d ms", timeout);

		ret = zsock_poll(fds.array, clients_count + servers_count, timeout);
		if (ret >= 0) {
#if CONFIG_HTTP_SERVER_NONSECURE
			if (fds.srv.revents & POLLIN) {
				ret = srv_accept(fds.srv.fd);
			}
#endif /* CONFIG_HTTP_SERVER_NONSECURE */

			if (fds.sec.revents & POLLIN) {
				ret = srv_accept(fds.sec.fd);
			}

			/* We iterate over the connections and check if there are any data,
			 * or if the connection has timeout.
			 */
			uint_fast8_t idx = 0;
			while (idx < clients_count) {
				http_connection_t *conn = http_conn_get_by_sock(fds.cli[idx].fd);

				__ASSERT_NO_MSG(conn != NULL);

				bool close = false;

				if (fds.cli[idx].revents & POLLIN) { /* data available */
					close = process_request(conn) != true;
				} else if (http_conn_is_outdated(conn)) { /* check if the connection has timed out */
					close = true;
					LOG_WRN("(%d) Closing outdated connection %p", conn->sock, conn);
				}

				/* Close the connection, remove the socket from the pollfd array */
				if (close) {
					LOG_INF("(%d) Closing sock conn %p", conn->sock, conn);
					zsock_close(conn->sock);
					http_conn_free(conn);
					remove_pollfd_by_index(idx);
					show_pfd();
				} else {
					idx++;
				}
			}
		} else {
			LOG_ERR("unexpected poll(%p, %d, %d) return value = %d",
				&fds, clients_count + servers_count, SYS_FOREVER_MS, ret);

			/* TODO remove, sleep 1 sec here */
			k_sleep(K_MSEC(5000));
		}
	}

#if CONFIG_HTTP_SERVER_NONSECURE
	zsock_close(fds.srv.fd);
#endif /* CONFIG_HTTP_SERVER_NONSECURE */

	zsock_close(fds.sec.fd);
}

static int sendall(int sock, char *buf, size_t len)
{
	int ret;
	size_t sent = 0;

	LOG_DBG("sendall(%d, %p, %u)", sock, buf, len);

	while (sent < len) {
		ret = zsock_send(sock, &buf[sent], len - sent, 0U);
		if (ret < 0) {
			if (ret == -EAGAIN) {
				LOG_INF("-EAGAIN (%d)", sock);
				continue;
			}
			goto exit;
		} else if (ret > 0) {
			sent += ret;
		} else {
			LOG_ERR("ret == %d ???", 0);
			goto exit;
		}
	}

	ret = sent;

exit:
	return ret;
}

static int send_response(http_connection_t *conn)
{
	int ret, sent;
	char *b = buffer_internal;
	const size_t buf_size = sizeof(buffer_internal);
	http_response_t *resp = conn->resp;
	
	int encoded = 0;

	ret = http_encode_status(b + encoded, buf_size - encoded,
				 resp->status_code);

	encoded += ret;
	ret = http_encode_header_connection(b + encoded, buf_size - encoded,
					    (bool)conn->keep_alive.enabled);

	encoded += ret;
	ret = http_encode_header_content_type(b + encoded, buf_size - encoded,
					      resp->content_type);

	encoded += ret;
	ret = http_encode_header_content_length(b + encoded, buf_size - encoded,
						resp->content_length);

	encoded += ret;
	ret = http_encode_header_end(b + encoded, buf_size - encoded);

	encoded += ret;

	/* send headers */
	ret = sendall(conn->sock, b, encoded);
	if (ret < 0) {
		goto exit;
	}

	sent = ret;

	/* send body */
	if (http_code_has_payload(resp->status_code)) {
		ret = sendall(conn->sock, resp->buffer.data, resp->buffer.filling);
		if (ret < 0) {
			goto exit;
		}
		sent += ret;
	} else if (resp->content_length > 0) {
		LOG_WRN("Trying to send a content for invalid response "
			"code (%d != 200)", resp->status_code);
	}

	ret = sent;
exit:
	return ret;
}


static bool process_request(http_connection_t *conn)
{
	static http_request_t req;
	static http_response_t resp;

	static cursor_buffer_t cbuf;
	
	http_request_init(&req);
	http_response_init(&resp);
	
	/* Buffer is shared between request and response */
	cursor_buffer_init(&cbuf, buffer, sizeof(buffer));
	buffer_init(&resp.buffer, buffer, sizeof(buffer));

	conn->req = &req;
	conn->resp = &resp;

	while (conn->req->complete == 0U) {
		__ASSERT_NO_MSG(cursor_buffer_full(&cbuf) == false);

		ssize_t rc = zsock_recv(conn->sock, cbuf.cursor, cursor_buffer_remaining(&cbuf), 0);
		LOG_DBG("zsock_recv(%d, %p, %d, 0) = %d", conn->sock, cbuf.cursor, cursor_buffer_remaining(&cbuf), rc);
		if (rc < 0) {
			if (rc == -EAGAIN) {
				LOG_WRN("-EAGAIN = %d", -EAGAIN);
				/* Todo, find a way to return to function caller */
				continue;
			}

			LOG_ERR("recv failed = %d", rc);
			goto close;
		} else if (rc == 0) {
			LOG_INF("(%d) Connection closed by peer", conn->sock);
			goto close;
		} else {
			if (http_request_parse(&req, cbuf.cursor, rc) == false) {
				goto close;
			}

			/* Prepare buffer for next receiving.
			*
			* Only shift the position in the buffer if we are
			* handling a "message" request
			*
			* (if stream) reset buffer as the stream route
			* handler has already consumed the data, so no need to
			* update it */
			if (http_request_is_message(&req)) {
				cursor_buffer_shift(&cbuf, rc);

				if (cursor_buffer_full(&cbuf) == true) {
					http_request_discard(&req, HTTP_REQUEST_PAYLOAD_TOO_LARGE);
					LOG_WRN("(%d) Recv buffer full, discarding ...",
						conn->sock);
				}
			}

			if (http_request_is_discarded(&req) == true) {
				cursor_buffer_reset(&cbuf);
			}
		}
	}

	/* prepare response, move to a function */

	if (http_request_is_discarded(&req)) {
		switch(req.discard_reason) {
		case HTTP_REQUEST_ROUTE_UNKNOWN:
			resp.status_code = HTTP_NOT_FOUND;
			break;
		case HTTP_REQUEST_ROUTE_NO_HANDLER:
			resp.status_code = HTTP_NOT_IMPLEMENTED;
			break;
		case HTTP_REQUEST_STREAMING_UNSUPPORTED:
			resp.status_code = HTTP_NOT_IMPLEMENTED;
			break;
		case HTTP_REQUEST_PAYLOAD_TOO_LARGE:
			resp.status_code = HTTP_REQUEST_ENTITY_TOO_LARGE;
			break;
		case HTTP_REQUEST_STREAM_PROCESSING_ERROR:
		case HTTP_REQUEST_FATAL_ERROR:
		default:
			resp.status_code = HTTP_INTERNAL_SERVER_ERROR;
			break;
		}
	} else {
		/* We update the connection keep_alive configuration
		* based on the request.
		*/
		conn->keep_alive.enabled = req.keep_alive;

		/* Clear chunk buffer */
		if (http_request_is_stream(&req)) {
			req.chunk.loc = NULL;
			req.chunk.len = 0;
			req.chunk.id = 0;
		}

		resp.content_type = http_route_resp_default_content_type(req.route);

#if defined(CONFIG_HTTP_TEST)
		http_test_run(&req._test_ctx, &req, &resp);
#endif /* CONFIG_HTTP_TEST */

		/* process request, prepare response */
		int ret = req.route->handler(&req, &resp);
		if (ret != 0) {
			resp.status_code = HTTP_INTERNAL_SERVER_ERROR;
			LOG_ERR("(%d) Request processing failed = %d", conn->sock, ret);
		}

		/* TODO check whether resp.complete flag is set and
		 * call the handler again */

		req.calls_count++;
	}

	/* Set content-length according to the buffer filling */
	/* TODO check for request type : stream/data */
	resp.content_length = resp.buffer.filling;

	int sent = send_response(conn);
	if (sent < 0) {
		goto close;
	}

	LOG_INF("(%d) Processing req total len %u B resp status %d len %u B (keep"
		"-alive=%d)", conn->sock, req.payload_len, resp.status_code,
		sent, conn->keep_alive.enabled);

	if (conn->keep_alive.enabled) {
		conn->keep_alive.last_activity = k_uptime_get_32();
		return true;
	}

close:
	return false;
}

/*____________________________________________________________________________*/