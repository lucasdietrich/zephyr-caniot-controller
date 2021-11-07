#ifndef _UTILS_H_
#define _UTILS_H_

#include <stddef.h>
#include <net/net_ip.h>

int ipv4_to_str(struct in_addr *addr, char *buffer, size_t len);

#endif