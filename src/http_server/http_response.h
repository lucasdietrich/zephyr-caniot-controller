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

struct http_response
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
	size_t content_length;

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
};

typedef struct http_response http_response_t;

void http_response_init(http_response_t *resp);

#endif