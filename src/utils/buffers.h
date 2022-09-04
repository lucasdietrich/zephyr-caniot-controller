/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UTILS_BUFFERS_H_
#define _UTILS_BUFFERS_H_

#include <zephyr.h>

#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>

struct readonly_buf
{
	const char *data;
	size_t len;
};

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

int buffer_snprintf(buffer_t *buf, const char *fmt, ...);

static inline size_t buffer_remaining(buffer_t *buffer)
{
	return buffer->size - buffer->filling;
}

static inline size_t buffer_full(buffer_t *buffer)
{
	return buffer->filling == buffer->size;
}

typedef struct {
	char *buffer;
	size_t size;
	char *cursor;
} cursor_buffer_t;

#define CUR_BUFFER_STATIC_INIT(_buf, _size) { \
	.buffer = _buf, \
	.size = _size, \
	.cursor = _buf \
}

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

int cursor_buffer_snprintf(cursor_buffer_t *cbuf, const char *fmt, ...);

#endif /* _UTILS_H_ */