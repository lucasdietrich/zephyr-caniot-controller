/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_response.h"

void http_response_init(http_response_t *resp)
{
	/* TODO optimize the way the request is initialized */
	memset(resp, 0x00U, sizeof(http_response_t));

	resp->buffer.data = NULL;
	resp->buffer.size = 0u;
	resp->buffer.filling = 0u;

	/* default response */
	resp->content_length = 0u;
	resp->status_code = 200U;
	resp->content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN;

	resp->stream = 0u;
	resp->complete = 1u;
}