#include "http_server.h"

#include <net/socket.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_config.h>

#define server_init                     http_server_init
#define setup_sockets                   http_server_setup_sockets
#define accept_connection               http_server_accept_connection
#define handle_connection               http_server_handle_connection

#define CLIENTS_COUNT   3

#define HTTP_FD_INDEX   0
// #define HTTPS_FD_INDEX   1
#define CLI1_FD_INDEX   1
#define CLI2_FD_INDEX   CLI2_FD_INDEX + 1
#define CLI3_FD_INDEX   CLI3_FD_INDEX + 1

#define HTTP_PORT       80
#define HTTPS_PORT      443

#include <logging/log.h>
LOG_MODULE_REGISTER(http_server, LOG_LEVEL_DBG);

K_THREAD_DEFINE(http_server, 0x1000, http_server_thread,
                NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, 0);

/**
 * @brief 
 * - 1 TCP socket for HTTP
 * - 1 TLS socket for HTTPS
 * - 3 client sockets
 */
int fds_count;
static struct pollfd fds[1 + CLIENTS_COUNT];
struct connection connections[CLIENTS_COUNT];

int http_server_init(void)
{
        int ret;

        ret = http_server_setup_sockets();
        if (ret == 0) {
                k_thread_start(http_server);
        }

        return ret;
}

int http_server_setup_sockets(void)
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
        fds_count = 1;

exit:
        return ret;
}

void http_server_thread(void *_a, void *_b, void *_c)
{
        int ret;
        struct pollfd *pfd;

        for (;;) {
                ret = poll(fds, fds_count, SYS_FOREVER_MS);
                if (ret > 0) {
                        if (fds[HTTP_FD_INDEX].revents & POLLIN) {
                                accept_connection(pfd, NULL);
                        }

                        for (pfd = &fds[1]; pfd <= &fds[ARRAY_SIZE(fds) - 1]; pfd++) {
                                handle_connection(pfd, NULL);
                        }
                } else {
                        LOG_ERR("unexpected poll(%p, %d, %d) return value",
                                fds, fds_count, SYS_FOREVER_MS);
                }
        }
}

int http_server_accept_connection(struct pollfd *pfd, struct connection *conn)
{
        int ret, sock;
        struct sockaddr addr;
        socklen_t len;

        sock = zsock_accept(pfd->fd, &addr, &len);
        if (ret < 0) {
                LOG_ERR("accept failed = %d", sock);
                ret = sock;
                goto exit;
        }

        LOG_ERR("unimplemented %d ...", ret);
        zsock_close(sock);

        return 0;
        
exit:
        return ret;
}

int http_server_handle_connection(struct pollfd *pfd, struct connection *conn)
{
        return 0;
}