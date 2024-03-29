/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_sections.h"
#include "assert.h"
#include "creds/manager.h"
#include "http_request.h"
#include "http_response.h"
#include "http_server.h"
#include "http_session.h"
#include "http_utils.h"
#include "routes.h"
#include "utils/buffers.h"
#include "utils/misc.h"

#include <stddef.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/http/parser.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/tls_verify_cb.h>
#include <zephyr/posix/poll.h>

#include <fcntl.h>
#include <mbedtls/oid.h>
#include <mbedtls/x509_crt.h>
#include <user/auth.h>
LOG_MODULE_REGISTER(http_server, LOG_LEVEL_INF); /* INF */

/* O_NONBLOCK */
#define SOCK_BLOCKING_OPT 0u

#define HTTP_PORT  80
#define HTTPS_PORT 443

#define HTTPS_SERVER_SEC_TAG 1

#if defined(CONFIG_APP_HTTP_SERVER_SECURE) && defined(CONFIG_APP_HTTP_SERVER_NONSECURE)
#define SERVER_FD_COUNT 2
#elif defined(CONFIG_APP_HTTP_SERVER_SECURE) || defined(CONFIG_APP_HTTP_SERVER_NONSECURE)
#define SERVER_FD_COUNT 1u
#else
#error "No server socket configured"
#endif

// externs
extern size_t strnlen(const char *, size_t);

// forward declarations
static bool process_request(http_session_t *sess);
static void http_srv_thread(void *_a, void *_b, void *_c);

static const sec_tag_t sec_tag_list[] = {HTTPS_SERVER_SEC_TAG};

#define KEEP_ALIVE_DEFAULT_TIMEOUT_MS (CONFIG_APP_HTTP_SESSION_TIMEOUT * 1000)

K_THREAD_DEFINE(http_thread,
				CONFIG_APP_HTTP_THREAD_STACK_SIZE,
				http_srv_thread,
				NULL,
				NULL,
				NULL,
				K_PRIO_PREEMPT(5u),
				0,
				0);

/* We use the same buffer for all sessions,
 * each HTTP request should be parsed and processed immediately.
 *
 * Same buffer for HTTP request and HTTP response
 *
 * TODO: Find a way to use a single 0x1000 sized buffer
 */
__buf_noinit_section char buffer[CONFIG_APP_HTTP_BUFFER_SIZE];
__buf_noinit_section char buffer_internal[CONFIG_APP_HTTP_HEADERS_BUFFER_SIZE]; /* For
									   encoding
								   response headers */

static struct http_stats stats;

/**
 * @brief
 * - 1 TCP socket for HTTP
 * - 1 TLS socket for HTTPS
 * - 3 client sockets
 */
static union {
	struct pollfd array[CONFIG_APP_HTTP_MAX_SESSIONS + SERVER_FD_COUNT];
	struct {
#if defined(CONFIG_APP_HTTP_SERVER_NONSECURE)
		struct pollfd srv; /* unsecure server socket */
#endif
#if defined(CONFIG_APP_HTTP_SERVER_SECURE)
		struct pollfd sec; /* secure server socket */
#endif
		struct pollfd cli[CONFIG_APP_HTTP_MAX_SESSIONS];
	};
} fds;

static int servers_count = 0;
static int clients_count = 0;

/* debug functions */
static void show_pfd(void)
{
	return;

	LOG_DBG("servers_count=%d clients_count=%d", servers_count, clients_count);
	for (struct pollfd *pfd = fds.array; pfd < fds.array + ARRAY_SIZE(fds.array); pfd++) {
		LOG_DBG("\tfd=%d ev=%d", pfd->fd, (int)pfd->events);
	}
}

const struct user *handshake_auth_user = NULL;

#if defined(CONFIG_NET_SOCKETS_TLS_PEER_VERIFY_CALLBACK)

static int tls_verify_callback(void *user_data,
							   struct mbedtls_x509_crt *crt,
							   int depth,
							   uint32_t *flags)
{
	if (depth != 0) return 0;

	const mbedtls_x509_name *name;
	struct user_auth auth;

	LOG_INF("tls_verify_callback: user_data: %p, crt: %p, depth: %d, flags: %08x",
			user_data, crt, depth, *flags);

	for (name = &crt->subject; name != NULL; name = name->next) {
		if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_CN, &name->oid) == 0) {
			memcpy(auth.username, name->val.p, name->val.len);
			auth.username[name->val.len] = '\0';

			handshake_auth_user = user_auth_verify(&auth);

			LOG_INF("username: %s role: %s", handshake_auth_user->name,
					user_role_to_str(handshake_auth_user->role));

			break;
		}
	}

	return 0;
}

#endif /* CONFIG_NET_SOCKETS_TLS_PEER_VERIFY_CALLBACK */

static int setup_socket(struct pollfd *pfd, bool secure)
{
	int sock, ret;
	struct sockaddr_in local = {
		.sin_family = AF_INET,
		.sin_port	= htons(secure ? HTTPS_PORT : HTTP_PORT),
		.sin_addr	= {.s_addr = INADDR_ANY},
	};

	sock = zsock_socket(AF_INET, SOCK_STREAM, secure ? IPPROTO_TLS_1_2 : IPPROTO_TCP);
	if (sock < 0) {
		ret = sock;
		LOG_ERR("Failed to create socket = %d", ret);
		goto exit;
	}

	ret = zsock_fcntl(sock, F_SETFL, SOCK_BLOCKING_OPT);
	if (ret < 0) {
		LOG_ERR("(%d) Failed to set socket non-blocking = %d", sock, ret);
		goto exit;
	}

	/* set secure tag */
	if (secure) {
		ret = zsock_setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
							   sizeof(sec_tag_list));
		if (ret < 0) {
			LOG_ERR("(%d) Failed to set TLS tag list : %d", sock, ret);
			goto exit;
		}

#if defined(CONFIG_APP_HTTP_SERVER_VERIFY_CLIENT)
		int verify;

		verify = TLS_PEER_VERIFY_OPTIONAL;
		ret	   = zsock_setsockopt(sock, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(int));
		if (ret < 0) {
			LOG_ERR("(%d) Failed to set TLS peer verify option : "
					"%d",
					sock, ret);
			goto exit;
		}

#if defined(CONFIG_NET_SOCKETS_TLS_PEER_VERIFY_CALLBACK)
		struct tls_verify_cb verify_cb;
		verify_cb.callback	= tls_verify_callback;
		verify_cb.user_data = (void *)0xAABBCCDD;

		ret = zsock_setsockopt(sock, SOL_TLS, TLS_PEER_VERIFY_CB, &verify_cb,
							   sizeof(verify_cb));
		if (ret < 0) {
			LOG_ERR("(%d) Failed to set TLS peer verify callback : "
					"%d",
					sock, ret);
			goto exit;
		}
#endif /* CONFIG_NET_SOCKETS_TLS_PEER_VERIFY_CALLBACK */
#endif /* CONFIG_APP_HTTP_SERVER_VERIFY_CLIENT */
	}

	ret = zsock_bind(sock, (const struct sockaddr *)&local, sizeof(struct sockaddr_in));
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

	pfd->fd		= sock;
	pfd->events = POLLIN;

	servers_count++;

	ret = sock;
exit:
	return ret;
}

int setup_sockets(void)
{
	int ret;

	/* setup non-secure HTTP socket (port 80) */
#if defined(CONFIG_APP_HTTP_SERVER_NONSECURE)
	ret = setup_socket(&fds.srv, false);
	if (ret < 0) {
		goto exit;
	}
#endif /* CONFIG_APP_HTTP_SERVER_NONSECURE */

#if defined(CONFIG_APP_HTTP_SERVER_SECURE)
	/* setup secure HTTPS socket (port 443) */
	struct cred cert, key;
	ret = cred_get(CRED_HTTPS_SERVER_CERTIFICATE, &cert);
	CHECK_OR_EXIT(ret == 0);
	ret = cred_get(CRED_HTTPS_SERVER_PRIVATE_KEY, &key);
	CHECK_OR_EXIT(ret == 0);

#if defined(CONFIG_APP_HTTP_SERVER_VERIFY_CLIENT)
	struct cred ca;
	ret = cred_get(CRED_HTTPS_SERVER_CLIENT_CA_DER, &ca);
	CHECK_OR_EXIT(ret == 0);
#endif

	/* include this PR :
	 * https://github.com/zephyrproject-rtos/zephyr/pull/40255 related issue
	 * : https://github.com/zephyrproject-rtos/zephyr/issues/40267
	 */
	tls_credential_add(HTTPS_SERVER_SEC_TAG, TLS_CREDENTIAL_SERVER_CERTIFICATE, cert.data,
					   cert.len);
	tls_credential_add(HTTPS_SERVER_SEC_TAG, TLS_CREDENTIAL_PRIVATE_KEY, key.data,
					   key.len);

#if defined(CONFIG_APP_HTTP_SERVER_VERIFY_CLIENT)
	tls_credential_add(HTTPS_SERVER_SEC_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, ca.data,
					   ca.len);
#endif

	ret = setup_socket(&fds.sec, true);
	if (ret < 0) {
		goto exit;
	}
#endif

	clients_count = 0;
	ret			  = 0;
exit:
	return ret;
}

static void remove_pollfd_by_index(uint8_t index)
{
	LOG_DBG("Compress fds count = %u", clients_count);

	if (index >= clients_count) {
		return;
	}

	int move_count = clients_count - index;
	if (move_count > 0) {
		memmove(&fds.cli[index], &fds.cli[index + 1], move_count * sizeof(struct pollfd));
	}

	memset(&fds.cli[clients_count], 0U, sizeof(struct pollfd));

	clients_count--;

	show_pfd();
}

static int srv_accept(int serv_sock, bool secure)
{
	int ret, sock;
	struct sockaddr_in addr;
	http_session_t *sess;
	socklen_t len = sizeof(struct sockaddr_in);

	uint32_t a = k_uptime_get();

	/* Make sure to clear the last authenticated user */
	handshake_auth_user = user_get_unauthenticated_user();

	sock = zsock_accept(serv_sock, (struct sockaddr *)&addr, &len);
	if (sock < 0) {
		stats.accept_failed++;
		LOG_ERR("(%d) Accept failed = %d", serv_sock, sock);
		ret = sock;
		goto exit;
	}

	ret = zsock_fcntl(sock, F_SETFL, SOCK_BLOCKING_OPT);
	if (ret < 0) {
		LOG_ERR("(%d) Failed to set socket non-blocking = %d", sock, ret);
		goto exit;
	}

	char ipv4_str[NET_IPV4_ADDR_LEN];
	ipv4_to_str(&addr.sin_addr, ipv4_str, sizeof(ipv4_str));

	LOG_DBG("(%d) Accepted session, cli sock = "
			"(%d)",
			serv_sock, sock);

	sess = http_session_alloc();
	if (sess == NULL) {
		stats.conn_alloc_failed++;
		LOG_WRN("(%d) Connection refused from %s:%d, cli sock = (%d)", serv_sock,
				ipv4_str, htons(addr.sin_port), sock);

		zsock_close(sock);

		ret = -1;
		goto exit;
	} else {
		LOG_INF("(%d) Connection accepted from %s:%d, cli sock = (%d)", serv_sock,
				ipv4_str, htons(addr.sin_port), sock);

		__ASSERT_NO_MSG(clients_count < CONFIG_APP_HTTP_MAX_SESSIONS);

		struct pollfd *pfd = &fds.cli[clients_count++];

		pfd->fd		= sock;
		pfd->events = POLLIN;

		/* reference session socket */
		sess->sock = sock;

		/* initialize keep-alive context */
		sess->keep_alive.timeout	   = KEEP_ALIVE_DEFAULT_TIMEOUT_MS;
		sess->keep_alive.last_activity = k_uptime_get_32();

		/* Mark session as secure if secure socket */
		sess->secure = secure;
		sess->auth	 = handshake_auth_user;
	}

	show_pfd();

	uint32_t b = k_uptime_get();

	LOG_DBG("Accept delay %u ms", b - a);

	stats.conn_opened_count++;

	return 0;
exit:
	stats.conn_open_failed++;
	return ret;
}

static void handle_active_sessions(void)
{
	uint8_t idx = 0;
	http_session_t *sess;

	/* We iterate over the session and check if there are
	 * any data, or if the session has timeout.
	 */
	while (idx < clients_count) {
		bool close = false;

		sess = http_session_get_by_sock(fds.cli[idx].fd);
		__ASSERT_NO_MSG(sess != NULL);

		if (fds.cli[idx].revents & POLLIN) {
			/* data available */
			close = process_request(sess) != true;
		} else if (fds.cli[idx].revents & (POLLHUP | POLLERR)) {
			/* Error or hangup detected on client connection -> unexpected
			 * close */
			close = true;
			LOG_WRN("(%d) Unexpected close", sess->sock);
		} else if (http_session_is_outdated(sess)) {
			/* Session has timed out */
			close = true;
			stats.conn_error_count++;
			LOG_WRN("(%d) Closing outdated session "
					"%p",
					sess->sock, sess);
		}

		/* Close the session, remove the socket from the
		 * pollfd array */
		if (close) {
			stats.conn_closed_count++;
			LOG_INF("(%d) Closing sock sess %p", sess->sock, sess);
			zsock_close(sess->sock);
			http_session_free(sess);
			remove_pollfd_by_index(idx);
			show_pfd();
		} else {
			idx++;
		}
	}
}

static void http_srv_thread(void *_a, void *_b, void *_c)
{
	ARG_UNUSED(_a);
	ARG_UNUSED(_b);
	ARG_UNUSED(_c);

	int ret, timeout;

	http_session_init();

	setup_sockets();

	for (;;) {
		show_pfd();

		timeout = http_session_time_to_next_outdated();

		ret = zsock_poll(fds.array, clients_count + servers_count, timeout);
		if (ret >= 0) {
#if defined(CONFIG_APP_HTTP_SERVER_NONSECURE)
			if (fds.srv.revents & POLLIN) {
				ret = srv_accept(fds.srv.fd, false);
			}
#endif /* CONFIG_APP_HTTP_SERVER_NONSECURE */

#if defined(CONFIG_APP_HTTP_SERVER_SECURE)
			if (fds.sec.revents & POLLIN) {
				ret = srv_accept(fds.sec.fd, true);
			}
#endif /* CONFIG_APP_HTTP_SERVER_SECURE */

			handle_active_sessions();
		} else {
			LOG_ERR("unexpected poll(%p, %d, %d) return value = %d", &fds,
					clients_count + servers_count, SYS_FOREVER_MS, errno);

			k_sleep(K_MSEC(5000));
		}
	}

#if defined(CONFIG_APP_HTTP_SERVER_NONSECURE)
	zsock_close(fds.srv.fd);
#endif /* CONFIG_APP_HTTP_SERVER_NONSECURE */

#if defined(CONFIG_APP_HTTP_SERVER_SECURE)
	zsock_close(fds.sec.fd);
#endif /* CONFIG_APP_HTTP_SERVER_SECURE */
}

static int sendall(int sock, char *buf, size_t len)
{
	int ret;
	size_t sent = 0;

	LOG_DBG("(%d, %p, %u)", sock, (void *)buf, len);

	while (sent < len) {
		ret = zsock_send(sock, &buf[sent], len - sent, 0U);
		if (ret > 0) {
			stats.tx += ret;
			sent += ret;
		} else {
			ret = -errno;
			if (ret == -EAGAIN) {
				stats.send_eagain++;
				LOG_INF("(%d) -EAGAIN", sock);
				continue;
			} else {
				LOG_ERR("(%d) ret == %d ???", sock, ret);
				stats.send_failed++;
				goto exit;
			}
		}
	}

	ret = sent;

exit:
	return ret;
}

static bool send_headers(http_session_t *sess)
{
	int ret;
	buffer_t buf;
	buffer_init(&buf, buffer_internal, sizeof(buffer_internal));
	http_response_t *const resp = sess->resp;

	ret = http_encode_status(&buf, resp->status_code);
	ret = http_encode_header_connection(&buf, sess->keep_alive.enabled);
	ret = http_encode_header_content_type(&buf, resp->content_type);
	if (resp->chunked == 1u) {
		ret = http_encode_header_transer_encoding_chunked(&buf);
	} else {
		ret = http_encode_header_content_length(&buf, resp->content_length);
	}
	ret = http_encode_header_end(&buf);

	/* Send custom headers */

	/* TODO check for 'ret' errors */

	ret = sendall(sess->sock, buf.data, buf.filling);
	if (ret >= 0) {
		resp->headers_sent += ret;
	} else {
		stats.headers_send_failed++;
		LOG_ERR("(%d) Failed to send headers = %d", sess->sock, ret);
	}

	return ret >= 0;
}

static void request_chunk_buf_cleanup(http_request_t *req)
{
	if (http_request_is_stream(req)) {
		req->chunk.id  = 0;
		req->chunk.len = 0;
		req->chunk.loc = NULL;
	}
}

static int sock_recv(int sock, char *buf, size_t len)
{
	int rc = zsock_recv(sock, buf, len, 0);
	LOG_DBG("(%d, %p, %d, 0) = %d", sock, (void *)buf, len, rc);

	if (rc == -EAGAIN) {
		stats.recv_eagain++;
		/* TODO find a way to return to wait for data */
		LOG_WRN("(%d) -EAGAIN = %d", sock, rc);
	} else if (rc < 0) {
		stats.recv_failed++;
		LOG_ERR("(%d) recv failed = %d", sock, rc);
	} else if (rc == 0) {
		LOG_INF("(%d) Connection closed by peer", sock);
		stats.recv_closed++;
	} else {
		stats.rx += rc;
	}

	return rc;
}

static bool handle_request(http_session_t *sess)
{
	ssize_t rc;
	http_request_t *const req = sess->req;

	char *p			  = buffer;
	ssize_t remaining = sizeof(buffer);

	while (req->complete == 0U) {
		if (remaining <= 0) {
			http_request_discard(req, HTTP_REQUEST_PAYLOAD_TOO_LARGE);

			/* Reset buffer */
			p		  = buffer;
			remaining = sizeof(buffer);
			LOG_WRN("(%d) Request payload too large, discarding", sess->sock);
		}

		rc = sock_recv(sess->sock, p, remaining);
		if (rc > 0) {
			if (http_request_parse_buf(req, p, rc) == false) {
				goto close;
			}

			/* If not streaming, we need to keep the data in
			 * the buffer for later processing.
			 */
			if (!req->streaming && !req->discarded) {
				p += rc;
				remaining -= rc;
			}
		} else if ((rc <= 0) && (rc != -EAGAIN)) {
			goto close;
		}
	}

	request_chunk_buf_cleanup(req);

	return true;

close:
	return false;
}

static bool send_buffer(http_session_t *sess)
{
	http_response_t *const resp = sess->resp;

	__ASSERT_NO_MSG(resp->chunked == 0u);

	int ret = sendall(sess->sock, resp->buffer.data, resp->buffer.filling);
	if (ret >= 0) {
		resp->payload_sent += ret;

		if (resp->payload_sent > resp->content_length) {
			LOG_ERR("(%d) Payload sent > content_length, %u > %u, "
					"closing",
					sess->sock, resp->payload_sent, resp->content_length);
			goto close;
		}
	} else {
		goto close;
	}

	return true;
close:
	return false;
}

static bool send_chunk(http_session_t *sess)
{
	int ret;
	http_response_t *const resp = sess->resp;

	__ASSERT_NO_MSG(resp->chunked == 1u);

	if (resp->buffer.filling == 0u) {
		/* Nothing to send */
		return true;
	}

	/* Prepare chunk header */
	char chunk_header[16];
	ret = snprintf(chunk_header, sizeof(chunk_header), "%x\r\n", resp->buffer.filling);
	if (ret < 0) {
		goto close;
	}

	/* Send chunk header */
	ret = sendall(sess->sock, chunk_header, ret);
	if (ret < 0) {
		goto close;
	}

	/* Send chunk data */
	ret = sendall(sess->sock, resp->buffer.data, resp->buffer.filling);
	if (ret >= 0) {
		resp->payload_sent += ret;
	} else {
		goto close;
	}

	/* Send end of chunk */
	ret = sendall(sess->sock, "\r\n", 2u);
	if (ret < 0) {
		goto close;
	}

	return true;
close:
	return false;
}

static int send_end_of_chunked_encoding(http_session_t *sess)
{
	__ASSERT_NO_MSG(sess->resp->chunked == 1u);

	/* Send end of chunk */
	return sendall(sess->sock, "0\r\n\r\n", 5u);
}

static int call_resp_handler(http_request_t *req, http_response_t *resp)
{
#if defined(CONFIG_APP_HTTP_TEST)
	/* Should always be called when the handler is called */
	http_test_run(&req->_test_ctx, req, resp, HTTP_TEST_HANDLER_RESP);
#endif /* CONFIG_APP_HTTP_TEST */

	http_handler_t resp_hdlr = route_get_resp_handler(req->route);

	__ASSERT_NO_MSG(resp_hdlr != NULL);

	const int ret = resp_hdlr(req, resp);
	if (ret >= 0) {
		resp->calls_count++;
	} else {
		stats.resp_handler_failed++;
	}

	return ret;
}

int http_call_req_handler(http_request_t *req)
{
#if defined(CONFIG_APP_HTTP_TEST)
	/* Should always be called when the handler is called */
	http_test_run(&req->_test_ctx, req, NULL, HTTP_TEST_HANDLER_REQ);
#endif /* CONFIG_APP_HTTP_TEST */

	const http_handler_t req_hdlr = route_get_req_handler(req->route);

	const int ret = req_hdlr(req, NULL);
	if (ret >= 0) {
		/* Reset payload buffer */
		req->payload.loc = NULL;
		req->payload.len = 0u;

		req->calls_count++;
	} else {
		stats.req_handler_failed++;
	}

	return ret;
}

static bool send_error_response(http_session_t *sess)
{
	http_response_t *const resp = sess->resp;

	__ASSERT_NO_MSG(sess->req->discarded == 1u);

	resp->content_length = 0;
	resp->content_type	 = HTTP_CONTENT_TYPE_TEXT_PLAIN;

	switch (sess->req->discard_reason) {
	case HTTP_REQUEST_ROUTE_UNKNOWN:
		resp->status_code = HTTP_STATUS_NOT_FOUND;
		strncpy(resp->buffer.data, "404 Not Found", resp->buffer.size);
		break;
	case HTTP_REQUEST_BAD:
		resp->status_code = HTTP_STATUS_BAD_REQUEST;
		strncpy(resp->buffer.data, "400 Bad Request", resp->buffer.size);
		break;
	case HTTP_REQUEST_ROUTE_NO_HANDLER:
	case HTTP_REQUEST_STREAMING_UNSUPPORTED:
		resp->status_code = HTTP_STATUS_NOT_IMPLEMENTED;
		strncpy(resp->buffer.data, "501 Not Implemented", resp->buffer.size);
		break;
	case HTTP_REQUEST_PAYLOAD_TOO_LARGE:
		resp->status_code = HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE;
		strncpy(resp->buffer.data, "413 Entity Too Large", resp->buffer.size);
		break;
	case HTTP_REQUEST_UNSECURE_ACCESS:
		resp->status_code = HTTP_STATUS_FORBIDDEN;
		strncpy(resp->buffer.data, "403 Forbidden", resp->buffer.size);
		break;
	case HTTP_REQUEST_PROCESSING_ERROR:
	default:
		resp->status_code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
		strncpy(resp->buffer.data, "500 Server Error", resp->buffer.size);
		break;
	}

	resp->buffer.filling = strnlen(resp->buffer.data, resp->buffer.size);
	resp->content_length = resp->buffer.filling;

	return send_headers(sess) && send_buffer(sess);
}

static bool send_response(http_session_t *sess)
{
	int ret;

	http_request_t *const req	= sess->req;
	http_response_t *const resp = sess->resp;

	if (req->discarded) {
		stats.req_discarded_count++;
		return send_error_response(sess);
	}

	resp->content_type = http_route_resp_default_content_type(req->route);

	do {
		/* Prepare the buffer for route handler call */
		buffer_reset(&resp->buffer);

		/* Mark as complete by default, can be deasserted by the
		 * application, to send more data.
		 */
		resp->complete = 1u;

		/* process request, prepare response */

		ret = call_resp_handler(req, resp);
		if (ret < 0) {
			stats.req_discarded_count++;
			http_request_discard(sess->req, HTTP_REQUEST_PROCESSING_ERROR);
			LOG_ERR("(%d) Request processing failed = %d", sess->sock, ret);

			if (!resp->headers_sent) {
				/* Headers already sent, cannot send proper error
				 * response, just close the connection */
				goto close;
			} else {
				/* It still time to send HTTP response */
				return send_error_response(sess);
			}
		}

		if (!resp->headers_sent) {
			if (http_code_has_payload(resp->status_code)) {
				/* If response handler is called a single time
				 * and content-length is not set then set it to
				 * the length of the buffer */
				if (resp->complete) {
					if (resp->content_length == 0u) {
						resp->content_length = resp->buffer.filling;
						LOG_DBG("(%d) Content-Length not "
								"configure, forced to %u",
								sess->sock, resp->content_length);
					} else if (resp->content_length != resp->buffer.filling) {
						LOG_ERR("(%d) Content-Length "
								"mismatch, expected %u (buffer "
								"filling), "
								"got %u",
								sess->sock, resp->content_length, resp->buffer.filling);
						stats.req_discarded_count++;
						http_request_discard(sess->req, HTTP_REQUEST_PROCESSING_ERROR);
						return send_error_response(sess);
					}
				}
			} else {
				resp->content_length = 0;
				resp->complete		 = 1u;
				resp->buffer.filling = 0u;
			}

			/* Headers sent after handler first call */
			if (!send_headers(sess)) goto close;
		}

		const bool success =
			http_response_is_chunked(resp) ? send_chunk(sess) : send_buffer(sess);
		if (!success) {
			goto close;
		}
	} while (resp->complete == 0U);

	/* End of chunked encoding */
	if (http_response_is_chunked(resp)) {
		ret = send_end_of_chunked_encoding(sess);
		if (ret <= 0) {
			goto close;
		}
	}

	return ret >= 0;

close:
	return false;
}

static bool process_request(http_session_t *sess)
{
	static http_request_t req;
	static http_response_t resp;

	http_request_init(&req);
	http_response_init(&resp);

	/* Buffer is shared between request and response */
	buffer_init(&resp.buffer, buffer, sizeof(buffer));

	sess->req  = &req;
	sess->resp = &resp;

	/* Forward secure flag to "request" structure (TODO ugly change this)*/
	sess->req->secure = sess->secure;

	/* Set where to copy URL */
	char url_copy[HTTP_URL_MAX_LEN];
	req._url_copy = url_copy;

	if (handle_request(sess) == false) {
		goto close;
	}

	/* We update the session keep_alive configuration
	 * based on the request. Before sending headers.
	 */
	sess->keep_alive.enabled = req.keep_alive;

	const bool success = send_response(sess);
	if (!success) {
		stats.conn_process_failed++;
		LOG_ERR("(%d) Processing failed", sess->sock);
		goto close;
	}

	LOG_INF("(%d) Req %s %s [%u B] -> Status %d [%u B] "
			"(keep-alive=%d)",
			sess->sock, http_method_str(req.method), url_copy, req.payload_len,
			resp.status_code, resp.payload_sent, sess->keep_alive.enabled);

	/* Update last activity time */
	if (sess->keep_alive.enabled) {
		stats.conn_keep_alive_count++;
		sess->keep_alive.last_activity = k_uptime_get_32();
		return true;
	}

close:
	return false;
}

void http_server_get_stats(struct http_stats *dest)
{
	memcpy(dest, &stats, sizeof(stats));
}