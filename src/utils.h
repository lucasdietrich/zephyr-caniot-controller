#ifndef _UTILS_H_
#define _UTILS_H_

#include <stddef.h>

#include <net/net_ip.h>
#include <drivers/can.h>

int ipv4_to_str(struct in_addr *addr, char *buffer, size_t len);

int strcicmp(char const *a, char const *b);

int strncicmp(char const *a, char const *b, size_t len);

int get_repr_can_frame(struct zcan_frame *frame, char *buf, size_t len);

#endif /* _UTILS_H_ */