#ifndef _UDP_DISCOVERY_H_
#define _UDP_DISCOVERY_H_

#include <net/net_ip.h>

struct discovery_response {
        uint32_t ip;
        char str_ip[NET_IPV4_ADDR_LEN];
} __attribute__((__packed__));

int discovery_setup_socket(void);

int discovery_prepare_reponse(struct discovery_response *resp, size_t len);

void discovery_thread(void *_a, void *_b, void *_c);

#endif