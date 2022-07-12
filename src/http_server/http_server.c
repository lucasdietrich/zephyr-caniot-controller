#include "http_server.h"

#include <stdio.h>

#include <net/socket.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_config.h>
#include <net/tls_credentials.h>

#include <net/http_parser.h>

#include "http_utils.h"
#include "http_conn.h"
#include "routes.h"
#include "utils.h"

#include "creds/credentials.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_server, LOG_LEVEL_INF); /* INF */

/*___________________________________________________________________________*/

#define HTTP_FD_INDEX   0

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

/*___________________________________________________________________________*/

K_THREAD_DEFINE(http_server, 0x1000, http_srv_thread,
		NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, 0);

/*___________________________________________________________________________*/


/* We use the same buffer for all connections,
 * each HTTP request should be parsed and processed immediately.
 *
 * Same buffer for HTTP request and HTTP response
 */
__noinit char buffer[0x3000];
__noinit char buffer_internal[0x800]; /* For encoding response headers */

/**
 * @brief
 * - 1 TCP socket for HTTP
 * - 1 TLS socket for HTTPS
 * - 3 client sockets
 */
static union
{
	struct pollfd array[CONFIG_MAX_HTTP_CONNECTIONS + SERVER_FD_COUNT];
	struct {
#if CONFIG_HTTP_SERVER_NONSECURE
		struct pollfd srv;      /* non secure server socket */
#endif
		struct pollfd sec;      /* secure server socket */
		struct pollfd cli[CONFIG_MAX_HTTP_CONNECTIONS];
	};
} fds;

static int listening_count = 0;
static int clients_count = 0;

extern const struct http_parser_settings parser_settings;

/* debug functions */
static void show_pfd(void)
{
	LOG_DBG("listening_count=%d clients_count=%d", listening_count, clients_count);
	for (struct pollfd *pfd = fds.array;
	     pfd < fds.array + ARRAY_SIZE(fds.array); pfd++) {
		LOG_DBG("\tfd=%d ev=%d", pfd->fd, (int)pfd->events);
	}
}

/*___________________________________________________________________________*/

// forward declarations 
static void handle_request(http_connection_t *conn);

/*___________________________________________________________________________*/

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

	listening_count++;

	return sock;
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

		__ASSERT_NO_MSG(clients_count < CONFIG_MAX_HTTP_CONNECTIONS);

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

void http_srv_thread(void *_a, void *_b, void *_c)
{
	ARG_UNUSED(_a);
	ARG_UNUSED(_b);
	ARG_UNUSED(_c);

	int ret, timeout;

	http_conn_init();

	ret = setup_sockets();

	for (;;) {
		show_pfd();

		timeout = http_conn_get_duration_to_next_outdated_conn();
		LOG_DBG("zsock_poll timeout: %d ms", timeout);

		ret = zsock_poll(fds.array, clients_count + listening_count, timeout);
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

				if (fds.cli[idx].revents & POLLIN) { /* data available */
					handle_request(conn);
				} else if (http_conn_is_outdated(conn)) { /* check if the connection has timed out */
					zsock_close(conn->sock);
					http_conn_free(conn);
					LOG_WRN("(%d) Closing outdated connection %p", conn->sock, conn);
				}

				/* if connection is closed, remove the socket from the pollfd array */
				if (http_conn_is_closed(conn)) {
					remove_pollfd_by_index(idx);
					show_pfd();
					continue;
				} else {
					idx++;
				}
			}
		} else {
			LOG_ERR("unexpected poll(%p, %d, %d) return value = %d",
				&fds, clients_count + listening_count, SYS_FOREVER_MS, ret);

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

	while (sent < len) {
		ret = zsock_send(sock, &buf[sent], len - sent, 0);
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

	return sent;

exit:
	return ret;
}

// static int encode_response_headers(http_connection_t *conn,
// 				   http_response_t *resp)
// {
// 	return 0;
// }

static int send_response(http_connection_t *conn,
			 http_response_t *resp)
{
	int ret, sent;
	char *b = buffer_internal;
	const size_t buf_size = sizeof(buffer_internal);
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
						resp->content_len);

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
		ret = sendall(conn->sock, resp->buf, resp->content_len);
		if (ret < 0) {
			goto exit;
		}
		sent += ret;
	} else if (resp->content_len) {
		LOG_WRN("Trying to send a content for invalid response "
			"code (%d != 200)", resp->status_code);
	}

	return sent;
exit:
	return ret;
}

static int process_chunk(http_request_t *req,
			   http_response_t *resp)
{
	const http_route_t *const route = req->route; /* Route should be set at this point */

	/* Check if the route is set */
	if (route == NULL) {
		LOG_WRN("No route found for %s %s",
			log_strdup(http_method_str(req->method)), log_strdup(req->url));
		resp->status_code = 404U;
		goto exit;
	}

	/* Check if handler is set */
	if (route->handler == NULL) {
		LOG_ERR("No handler for route %s", log_strdup(req->url));
		resp->status_code = 404U;
		goto exit;
	}

	/* set default content type in function of the route */
	resp->content_type = http_route_get_default_content_type(route);

	/* call handler */
	int ret = route->handler(req, resp);
	if (ret != 0) {
		LOG_ERR("Handler failed: err = %d", ret);

		/* encode HTTP internal server error */
		resp->status_code = 500U;
	}

exit:
	/* post check on payload to send */
	return 0;
}

static void init_request(http_request_t *req)
{
	static http_route_args_t route_args;

	memset(req, 0, sizeof(http_request_t));

	req->route_args = &route_args;

	req->chunk.loc = NULL;
	req->chunk.len = 0U;
	req->chunk.id = 0U;

	req->keep_alive = 0U;
	req->timeout_ms = 0U;
	req->chunked = 0U;
	req->content_type = HTTP_CONTENT_TYPE_NONE;

	req->request_complete = 0U;
	req->headers_complete = 0U;
	req->discard = 0U;
	req->stream = 0U;

	req->payload.len = 0U;
	req->payload.loc = NULL;

	req->parsed_content_length = 0U;

	http_parser_init(&req->parser, HTTP_REQUEST);
}

static void init_response(http_response_t *resp)
{
	memset(resp, 0, sizeof(http_response_t));

	resp->buf = buffer;
	resp->buf_size = sizeof(buffer);

	/* default response */
	resp->content_len = 0U,
		resp->status_code = 200U;
	resp->content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN;
}

static void handle_request(http_connection_t *conn)
{
	static http_route_args_t route_args;
	static http_request_t req;
	static http_response_t resp;

	init_request(&req);
	init_response(&resp);

	const int sock = conn->sock;
	req._conn = conn;

	conn->req = &req;
	conn->resp = &resp;

	/* reset keep alive flag */
	conn->keep_alive.enabled = 0U;
	
	while (conn->req->request_complete == 0U) {

		uint8_t *buf = buffer;
		size_t buf_remaining = sizeof(buffer);

		size_t received = 0U;
		size_t parsed = 0U;

		ssize_t rc;

		for (;;) {
			if (buf_remaining <= 0) {
				LOG_WRN("(%d) Recv buffer full, closing connection ...",
					sock);
				rc = -ENOMEM;
				goto close;
			}

			rc = zsock_recv(sock, &buffer[received], buf_remaining, 0);
			if (rc < 0) {
				if (rc == -EAGAIN) {
					LOG_WRN("-EAGAIN = %d", -EAGAIN);
					continue;
				}

				LOG_ERR("recv failed = %d", rc);
				goto close;
			} else if (rc == 0) {
				LOG_INF("(%d) Connection closed by peer", sock);
				goto close;
			} else {
				parsed = http_parser_execute(&req.parser,
							     &parser_settings,
							     &buffer[received],
							     rc);

				if (req.stream || req.discard) {
					/* reset buffer as the stream handler has
					 * already consumed the data */
					buf = buffer;
					buf_remaining = sizeof(buffer);
				}

				if (req.headers_complete && req.discard) {
					/* TODO, properly discard the request
					 * without closing the connection
					 */
					LOG_WRN("(%d) Discarding request (close - TODO -> 404)", sock);
					rc = 0;
					goto close;
				} else if (req.request_complete) {
					/* We update the connection keep_alive configuration
					 * based on the request.
					 */
					conn->keep_alive.enabled = req.keep_alive;

					break;
				}
			}
		}
	}
	
	// if (recv_request(conn) <= 0) { /* Including 0 is important ! */
	// 	goto close;
	// }

	if (conn->req->stream == 0U) {
		if (conn->req->payload.len == conn->req->parsed_content_length) {
			LOG_INF("Content-length = %d", conn->req->parsed_content_length);
		} else {
			LOG_ERR("actually rcv length = %u / %u content-length header",
				conn->req->payload.len, conn->req->parsed_content_length);
		}
	}

	// if (process_chunk(&req, &resp) != 0) {
	// 	goto close;
	// }

	/* in prepare response set content-type, length, ... */
	// if (encode_response_headers(&req, &resp) != 0) {
	// 	goto close;
	// }

	int sent = send_response(conn, &resp);
	if (sent < 0) {
		goto close;
	}

	LOG_INF("(%d) Processing req total len %u B status %d len %u B (keep"
		"-alive=%d)", sock, req.payload.len, resp.status_code,
		sent, conn->keep_alive.enabled);

	if (conn->keep_alive.enabled) {
		conn->keep_alive.last_activity = k_uptime_get_32();
		return;
	}

close:
	zsock_close(conn->sock);
	http_conn_free(conn);
	LOG_INF("(%d) Closing sock conn %p", sock, conn);
}

/*___________________________________________________________________________*/