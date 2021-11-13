#include "utils.h"

#include <string.h>
#include <ctype.h>

int ipv4_to_str(struct in_addr *addr, char *buffer, size_t len)
{
        return net_addr_ntop(AF_INET, addr, buffer, len) == NULL ? - 1 : 0;
}

int strcicmp(char const *a, char const *b)
{
        for (;; a++, b++) {
                int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
                if (d != 0 || !*a)
                        return d;
        }
}

int strncicmp(char const *a, char const *b, size_t len)
{
        for (int s = 0; s < len; a++, b++, s++) {
                int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
                if (d != 0 || !*a)
                        return d;
        }
        return 0;
}