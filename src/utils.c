#include "utils.h"

int ipv4_to_str(struct in_addr *addr, char *buffer, size_t len)
{
        return net_addr_ntop(AF_INET, addr, buffer, len) == NULL ? - 1 : 0;
}