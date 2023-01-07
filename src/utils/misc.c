#include "misc.h"

#include <ctype.h>

ssize_t mem_append(void *dst, size_t dst_len, const void *src, size_t src_len)
{
	if (dst_len < src_len) {
		return -EINVAL;
	}

	memcpy(dst, src, src_len);
	return src_len;
}

ssize_t mem_append_string(void *dst, size_t dst_len, const char *string)
{
	return mem_append(dst, dst_len, string, strlen(string));
}

ssize_t mem_append_strings(void *dst, size_t dst_len, const char **strings, size_t count)
{
	ssize_t appended;
	ssize_t total = 0;

	const char **string;

	for (string = strings; string < strings + count; string++) {
		appended = mem_append_string(
			(uint8_t *)dst + total, dst_len - total, *string);
		if (appended < 0) {
			return appended;
		}
		total += appended;
	}
	return total;
}

int ipv4_to_str(struct in_addr *addr, char *buffer, size_t len)
{
	return net_addr_ntop(AF_INET, addr, buffer, len) == NULL ? -1 : 0;
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

void str_tolower(char *str)
{
	for (int i = 0; str[i]; i++) {
		str[i] = tolower(str[i]);
	}
}

int get_repr_can_frame(struct can_frame *frame, char *buf, size_t len)
{
	return snprintf(buf,
			len,
			"can id: 0x%x, len: %d, data: %02x %02x %02x %02x %02x %02x"
			" %02x %02x",
			frame->id,
			frame->dlc,
			frame->data[0],
			frame->data[1],
			frame->data[2],
			frame->data[3],
			frame->data[4],
			frame->data[5],
			frame->data[6],
			frame->data[7]);
}