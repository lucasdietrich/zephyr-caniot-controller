/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "buffers.h"

ssize_t mem_append(void *dst,
		   size_t dst_len,
		   const void *src,
		   size_t src_len)
{
	if (dst_len < src_len) {
		return -EINVAL;
	}

	memcpy(dst, src, src_len);
	return src_len;
}

ssize_t mem_append_string(void *dst,
			  size_t dst_len,
			  const char *string)
{
	return mem_append(dst, dst_len, string, strlen(string));
}

ssize_t mem_append_strings(void *dst,
			   size_t dst_len,
			   const char **strings,
			   size_t count)
{
	ssize_t appended;
	ssize_t total = 0;

	const char **string;

	for (string = strings; string < strings + count; string++) {
		appended = mem_append_string((uint8_t *)dst + total,
					     dst_len - total,
					     *string);
		if (appended < 0) {
			return appended;
		}
		total += appended;
	}
	return total;
}

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

int buffer_init(buffer_t *buffer, char *data, size_t size)
{
	int ret = -EINVAL;

	if ((data != NULL) && (size > 0U)) {
		buffer->data = data;
		buffer->size = size;
		buffer->filling = 0U;

		ret = 0;
	}

	return ret;
}

int buffer_reset(buffer_t *buffer)
{
	int ret = -EINVAL;

	if ((buffer != NULL) && (buffer->data != NULL)) {
		buffer->filling = 0U;

		ret = 0;
	}

	return ret;
}

ssize_t buffer_append(buffer_t *buffer, char *data, size_t size)
{
	int ret = -EINVAL;

	if ((buffer->filling + size) <= buffer->size) {
		memcpy((uint8_t *)buffer->data + buffer->filling, data, size);
		buffer->filling += size;

		ret = size;
	}

	return ret;
}

ssize_t buffer_append_string(buffer_t *buffer, const char *string)
{
	// printk("%s [%u] (%p %u)\n", string, strlen(string), buffer->data, buffer->filling);

	return buffer_append(buffer, (void *)string, strlen(string));
}

ssize_t buffer_append_strings(buffer_t *buffer, const char **strings, size_t count)
{
	ssize_t appended;
	ssize_t total = 0;

	const char **string;

	for (string = strings; string < strings + count; string++) {
		appended = buffer_append_string(buffer, *string);
		if (appended < 0) {
			return appended;
		}
		total += appended;
	}
	return total;
}

int buffer_snprintf(buffer_t *buf, const char *fmt, ...)
{
	int ret;
	va_list args;
	va_start(args, fmt);
	const size_t remaining = buf->size - buf->filling;
	ret = snprintf(buf->data, remaining, args);
	if (ret >= 0 && ret <= remaining) {
		buf->filling += ret;
	}
	va_end(args);
	return ret;
}

/*____________________________________________________________________________*/

int cursor_buffer_init(cursor_buffer_t *cbuf, char *buffer, size_t size)
{
	int ret = -EINVAL;

	if ((buffer != NULL) && (size > 0U)) {
		cbuf->buffer = buffer;
		cbuf->cursor = buffer;
		cbuf->size = size;

		ret = 0;
	}

	return ret;
}

int cursor_buffer_reset(cursor_buffer_t *cbuf)
{
	int ret = -EINVAL;

	if ((cbuf != NULL) && (cbuf->buffer != NULL)) {
		cbuf->cursor = cbuf->buffer;

		ret = 0;
	}

	return ret;
}

ssize_t cursor_buffer_append(cursor_buffer_t *cbuf, char *data, size_t size)
{
	int ret = -EINVAL;

	if ((cbuf->cursor - cbuf->buffer) <= cbuf->size) {
		memcpy((uint8_t *)cbuf->cursor, data, size);
		cbuf->cursor += size;

		ret = size;
	}

	return ret;
}