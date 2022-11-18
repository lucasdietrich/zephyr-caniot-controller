/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_UTILS_H_
#define _HTTP_UTILS_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "utils/buffers.h"

typedef enum {
	HTTP_CONTENT_TYPE_NONE = 0,
	HTTP_CONTENT_TYPE_TEXT_PLAIN,
	HTTP_CONTENT_TYPE_TEXT_HTML,
	HTTP_CONTENT_TYPE_APPLICATION_JSON,
	HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA,
	HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM,
} http_content_type_t;

typedef enum {
	/* 400 */
	HTTP_STATUS_OK = 200,
	HTTP_STATUS_CREATED = 201,
	HTTP_STATUS_ACCEPTED = 202,
	HTTP_STATUS_NO_CONTENT = 204,

	/* 400 */
	HTTP_STATUS_BAD_REQUEST = 400,
	HTTP_STATUS_UNAUTHORIZED = 401,
	HTTP_STATUS_FORBIDDEN = 403,
	HTTP_STATUS_NOT_FOUND = 404,
	// HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
	// HTTP_STATUS_NOT_ACCEPTABLE = 406,
	HTTP_STATUS_REQUEST_TIMEOUT = 408,
	// HTTP_STATUS_CONFLICT = 409,
	// HTTP_STATUS_GONE = 410,
	HTTP_STATUS_LENGTH_REQUIRED = 411,
	// HTTP_STATUS_PRECONDITION_FAILED = 412,
	HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE = 413,
	HTTP_STATUS_REQUEST_URI_TOO_LONG = 414,
	HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE = 415,
	// HTTP_STATUS_RANGE_NOT_SATISFIABLE = 416,

	/* 500 */
	HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
	HTTP_STATUS_NOT_IMPLEMENTED = 501,
	HTTP_STATUS_BAD_GATEWAY = 502,
	HTTP_STATUS_SERVICE_UNAVAILABLE = 503,
	HTTP_STATUS_GATEWAY_TIMEOUT = 504,
	HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED = 505,
	HTTP_STATUS_INSUFFICIENT_STORAGE = 507,

} http_status_code_t;


int http_encode_status(buffer_t *buf, http_status_code_t status_code);

/**
 * @brief Encode the Content-Length header if content_length is greater or equal than 0
 * 
 * @param buf 
 * @param content_length 
 * @return int 
 */
int http_encode_header_content_length(buffer_t *buf, ssize_t content_length);

int http_encode_header_transer_encoding_chunked(buffer_t *buf);

int http_encode_header_connection(buffer_t *buf, bool keep_alive);

int http_encode_header_content_type(buffer_t *buf, http_content_type_t type);

int http_encode_endline(buffer_t *buf);

static inline int http_encode_header_end(buffer_t *buf)
{
	return http_encode_endline(buf);
}

bool http_code_has_payload(uint16_t status_code);

const char *http_content_type_to_str(http_content_type_t content_type);

/*____________________________________________________________________________*/

/* Test Utils */
typedef enum {
	HTTP_TEST_RESULT_OK = 0,
	HTTP_TEST_NO_CONTEXT_GIVEN,
	HTTP_TEST_RESULT_MESSAGE_EXPECTED,
	HTTP_TEST_RESULT_STREAM_EXPECTED,
	HTTP_TEST_RESULT_DISCARDING_BUT_CALLED,
	HTTP_TEST_RESULT_REQ_EXPECTED,
	HTTP_TEST_RESULT_RESP_EXPECTED,
	HTTP_TEST_RESULT_ROUTE_EXPECTED,
	HTTP_TEST_RESULT_METHOD_UNEXPECTED,
	HTTP_TEST_RESULT_HEADERS_COMPLETE_EXPECTED,
	HTTP_TEST_RESULT_CALLS_COUNT_IS_NOT_ZERO,
	HTTP_TEST_RESULT_CALLS_COUNT_DISCONTINUITY,
	HTTP_TEST_RESULT_CHUNK_ID_IS_NOT_ZERO,
	HTTP_TEST_RESULT_CHUNK_ID_DISCONTINUITY,
	HTTP_TEST_RESULT_CHUNK_LOC_EXPECTED,
	HTTP_TEST_RESULT_CHUNK_LEN_EXPECTED,
	HTTP_TEST_RESULT_CHUNK_LOC_UNEXPECTED,
	HTTP_TEST_RESULT_CHUNK_LEN_UNEXPECTED,
	HTTP_TEST_RESULT_CHUNK_OFFSET_INVALID,
	HTTP_TEST_RESULT_PAYLOAD_LEN_INVALID,
	HTTP_TEST_RESULT_RESP_BUFFER_EXPECTED,
	HTTP_TEST_RESULT_REQ_PAYLOAD_EXPECTED,
	HTTP_TEST_RESULT_MESSAGE_MODE_SINGLE_CALL_EXPECTED,
	HTTP_TEST_RESULT_USER_DATA_IS_NOT_NULL,
	HTTP_TEST_RESULT_USER_DATA_IS_NOT_VALID,
	HTTP_TEST_RESULT_RESP_CALLS_COUNT_INVALID,
	HTTP_TEST_RESULT_RESP_COMPLETE_INVALID,
	HTTP_TEST_RESULT_RESP_HANDLER_UNEXPECTED,
	HTTP_TEST_RESULT_REQ_COMPLETE_UNEXPECTED,
	HTTP_TEST_RESULT_REQ_CALLS_COUNT_IS_NOT_ZERO,
	HTTP_TEST_RESULT_REQ_CALLS_COUNT_DISCONTINUITY,
	HTTP_TEST_RESULT_REQ_NOT_COMPLETE,
	HTTP_TEST_RESULT_RESP_NOT_ASSUMED_COMPLETE_BY_DEFAULT,
	HTTP_TEST_RESULT_RESP_BUFFER_NOT_EMPTY,
	HTTP_TEST_RESULT_RESP_BUFFER_TOO_SMALL,
	HTTP_TEST_RESULT_RESP_CALLS_COUNT_IS_NOT_ZERO,
	HTTP_TEST_RESULT_RESP_CONTENT_LENGTH_INVALID,
	HTTP_TEST_RESULT_RESP_HEADERS_SENT_INVALID,
	HTTP_TEST_RESULT_RESP_HEADERS_RECEIVED_INVALID,
	HTTP_TEST_RESULT_RESP_DEFAULT_STATUS_CODE_INVALID,
	HTTP_TEST_RESULT_RESP_DEFAULT_NO_STREAM_BY_DEFAULT,
	HTTP_TEST_RESULT_RESP_CALLS_COUNT_DISCONTINUITY,
	HTTP_TEST_RESULT_PAYLOAD_LOC_EXPECTED,
	HTTP_TEST_RESULT_CHUNK_ID_UNEXPECTED,
} http_test_result_t;

struct http_test_context {
	uint32_t checksum;

	enum {
		HTTP_CHECKSUM_ALGO_NONE = 0,
		HTTP_CHECKSUM_ALGO_ADD,
	} checksum_algo;

	uint32_t received_bytes;

	uint32_t chunk_received_bytes;

	/**
	 * @brief When called in stream mode, this is the 
	 * id of the last chunked processed.
	 */
	uint32_t last_chunk_id;

	/**
	 * @brief When called in message mode, this is the
	 * number of times the handler is called,
	 * Should be called once only.
	 * 
	 * In stream mode, this is the 
	 * expected value for "calls_count" on each call
	 */
	uint32_t req_calls_count;

	uint32_t resp_calls_count; /* same as req, but for response */

	/**
	 * @brief When called in stream mode:
	 * - On first call, this is the uptime of the first call.
	 * - On last call, this contain the difference between the first and last call.
	 * 
	 * Ignored in message mode.
	 */
	union {

		uint32_t uptime_ms;
		uint32_t delta_ms;
	};

	/**
	 * @brief Expected request type stream or message
	 * (decided on the first call)
	 */
	uint32_t stream: 1;

	/**
	 * @brief Current test result
	 */
	http_test_result_t result;

	/* On response infos */
	void *payload_buf;
	size_t payload_size;
};

struct http_request;
struct http_response;

void http_test_init_context(struct http_test_context *ctx);

enum http_test_handler {
	HTTP_TEST_HANDLER_REQ,
	HTTP_TEST_HANDLER_RESP,
};


http_test_result_t http_test_run(struct http_test_context *ctx,
				 struct http_request *req,
				 struct http_response *resp,
				 enum http_test_handler cur_handler);

const char *http_test_result_to_str(http_test_result_t result);

#endif