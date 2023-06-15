/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "buffers.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>

int buffer_init(buffer_t *buffer, char *data, size_t size)
{
	int ret = -EINVAL;

	if ((data != NULL) && (size > 0U)) {
		buffer->data	= data;
		buffer->size	= size;
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
	// printk("%s [%u] (%p %u)\n", string, strlen(string), buffer->data,
	// buffer->filling);

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
	ret					   = vsnprintf(&buf->data[buf->filling], remaining, fmt, args);
	if (ret >= 0 && ret <= remaining) {
		buf->filling += ret;
	}
	va_end(args);
	return ret;
}

int cursor_buffer_init(cursor_buffer_t *cbuf, char *buffer, size_t size)
{
	int ret = -EINVAL;

	if ((buffer != NULL) && (size > 0U)) {
		cbuf->buffer = buffer;
		cbuf->cursor = buffer;
		cbuf->size	 = size;

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

int cursor_buffer_snprintf(cursor_buffer_t *cbuf, const char *fmt, ...)
{
	int ret;
	va_list args;
	va_start(args, fmt);
	const size_t remaining = cursor_buffer_remaining(cbuf);
	ret					   = vsnprintf(cbuf->cursor, remaining, fmt, args);
	if (ret >= 0 && ret <= remaining) {
		cbuf->cursor += ret;
	}
	va_end(args);
	return ret;
}