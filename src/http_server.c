#include "http_server.h"

#include <net/socket.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_config.h>

#include <net/http_parser.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(http_server, LOG_LEVEL_DBG);

/*___________________________________________________________________________*/

#define HTTP_FD_INDEX   0

#define HTTP_PORT       80
#define HTTPS_PORT      443

K_THREAD_DEFINE(http_server, 0x1000, http_srv_thread,
                NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, 0);

/*___________________________________________________________________________*/

int on_message_begin(struct http_parser *parser)
{
        LOG_INF("on_message_begin (%d)", 0);
        return 0;
}

int on_url(struct http_parser *parser, const char *at, size_t length)
{
        LOG_INF("on_url at=%p len=%u", at, length);
        LOG_HEXDUMP_DBG(at, length, "");
        return 0;
}

int on_status(struct http_parser *parser, const char *at, size_t length)
{
        LOG_INF("on_status at=%p len=%u", at, length);
        LOG_HEXDUMP_DBG(at, length, "");
        return 0;
}

int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
        LOG_INF("on_header_field at=%p len=%u", at, length);
        LOG_HEXDUMP_DBG(at, length, "");
        return 0;
}

int on_header_value(struct http_parser *parser, const char *at, size_t length)
{
        LOG_INF("on_header_value at=%p len=%u", at, length);
        LOG_HEXDUMP_DBG(at, length, "");
        return 0;
}

int on_headers_complete(struct http_parser *parser)
{
        LOG_INF("on_headers_complete (%d)", 0);
        return 0;
}

int on_body(struct http_parser *parser, const char *at, size_t length)
{
        LOG_INF("on_body at=%p len=%u", at, length);
        LOG_HEXDUMP_DBG(at, length, "");
        return 0;
}

int on_message_complete(struct http_parser *parser)
{
        LOG_INF("on_message_complete (%d)", 0);
        return 0;
}

int on_chunk_header(struct http_parser *parser)
{
        LOG_INF("on_chunk_header (%d)", 0);
        return 0;
}

int on_chunk_complete(struct http_parser *parser)
{
        LOG_INF("on_chunk_complete (%d)", 0);
        return 0;
}

const struct http_parser_settings settings = {
        .on_status = on_status,
        .on_url = on_url,
        .on_header_field = on_header_field,
        .on_header_value = on_header_value,
        .on_headers_complete = on_headers_complete,
        .on_message_begin = on_message_begin,
        .on_message_complete = on_message_complete,
        .on_body = on_body,

        .on_chunk_header = on_chunk_header,
        .on_chunk_complete = on_chunk_complete
};

/*___________________________________________________________________________*/

#define MAX_CONNECTIONS   3

/**
 * @brief 
 * - 1 TCP socket for HTTP
 * - 1 TLS socket for HTTPS
 * - 3 client sockets
 */
int conns_count;
static struct pollfd fds[1 + MAX_CONNECTIONS];

struct connection connections[MAX_CONNECTIONS];

uint8_t buffer[0x1000];

struct connection *get_connection(int index)
{
        __ASSERT((0 <= index) && (index < MAX_CONNECTIONS), "index out of range");

        return &connections[index];
}

inline int conn_get_index(struct connection *conn)
{
        return conn - connections;
}

struct connection *alloc_connection(void)
{
        if (conns_count < MAX_CONNECTIONS) {
                return &connections[conns_count++];
        }

        return NULL;
}

void free_connection(struct connection *conn)
{
        if (conn == NULL) {
                return;
        }

        int index = conn_get_index(conn);
        int move_count = conns_count - index - 1;
        if (move_count > 0) {
                memmove(&connections[index],
                        &connections[index + 1],
                        move_count);
        }
        conns_count--;
}

static void reset_conn(struct connection *conn)
{
        memset(conn, 0x00, sizeof(struct connection));
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

        fds[HTTP_FD_INDEX].fd = sock;
        fds[HTTP_FD_INDEX].events = POLLIN;
        conns_count = 0;

exit:
        return ret;
}

void http_srv_thread(void *_a, void *_b, void *_c)
{
        ARG_UNUSED(_a);
        ARG_UNUSED(_b);
        ARG_UNUSED(_c);

        int rc, ret;

        ret = http_srv_setup_sockets();

        for (;;) {
                LOG_INF("polling %d events", conns_count + 1);
                ret = poll(fds, conns_count + 1, SYS_FOREVER_MS);
                LOG_INF("poll returned = %d", ret);
                if (ret > 0) {
                        if (fds[HTTP_FD_INDEX].revents & POLLIN) {
                                http_srv_accept(fds[HTTP_FD_INDEX].fd);
                        }

                        for (uint_fast8_t i = 0; i < conns_count; i++) {
                                LOG_DBG("fds[%d].fd = %d", i, fds[i].fd);
                                if (fds[i + 1].revents & POLLIN) {
                                        rc = http_srv_handle_conn(fds[i + 1].fd,
                                                                  &connections[i]);
                                }
                        }
                } else {
                        LOG_ERR("unexpected poll(%p, %d, %d) return value",
                                fds, conns_count + 1, SYS_FOREVER_MS);
                }
        }

        zsock_close(fds[0].fd);
}

int http_srv_accept(int serv_sock)
{
        int ret, sock;
        struct sockaddr_in addr;
        struct connection *conn;
        socklen_t len = sizeof(struct sockaddr_in);

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
                LOG_INF("Connection accepted from %s:%d",
                        log_strdup(ipv4_str), htons(addr.sin_port));
                
                http_parser_init(&conn->parser, HTTP_REQUEST);

                const int index = conn_get_index(conn);
                fds[index + 1].fd = sock;
                fds[index + 1].events = POLLIN;
        }

        return 0;
exit:
        return ret;
}

int http_srv_handle_conn(int sock, struct connection *conn)
{
        size_t parsed;
        ssize_t rc;
        const size_t buf_len = sizeof(buffer);
        ssize_t received = 0;

        for (;;)
        {
                rc = zsock_recv(sock, buffer, buf_len - received, 0);
                LOG_DBG("recv(%d,,%d,) = %d", sock, buf_len - received, rc);
                if (rc < 0) {
                        if (rc == -EAGAIN) {
                                LOG_WRN("-EAGAIN = %d", -EAGAIN);
                                continue;
                        }

                        LOG_ERR("recv failed = %d", rc);
                        goto exit;
                } else if (rc == 0) {
                        LOG_INF("(%d) Connection closed", sock);
                        goto exit;
                } else {
                        parsed = http_parser_execute(&conn->parser, &settings,
                                                     &buffer[received], rc);
                        LOG_INF("http_parser_execute(,,received=%d, len=%d) = %d (parsed)", 
                                received, rc, parsed);

                        received += rc;
                }
        }

        return received;
exit:
        zsock_close (sock);
        free_connection(conn);
        return rc;
}

// int http_srv_handle_conn(struct pollfd *pfd, struct connection *conn)
// {
//         return 0;
// }

/*___________________________________________________________________________*/
