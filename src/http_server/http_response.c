/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_response.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_response, LOG_LEVEL_WRN);

void http_response_init(http_response_t *resp)
{
	/* TODO optimize the way the request is initialized */
	memset(resp, 0x00U, sizeof(http_response_t));

	resp->buffer.data = NULL;
	resp->buffer.size = 0u;
	resp->buffer.filling = 0u;

	/* default response */

	/**
	 * @brief HTTP Content length, header not encoded if negative.
	 */
	resp->content_length = 0u;
	resp->status_code = HTTP_DEFAULT_RESP_STATUS_CODE;
	resp->content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN;

	resp->stream = 0u;
	resp->complete = 1u;

	resp->calls_count = 0u;

	resp->headers_sent = 0u;
	resp->payload_sent = 0u;
}

static bool set_header_check(http_response_t *resp, const char *hdr_name)
{
	if (!http_response_is_first_call(resp)) {
		LOG_WRN("Cannot set (%s), Headers already sent, calls_count=%u",
			hdr_name, resp->calls_count);
		return false;
	}

	return true;
}

void http_response_set_content_type(http_response_t *resp,
				    http_content_type_t content_type)
{
	if (set_header_check(resp, "Content-Type") == true) {
		resp->content_type = content_type;
	}
}

void http_response_set_status_code(http_response_t *resp,
				   uint16_t status_code)
{
	if (set_header_check(resp, "Status-Code") == true) {
		resp->status_code = status_code;
	}
}

void http_response_set_content_length(http_response_t *resp,
				      ssize_t content_length)
{
	if (content_length < 0) {
		LOG_WRN("Cannot set Content-Length to negative value (%d)", content_length);
		return;
	}

	if (set_header_check(resp, "Content-Length") == true) {
		resp->content_length = content_length;
		resp->stream = 0u; /* Disable chunked encoding */
	}
}

void http_response_enable_chunk_encoding(http_response_t *resp)
{
	if (set_header_check(resp, "Chunk-Encoding") == true) {
		resp->stream = 1u;
		resp->content_length = -1; /* Disable content length */
	}
}

bool http_response_is_stream(http_response_t *resp)
{
	return (bool) resp->stream;
}