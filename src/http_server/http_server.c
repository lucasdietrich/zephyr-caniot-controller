#include "http_server.h"

#include <net/socket.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_config.h>

#include <net/http_parser.h>

#include <stdio.h>

#include "http_utils.h"
#include "http_conn.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_server, LOG_LEVEL_INF);

/*___________________________________________________________________________*/

#define HTTP_FD_INDEX   0

#define HTTP_PORT       80
#define HTTPS_PORT      443

K_THREAD_DEFINE(http_server, 0x1000, http_srv_thread,
                NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, 0);

/*___________________________________________________________________________*/


/* We use the same buffer for all connections,
 * each HTTP request should be parsed and processed immediately.
 *
 * Same buffer for RX and TX
 */
union {
        char request[0x1000];
        struct {
                char internal[0x200];
                char payload[0x800];
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
        struct pollfd array[MAX_CONNECTIONS + 1];
        struct {
                struct pollfd srv[1];
                struct pollfd cli[MAX_CONNECTIONS];
        };
} fds;

extern int conns_count;

extern const struct http_parser_settings settings;

struct pollfd *conn_get_pfd(struct connection *conn)
{
        return &fds.cli[conn_get_index(conn)];
}

int conn_get_sock(struct connection *conn)
{
        return conn_get_pfd(conn)->fd;
}

/*___________________________________________________________________________*/

int http_srv_setup_sockets(void)
{
        int sock, ret;
        struct sockaddr_in local = {
                .sin_family = AF_INET,
                .sin_port = htons(HTTP_PORT),
                .sin_addr = {
                        .s_addr = 0
                }
        };

        sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
                ret = sock;
                LOG_ERR("failed to create socket = %d", ret);
                goto exit;
        }

        ret = zsock_bind(sock, (const struct sockaddr *)&local,
                         sizeof(struct sockaddr_in));
        if (ret < 0) {
                LOG_ERR("failed to bind socket(%d) = %d", sock, ret);
                goto exit;
        }

        /* TODO adjust the backlog value */
        ret = zsock_listen(sock, 3);
        if (ret < 0) {
                LOG_ERR("failed to listen socket(%d) = %d", sock, ret);
                goto exit;
        }

        fds.srv->fd = sock;
        fds.srv->events = POLLIN;

        conns_count = 0;

exit:
        return ret;
}

static void compress_fds(uint_fast8_t index)
{
        if (index >= MAX_CONNECTIONS) {
                return;
        }
        int move_count = conns_count - index - 1;
        if (move_count > 0) {
                memmove(&fds.cli[index],
                        &fds.cli[index + 1],
                        move_count * sizeof(struct pollfd));
        }
}

void http_srv_thread(void *_a, void *_b, void *_c)
{
        ARG_UNUSED(_a);
        ARG_UNUSED(_b);
        ARG_UNUSED(_c);

        int ret;

        ret = http_srv_setup_sockets();

        for (;;) {
                ret = poll(fds.array, conns_count + 1, SYS_FOREVER_MS);
                if (ret > 0) {
                        if (fds.srv->revents & POLLIN) {
                                http_srv_accept(fds.srv->fd);

                                /* if poll returned for 1 event 
                                 * and conn accept, go to poll
                                 */
                                if (ret == 1) {
                                        continue;
                                }
                        }
                        
                        uint_fast8_t idx = 0;
                        while (idx < conns_count) {
                                if (fds.cli[idx].revents & POLLIN) {
                                        struct connection *conn =
                                                get_connection(idx);

                                        http_srv_handle_conn(conn);

                                        if (conn_is_closed(conn)) {
                                                compress_fds(idx);
                                                continue;
                                        }
                                }
                                idx++;
                        }
                } else {
                        LOG_ERR("unexpected poll(%p, %d, %d) return value",
                                &fds, conns_count + 1, SYS_FOREVER_MS);
                }
        }

        zsock_close(fds.srv->fd);
}

int http_srv_accept(int serv_sock)
{
        int ret, sock;
        struct sockaddr_in addr;
        struct connection *conn;
        socklen_t len = sizeof(struct sockaddr_in);

        uint32_t a = k_uptime_get();

        sock = zsock_accept(serv_sock, (struct sockaddr *)&addr, &len);
        if (sock < 0) {
                LOG_ERR("accept failed = %d", sock);
                ret = sock;
                goto exit;
        }

        char ipv4_str[NET_IPV4_ADDR_LEN];
        net_addr_ntop(AF_INET, &addr.sin_addr, ipv4_str, sizeof(ipv4_str));

        conn = alloc_connection();
        if (conn == NULL) {
                LOG_WRN("Connection refused from %s:%d",
                        log_strdup(ipv4_str), htons(addr.sin_port));
                
                zsock_close(sock);

                ret = -1;
                goto exit;
        } else {
                LOG_INF("(%d) Connection accepted from %s:%d", sock,
                        log_strdup(ipv4_str), htons(addr.sin_port));


                conn_get_pfd(conn)->fd = sock;
                conn_get_pfd(conn)->events = POLLIN;
        }

        uint32_t b = k_uptime_get();

        LOG_DBG("accept delay %u ms", b - a);

        return 0;
exit:
        return ret;
}

static int recv_request(struct connection *conn)
{
        size_t parsed;
        ssize_t rc;
        int sock = conn_get_sock(conn);
        struct http_request *req = conn->req;

        req->len = 0;

        for (;;)
        {
                int remaining = req->buffer.size - req->len;
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

                        if (conn->complete) {
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
                ret = zsock_send(sock, buf, len - sent, 0);
                if (ret < 0) {
                        if (ret == -EAGAIN) {
                                LOG_INF("-EAGAIN (%d)", sock);
                                continue;

                                goto exit;
                        }
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

void http_srv_handle_conn(struct connection *conn)
{
        /* initialized one time only !*/
        static struct http_request req = {
                .buffer = {
                        .buf = buffer.request,
                        .size = sizeof(buffer.request)
                }
        };
        static struct http_response resp = {
                .buf = buffer.response.payload,
        };

        const int sock = conn_get_sock(conn);

        conn->req = &req;
        conn->resp = &resp;

        /* reset req and resp values */
        conn->req->payload.loc = NULL;
        conn->req->payload.len = 0;

        conn->resp->len = 0,
        conn->resp->status_code = 200;

        conn->keep_alive = 0;
        conn->complete = 0;

        if (recv_request(conn) <= 0) {
                goto close;
        }

        if (http_srv_process_request(&req, &resp) < 0) {
                goto close;
        }

        if (http_srv_send_response(conn, &resp) < 0) {
                goto close;
        }

        if (conn->keep_alive) {
                LOG_INF("(%d) keeping connection %p alive", sock, conn);
                return;
        }

close:
        zsock_close(sock);
        free_connection(conn);
        LOG_INF("Closing sock (%d) conn %p", sock, conn);
}

int http_srv_send_response(struct connection *conn,
                           struct http_response *resp)
{
        int ret;
        int sock = conn_get_sock(conn);

        ret = encode_status_code(buffer.response.internal, 
                                 resp->status_code);
        if (ret < 0) {
                goto exit;
        }

        ret = sendall(sock, buffer.response.internal, ret);
        if (ret < 0) {
                goto exit;
        }

        /*
        ret = sendall(sock, resp->buf, resp->len);
        if (ret < 0) {
                goto exit;
        }
        */

        return 0;
exit:
        return ret;
}

int http_srv_process_request(struct http_request *req,
                             struct http_response *resp)
{
        LOG_INF("loc = %p len = %u", req->payload.loc, req->payload.len);
        
        // LOG_HEXDUMP_INF(req->payload.loc, req->payload.len, "Payload : ");
        
        resp->len = 0;
        resp->status_code = 200;

        return 0;
}

/*___________________________________________________________________________*/
