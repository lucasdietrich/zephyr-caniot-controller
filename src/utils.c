#include <kernel.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"

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

int get_repr_can_frame(struct zcan_frame *frame, char *buf, size_t len)
{
	return snprintf(buf, len, "can id: 0x%x, len: %d, data: %02x %02x %02x %02x %02x %02x"
			" %02x %02x", frame->id, frame->dlc, frame->data[0], frame->data[1],
			frame->data[2], frame->data[3], frame->data[4], frame->data[5],
			frame->data[6], frame->data[7]);
}