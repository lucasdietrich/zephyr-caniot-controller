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
LOG_MODULE_REGISTER(http_server, LOG_LEVEL_INF);

/*___________________________________________________________________________*/

#define HTTP_FD_INDEX   0

#define HTTP_PORT       80
#define HTTPS_PORT      443

#define HTTPS_SERVER_SEC_TAG   1

#if CONFIG_CONTROLLER_HTTP_SERVER_NONSECURE
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
union {
	char request[0x1000];
	struct {
		char internal[0x200];
		char payload[0xe00];
	} response;
} buffer;

/**
 * @brief 
 * - 1 TCP socket for HTTP
 * - 1 TLS socket for HTTPS
 * - 3 client sockets
 */
static union
{
        struct pollfd array[CONFIG_CONTROLLER_MAX_HTTP_CONNECTIONS + SERVER_FD_COUNT];
        struct {
#if CONFIG_CONTROLLER_HTTP_SERVER_NONSECURE
                struct pollfd srv;      /* non secure server socket */
#endif
                struct pollfd sec;      /* secure server socket */
                struct pollfd cli[CONFIG_CONTROLLER_MAX_HTTP_CONNECTIONS];
        };
} fds;

static int listening_count = 0;
extern int conns_count;

extern const struct http_parser_settings settings;

struct pollfd *conn_get_pfd(http_connection_t *conn)
{
        return &fds.cli[http_conn_get_index(conn)];
}

int conn_get_sock(http_connection_t *conn)
{
        return conn_get_pfd(conn)->fd;
}

/* debug functions */
static void show_pfd(void)
{
        LOG_DBG("listening_count=%d conns_count=%d", listening_count, conns_count);
        for (struct pollfd *pfd = fds.array;
             pfd < fds.array + ARRAY_SIZE(fds.array); pfd++)
        {
                LOG_DBG("\tfd=%d ev=%d", pfd->fd, (int)pfd->events);
        }
}

/*___________________________________________________________________________*/

// forward declarations 
static void handle_conn(http_connection_t *conn);

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
#if CONFIG_CONTROLLER_HTTP_SERVER_NONSECURE
        if (setup_socket(&fds.srv, false) < 0) {
                goto exit;
        }
#endif /* CONFIG_CONTROLLER_HTTP_SERVER_NONSECURE */

        /* setup secure HTTPS socket (port 443) */

        /* include this PR : https://github.com/zephyrproject-rtos/zephyr/pull/40255
         * related issue : https://github.com/zephyrproject-rtos/zephyr/issues/40267
         */
        tls_credential_add(
                HTTPS_SERVER_SEC_TAG, TLS_CREDENTIAL_SERVER_CERTIFICATE,
                x509_public_certificate_rsa1024_der, sizeof(x509_public_certificate_rsa1024_der));
        tls_credential_add(
                HTTPS_SERVER_SEC_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
                rsa_private_key_rsa1024_der, sizeof(rsa_private_key_rsa1024_der));

        if (setup_socket(&fds.sec, true) < 0) {
                goto exit;
        }

        conns_count = 0;
exit:
        return -1;
}

static void remove_pollfd_by_index(uint_fast8_t index)
{
        LOG_DBG("Compress fds count = %u", conns_count);

        // show_pfd();

        if (index >= CONFIG_CONTROLLER_MAX_HTTP_CONNECTIONS) {
                return;
        }
        int move_count = conns_count - index;
        if (move_count > 0) {
                memmove(&fds.cli[index],
                        &fds.cli[index + 1],
                        move_count * sizeof(struct pollfd));
        }

        memset(&fds.cli[conns_count], 0U,
               sizeof(struct pollfd));

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


                conn_get_pfd(conn)->fd = sock;
                conn_get_pfd(conn)->events = POLLIN;

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

        /* initialize connection pool */
        http_conn_init_pool();

        ret = setup_sockets();

        for (;;) {
                show_pfd();

		timeout = http_conn_get_duration_to_next_outdated_conn();
		LOG_DBG("zsock_poll timeout: %d ms", timeout);

                ret = zsock_poll(fds.array, conns_count + listening_count, timeout);
                if (ret >= 0) {
#if CONFIG_CONTROLLER_HTTP_SERVER_NONSECURE
                        if (fds.srv.revents & POLLIN) {
                                ret = srv_accept(fds.srv.fd);
                                if(ret != 0) {
                                        LOG_ERR("(%d) srv_accept failed", ret);
                                }
                        }
#endif /* CONFIG_CONTROLLER_HTTP_SERVER_NONSECURE */

                        if (fds.sec.revents & POLLIN) {
                                ret = srv_accept(fds.sec.fd);
                                if(ret != 0) {
                                        LOG_ERR("(%d) srv_accept failed", ret);
                                }
                        }

                        /* We iterate over the connections and check if there are any data,
			 * or if the connection has timeout.
                         */
                        uint_fast8_t idx = 0;
                        while (idx < conns_count) {
				http_connection_t *conn = http_conn_get(idx);
				
				if (fds.cli[idx].revents & POLLIN) { /* data available */
					handle_conn(conn);
				} else if (http_conn_is_outdated(conn)) { /* check if the connection has timed out */
					const int sock = conn_get_sock(conn);
					zsock_close(sock);
					http_conn_free(conn);
					LOG_WRN("(%d) Closing outdated connection %p", sock, conn);
				}

				/* if connection was closed, remove the socket from the pollfd array */
				if (http_conn_closed(conn)) {
					remove_pollfd_by_index(idx);
					show_pfd();
					continue;
				} else {
					idx++;
				}
                        }
                } else {
                        LOG_ERR("unexpected poll(%p, %d, %d) return value = %d",
                                &fds, conns_count + listening_count, SYS_FOREVER_MS, ret);
                        
                        /* TODO remove, sleep 1 sec here */
                        k_sleep(K_MSEC(5000));
                }
        }

#if CONFIG_CONTROLLER_HTTP_SERVER_NONSECURE
        zsock_close(fds.srv.fd);
#endif /* CONFIG_CONTROLLER_HTTP_SERVER_NONSECURE */

        zsock_close(fds.sec.fd);
}

static int recv_request(http_connection_t *conn)
{
        size_t parsed;
        ssize_t rc;
        int sock = conn_get_sock(conn);
        struct http_request *req = conn->req;
        int remaining = req->buffer.size;

        req->len = 0;

        for (;;)
        {
                if (remaining <= 0) {
                        LOG_WRN("(%d) Recv buffer full, closing connection ...", 
                                sock);
                        rc = -ENOMEM;
                        goto exit;
                }

                rc = zsock_recv(sock, &req->buffer.buf[req->len],
                                remaining, 0);
                if (rc < 0) {
                        if (rc == -EAGAIN) {
                                LOG_WRN("-EAGAIN = %d", -EAGAIN);
                                continue;
                        }

                        LOG_ERR("recv failed = %d", rc);
                        goto exit;
                } else if (rc == 0) {
                        LOG_INF("(%d) Connection closed by peer", sock);
                        goto exit;
                } else {
                        parsed = http_parser_execute(&conn->parser,
                                                     &settings,
                                                     &req->buffer.buf[req->len],
                                                     rc);

                        req->len += rc;
                        remaining -= rc;

                        if (conn->complete) {
				/* if request is complete, we only expect a following request */
                                break;
                        }
                }
        }

        return req->len;
exit:
        return rc;
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
// 				   struct http_response *resp)
// {
// 	return 0;
// }

static int send_response(http_connection_t *conn,
                           struct http_response *resp)
{
        int ret, sent;
        char *b = buffer.response.internal;
        const size_t buf_size = sizeof(buffer.response.internal);
        int encoded = 0;

        int sock = conn_get_sock(conn);

        ret = http_encode_status(b + encoded, buf_size - encoded,
                                 resp->status_code);

        encoded += ret;
        ret = http_encode_header_connection(b + encoded, buf_size - encoded,
                                            (bool)  conn->keep_alive.enabled);

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
        ret = sendall(sock, b, encoded);
        if (ret < 0) {
                goto exit;
        }

        sent = ret;

        /* send body */
        if (http_code_has_payload(resp->status_code)) {
                ret = sendall(sock, resp->buf, resp->content_len);
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

static int process_request(struct http_request *req,
			   struct http_response *resp)
{
	int ret;
	const struct http_route *route = route_resolve(req);

	if (route != NULL) {
		/* set default content type in function of the route */
		resp->content_type = http_get_route_default_content_type(route);

		if (route->handler != NULL) {
			ret = route->handler(req, resp);
			if (ret != 0) {
				LOG_ERR("Handler failed: err = %d", ret);

				/* encode HTTP internal server error */
				resp->status_code = 500U;
			}
		} else {
			LOG_ERR("No handler for route %s", req->url);

			/* Not Found */
			resp->status_code = 404U;
		}
	} else {
		LOG_WRN("No route found for %s", req->url);

		/* Not Found */
		resp->status_code = 404U;
	}

	/* post check on payload to send */
	return 0;
}

static void handle_conn(http_connection_t *conn)
{
        /* initialized one time only !*/
        static struct http_request req = {
                .buffer = {
                        .buf = buffer.request,
                        .size = sizeof(buffer.request)
                },
        };
        static struct http_response resp = {
                .buf = buffer.response.payload,
                .buf_size = sizeof(buffer.response.payload)
        };

        const int sock = conn_get_sock(conn);

        conn->req = &req;
        conn->resp = &resp;

        /* reset req and resp values */
        req.payload.loc = NULL;
        req.payload.len = 0;

        resp.content_len = 0,
        resp.status_code = 200;
	resp.content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN;

        conn->keep_alive.enabled = 0;
        conn->complete = 0;

        if (recv_request(conn) <= 0) {
                goto close;
        }

        if (process_request(&req, &resp) != 0) {
                goto close;
        }

	/* in prepare response set content-type, length, ... */
	// if (encode_response_headers(&req, &resp) != 0) {
	// 	goto close;
	// }

	int sent = send_response(conn, &resp);
        if (sent < 0) {
                goto close;
        }

	LOG_INF("(%d) Processing req total len %u B resp status %d len %u B (keep"
		"-alive = %d)", sock, req.len, resp.status_code,
		sent, conn->keep_alive.enabled);

        if (conn->keep_alive.enabled) {
		conn->keep_alive.last_activity = k_uptime_get_32();
                return;
        }

close:
        zsock_close(sock);
        http_conn_free(conn);
        LOG_INF("(%d) Closing sock conn %p", sock, conn);
}

/*___________________________________________________________________________*/