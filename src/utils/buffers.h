/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UTILS_BUFFERS_H_
#define _UTILS_BUFFERS_H_

#include <stddef.h>

#include <net/net_ip.h>
#include <drivers/can.h>

#include <sys/types.h>

/**
 * @brief Append memory to a buffer
 * 
 * @param dst buffer to store the result
 * @param dst_len buffer length
 * @param src source buffer
 * @param src_len source buffer length
 * @return ssize_t 
 */
ssize_t mem_append(void *dst,
		   size_t dst_len,
		   const void *src,
		   size_t src_len);

/**
 * @brief Append a string to a buffer
 * 
 * @param dst buffer to store the result
 * @param dst_len buffer length
 * @param string String to append
 * @return ssize_t 
 */
ssize_t mem_append_string(void *dst,
			  size_t dst_len,
			  const char *string);

/**
 * @brief Append and concatenate strings to a buffer
 * 
 * @param dst buffer to store the result
 * @param dst_len buffer length
 * @param strings Strings to concatenate, all strings should be fined 
 *  		   (i.e. not null) and must be null-terminated)
 * @param count Number of strings to concatenate
 * @return ssize_t 
 */
ssize_t mem_append_strings(void *dst,
			   size_t dst_len,
			   const char **strings,
			   size_t count);

int ipv4_to_str(struct in_addr *addr, char *buffer, size_t len);

int strcicmp(char const *a, char const *b);

int strncicmp(char const *a, char const *b, size_t len);

int get_repr_can_frame(struct zcan_frame *frame, char *buf, size_t len);

typedef struct {
	char *data;
	size_t size;
	size_t filling;
} buffer_t;

#define BUFFER_STATIC_INIT(_buf, _size) { \
	.data = _buf, \
	.size = _size, \
	.filling = 0 \
}

int buffer_init(buffer_t *buffer, char *data, size_t size);

int buffer_reset(buffer_t *buffer);

ssize_t buffer_append(buffer_t *buffer, char *data, size_t size);

ssize_t buffer_append_string(buffer_t *buffer, const char *string);

ssize_t buffer_append_strings(buffer_t *buffer, const char **strings, size_t count);

static inline size_t buffer_remaining(buffer_t *buffer)
{
	return buffer->size - buffer->filling;
}

static inline size_t buffer_full(buffer_t *buffer)
{
	return buffer->filling == buffer->size;
}

/*____________________________________________________________________________*/

typedef struct {
	char *buffer;
	size_t size;
	char *cursor;
} cursor_buffer_t;

int cursor_buffer_init(cursor_buffer_t *cbuf, char *buffer, size_t size);

int cursor_buffer_reset(cursor_buffer_t *cbuf);

ssize_t cursor_buffer_append(cursor_buffer_t *cbuf, char *data, size_t size);

static inline size_t cursor_buffer_filling(cursor_buffer_t *cbuf)
{
	return cbuf->cursor - cbuf->buffer;
}

static inline size_t cursor_buffer_remaining(cursor_buffer_t *cbuf)
{
	return cbuf->size - cursor_buffer_filling(cbuf);
}

static inline size_t cursor_buffer_full(cursor_buffer_t *cbuf)
{
	return cursor_buffer_filling(cbuf) == cbuf->size;
}

static inline void cursor_buffer_shift(cursor_buffer_t *cbuf, size_t size)
{
	cbuf->cursor += MIN(size, cursor_buffer_remaining(cbuf));
}

#endif /* _UTILS_H_ */