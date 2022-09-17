/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include <stdio.h>
#include <stdint.h>

#include "utils/buffers.h"
#include "http_utils.h"

#define HTTP_DEFAULT_RESP_STATUS_CODE HTTP_STATUS_OK

typedef struct http_response
{
	/**
	 * @brief Content-type to be encoded in the header
	 */
	http_content_type_t content_type;

	/**
	 * @brief HTTP Status code to be encoded
	 */
	uint16_t status_code;

	/**
	 * @brief Content-length of the payload
	 */
	ssize_t content_length;

	/**
	 * @brief Buffer used to store the payload
	 */
	buffer_t buffer;

	/**
	 * @brief Flag to indicate whether the response is complete
	 *
	 * Note: Set this flag to false to indicate there are more
	 *  data to be sent.
	 */
	uint8_t complete: 1u;

	/**
	 * @brief Flag to indicate whether the should be sent as chunks
	 * 
	 * Note: In this case the content_length field is ignored.
	 * Note: This flag must be set before the the first part of the payload
	 *   is	sent.
	 */
	uint8_t stream: 1u;

	/* Number of times the response handler has been called */
	uint32_t calls_count;

	/* Total number of bytes sent in headers and payload*/

	/**
	 * Total number of bytes sent in headers
	 * 
	 * Note: Indicate whether the headers have been already sent or not
	 */
	size_t headers_sent;

	/**
	 * Total number of bytes sent in the payload
	 */
	size_t payload_sent;
} http_response_t;

void http_response_init(http_response_t *resp);

static inline bool http_response_is_first_call(http_response_t *resp)
{
	return resp->calls_count == 0u;
}

void http_response_set_content_type(http_response_t *resp,
				    http_content_type_t content_type);

void http_response_set_status_code(http_response_t *resp,
				   uint16_t status_code);

void http_response_set_content_length(http_response_t *resp,
				      ssize_t content_length);

static inline void http_response_more_data(http_response_t *resp)
{
	resp->complete = 0u;
}

void http_response_enable_chunk_encoding(http_response_t *resp);

bool http_response_is_stream(http_response_t *resp);

#endif