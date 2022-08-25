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
#include "utils/buffers.h"

#include "creds/credentials.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_server, LOG_LEVEL_INF); /* INF */




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




static void http_srv_thread(void *_a, void *_b, void *_c);

#define HTTP_SERVER_THREAD_STACK_SIZE (0x1000U)
K_THREAD_DEFINE(http_thread,
		HTTP_SERVER_THREAD_STACK_SIZE,
		http_srv_thread, NULL, NULL, NULL,
		K_PRIO_PREEMPT(8),
		0, 0); 






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
	return;

	LOG_DBG("servers_count=%d clients_count=%d", servers_count, clients_count);
	for (struct pollfd *pfd = fds.array;
	     pfd < fds.array + ARRAY_SIZE(fds.array); pfd++) {
		LOG_DBG("\tfd=%d ev=%d", pfd->fd, (int)pfd->events);
	}
}




// forward declarations 
static bool process_request(http_connection_t *conn);




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

static int send_headers(http_connection_t *conn)
{
	int ret;
	buffer_t buf;
	buffer_init(&buf, buffer_internal, sizeof(buffer_internal));
	http_response_t *const resp = conn->resp;

	ret = http_encode_status(&buf, resp->status_code);
	ret = http_encode_header_connection(&buf, conn->keep_alive.enabled);
	ret = http_encode_header_content_type(&buf, resp->content_type);
	if (resp->stream == 1u) {
		ret = http_encode_header_transer_encoding_chunked(&buf);
	} else {
		ret = http_encode_header_content_length(&buf, resp->content_length);
	}
	ret = http_encode_header_end(&buf);

	/* Send custom headers */

	/* TODO check for errors */
	(void) ret;

	ret = sendall(conn->sock, buf.data, buf.filling);
	if (ret >= 0) {
		resp->headers_sent += ret;
	}
	return ret;
}

/* Response if the request is discarded */
static bool handle_error_response(http_connection_t *conn)
{
	__ASSERT_NO_MSG(http_request_is_discarded(conn->req));

	http_discard_reason_to_status_code(conn->req->discard_reason,
					   &conn->resp->status_code);

	return send_headers(conn) > 0;
}

static void request_cleanup(http_request_t *req)
{
	if (http_request_is_stream(req)) {
		req->chunk.id = 0;
		req->chunk.len = 0;
		req->chunk.loc = NULL;
	}
}

static bool handle_request(http_connection_t *conn)
{
	ssize_t rc;

	/* Buffer is shared between request and response */
	cursor_buffer_t cbuf;
	cursor_buffer_init(&cbuf, buffer, sizeof(buffer));

	http_request_t *const req = conn->req;

	while (req->complete == 0U) {
		__ASSERT_NO_MSG(cursor_buffer_full(&cbuf) == false);

		rc = zsock_recv(conn->sock, cbuf.cursor, 
					cursor_buffer_remaining(&cbuf), 0);
		LOG_DBG("zsock_recv(%d, %p, %d, 0) = %d", conn->sock, 
			cbuf.cursor, cursor_buffer_remaining(&cbuf), rc);
		if (rc < 0) {
			if (rc == -EAGAIN) {
				LOG_WRN("-EAGAIN = %d", -EAGAIN);
				/* TODO find a way to return to wait for data */
				continue;
			}

			LOG_ERR("recv failed = %d", rc);
			goto close;
		} else if (rc == 0) {
			LOG_INF("(%d) Connection closed by peer", conn->sock);
			goto close;
		} else {
			if (http_request_parse(req, cbuf.cursor, rc) == false) {
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
			if (http_request_is_message(req)) {
				cursor_buffer_shift(&cbuf, rc);

				if (cursor_buffer_full(&cbuf) == true) {
					http_request_discard(req, HTTP_REQUEST_PAYLOAD_TOO_LARGE);
					LOG_WRN("(%d) Recv buffer full, discarding ...",
						conn->sock);
				}
			}

			if (http_request_is_discarded(req) == true) {
				cursor_buffer_reset(&cbuf);
			}
		}
	}

	request_cleanup(req);

	return true;

close:
	return false;
}

static bool send_buffer(http_connection_t *conn)
{
	http_response_t *const resp = conn->resp;

	__ASSERT_NO_MSG(resp->stream == 0u);

	int ret = sendall(conn->sock, resp->buffer.data, resp->buffer.filling);
	if (ret >= 0) {
		resp->payload_sent += ret;

		if (resp->payload_sent > resp->content_length) {
			LOG_ERR("(%d) Payload sent > content_length, %u > %u, closing",
				conn->sock, resp->payload_sent, resp->content_length);
			goto close;
		}
	} else {
		goto close;
	}

	return true;
close:
	return false;
}

static bool send_chunk(http_connection_t *conn)
{
	int ret;
	http_response_t *const resp = conn->resp;

	__ASSERT_NO_MSG(resp->stream == 1u);

	/* Prepare chunk header */
	char chunk_header[16];
	ret = snprintf(chunk_header, sizeof(chunk_header), "%x\r\n",
		       resp->buffer.filling);
	if (ret < 0) {
		goto close;
	}

	/* Send chunk header */
	ret = sendall(conn->sock, chunk_header, ret);
	if (ret < 0) {
		goto close;
	}

	/* Send chunk data */
	ret = sendall(conn->sock, resp->buffer.data, resp->buffer.filling);
	if (ret >= 0) {
		resp->payload_sent += ret;
	} else {
		goto close;
	}

	/* Send end of chunk */
	ret = sendall(conn->sock, "\r\n", 2u);
	if (ret < 0) {
		goto close;
	}

	return true;
close:
	return false;
}

static bool send_end_of_chunked_encoding(http_connection_t *conn)
{
	int ret;

	__ASSERT_NO_MSG(conn->resp->stream == 1u);

	/* Send end of chunk */
	ret = sendall(conn->sock, "0\r\n\r\n", 5u);

	return ret >= 0;
}

static bool handle_response(http_connection_t *conn)
{
	int ret;

	http_request_t *const req = conn->req;
	http_response_t *const resp = conn->resp;

	resp->content_type = http_route_resp_default_content_type(req->route);

	do {
		/* Prepare the buffer for route handler call */
		buffer_reset(&resp->buffer);

		/* Mark as response by default */
		resp->complete = 1u;

#if defined(CONFIG_HTTP_TEST)
		/* Should always be called when the handler is called */
		http_test_run(&req->_test_ctx, req, resp, HTTP_TEST_HANDLER_RESP);
#endif /* CONFIG_HTTP_TEST */

		/* process request, prepare response */
		ret = req->route->resp_handler(req, resp);
		if (ret != 0) {
			http_request_discard(conn->req, HTTP_REQUEST_PROCESSING_ERROR);
			LOG_ERR("(%d) Request processing failed = %d", conn->sock, ret);
		}

		if (http_response_is_first_call(resp) == true) {
			if (http_request_is_discarded(req)) {
				http_discard_reason_to_status_code(req->discard_reason,
								   &resp->status_code);
			}

			if (http_code_has_payload(resp->status_code)) {
				/* If response handler is called a single time and content-length
				 * is not set then set it to the length of the buffer */
				if (resp->complete && resp->content_length == 0u) {
					resp->content_length = resp->buffer.filling;
					LOG_DBG("(%d) Content-Length not configure, forced to %u",
						conn->sock, resp->content_length);
				}
			} else {
				// if (resp->complete != 1u || resp->content_length != 0) {
				// 	LOG_WRN("(%d) No payload expected !", conn->sock);
				// }

				resp->content_length = 0;
				resp->complete = 1u;
				resp->buffer.filling = 0u;
			}

			/* Headers sent after handler first call */
			ret = send_headers(conn);
			if (ret <= 0) {
				LOG_ERR("(%d) Failed to send headers = %d", conn->sock, ret);
				goto exit;
			}
		}

		const bool success = http_response_is_stream(resp) ? 
			send_chunk(conn) : 
			send_buffer(conn);
		if (!success) {
			goto close;
		}

		resp->calls_count++;
	} while (resp->complete == 0U);

	/* End of chunked encoding */
	if (http_response_is_stream(resp)) {
		if (!send_end_of_chunked_encoding(conn)) {
			goto close;
		}
	}

exit:
	return ret > 0;

close:
	return false;
}



static bool process_request(http_connection_t *conn)
{
	static http_request_t req;
	static http_response_t resp;
	
	http_request_init(&req);
	http_response_init(&resp);
	
	/* Buffer is shared between request and response */
	buffer_init(&resp.buffer, buffer, sizeof(buffer));

	conn->req = &req;
	conn->resp = &resp;

	if (handle_request(conn) == false) {
		goto close;
	}

	const bool success = http_request_is_discarded(&req) ?
		handle_error_response(conn) :
		handle_response(conn);
	if (!success) {
		goto close;
	}

	LOG_INF("(%d) Req %s [payload %u B] returned resp status %d [payload %u B] "
		"(keep-alive=%d)", conn->sock, req.url, req.payload_len, resp.status_code,
		resp.payload_sent, conn->keep_alive.enabled);

	/* We update the connection keep_alive configuration
	* based on the request.
	*/
	conn->keep_alive.enabled = req.keep_alive;
	if (conn->keep_alive.enabled) {
		conn->keep_alive.last_activity = k_uptime_get_32();
		return true;
	}

close:
	return false;
}


