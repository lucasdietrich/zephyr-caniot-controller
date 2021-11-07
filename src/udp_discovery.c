#include "udp_discovery.h"

#include <net/socket.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_config.h>
#include <net/udp.h>

#include <sys/types.h>

#include "utils.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(discovery, LOG_LEVEL_DBG);

#define DISCOVERY_PORT          5000
#define SEARCH_STRING           "Search caniot-controller"
#define SEARCH_STRING_LEN       (sizeof("Search caniot-controller") - 1)

static int fd;

static void discovery_thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(discovery, 0x300, discovery_thread, NULL, NULL, NULL,
                K_PRIO_PREEMPT(8), 0, SYS_FOREVER_MS);

static int setup_socket(void)
{
        int ret;

        struct sockaddr_in addr = {
                .sin_family = AF_INET,
                .sin_port = htons(DISCOVERY_PORT),
                .sin_addr = {
                        .s_addr = 0
                }
        };

        fd = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (fd < 0) {
                LOG_ERR("failed to create socket = %d", fd);
                ret = fd;
                goto exit;
        }

        ret = zsock_bind(fd, (const struct sockaddr *)&addr,
                         sizeof(struct sockaddr_in));
        if (ret < 0) {
                LOG_ERR("failed to bind socket = %d", fd);
                goto exit;
        }

        ret = zsock_listen(fd, 1);
        if (ret < 0) {
                LOG_ERR("failed to listen socket = %d", fd);
                goto exit;
        }

        return fd;

exit:
        return ret;
}

static int prepare_reponse(struct discovery_response *resp, size_t len)
{
        const size_t resp_len = sizeof(struct discovery_response);
        if (len < resp_len) {
                LOG_ERR("buffer too small %d < %u", len, resp_len);
                return -EINVAL;
        }

        struct net_if *iface = net_if_get_default();
        struct in_addr *addr = &iface->config.ip.ipv4->unicast[0].address.in_addr;

        resp->ip = htonl(addr->s_addr);
        ipv4_to_str(addr, resp->str_ip, sizeof(resp->str_ip));

        return resp_len;
}

static char buffer[0x20];

/* addr support for IPV6 */
static void discovery_thread(void *_a, void *_b, void *_c)
{
        ARG_UNUSED(_a);
        ARG_UNUSED(_b);
        ARG_UNUSED(_c);

        ssize_t rc;
        struct sockaddr_in client;
        socklen_t addrlen = sizeof(struct sockaddr_in);

        for(;;) {
                /* recv */
                rc = zsock_recvfrom(fd, buffer, sizeof(buffer), 0,
                                    (struct sockaddr *)&client,
                                    &addrlen);
                if (rc < 0) {
                        LOG_ERR("failed to recvfrom socket = %d", rc);
                        continue;
                }

                /* display client ip:port */
                char ipv4_str[NET_IPV4_ADDR_LEN];
                if (net_addr_ntop(AF_INET, &client.sin_addr,
                                  ipv4_str, sizeof(ipv4_str)) != NULL) {
                        LOG_INF("Processing UDP packet from %s:%d",
                                log_strdup(ipv4_str), htons(client.sin_port));
                }

                LOG_HEXDUMP_DBG(buffer, rc, "UDP query received");

                if (strncmp(buffer, SEARCH_STRING,
                            MIN(rc, SEARCH_STRING_LEN)) != 0) {
                        LOG_DBG("query ignored %d", rc);
                        continue;
                }

                size_t tosend = prepare_reponse(
                        (struct discovery_response *)buffer, sizeof(buffer));
                if (tosend <= 0) {
                        continue;
                }

                rc = zsock_sendto(fd, buffer, tosend, 0,
                                  (const struct sockaddr *)&client, addrlen);
                if (rc < 0) {
                        LOG_ERR("failed to sendto = %d", rc);
                        continue;
                }

                LOG_HEXDUMP_DBG(buffer, tosend, "UDP response sent");
        }
}

int udp_discovery_init(void)
{
        int ret;
        
        ret = setup_socket();
        if (ret >= 0) {
                k_thread_start(discovery);
        }

        return ret;
}