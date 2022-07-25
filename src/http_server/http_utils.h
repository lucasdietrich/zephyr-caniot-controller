/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_UTILS_H_
#define _HTTP_UTILS_H_

#include <stdbool.h>
#include <stdint.h>

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
	HTTP_OK = 200,
	HTTP_CREATED = 201,
	HTTP_ACCEPTED = 202,
	HTTP_NO_CONTENT = 204,

	/* 400 */
	HTTP_BAD_REQUEST = 400,
	HTTP_UNAUTHORIZED = 401,
	HTTP_FORBIDDEN = 403,
	HTTP_NOT_FOUND = 404,
	// HTTP_METHOD_NOT_ALLOWED = 405,
	// HTTP_NOT_ACCEPTABLE = 406,
	HTTP_REQUEST_TIMEOUT = 408,
	// HTTP_CONFLICT = 409,
	// HTTP_GONE = 410,
	HTTP_LENGTH_REQUIRED = 411,
	// HTTP_PRECONDITION_FAILED = 412,
	HTTP_REQUEST_ENTITY_TOO_LARGE = 413,
	HTTP_REQUEST_URI_TOO_LONG = 414,
	HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
	// HTTP_RANGE_NOT_SATISFIABLE = 416,

	/* 500 */
	HTTP_INTERNAL_SERVER_ERROR = 500,
	HTTP_NOT_IMPLEMENTED = 501,
	HTTP_BAD_GATEWAY = 502,
	HTTP_SERVICE_UNAVAILABLE = 503,
	HTTP_GATEWAY_TIMEOUT = 504,
	HTTP_HTTP_VERSION_NOT_SUPPORTED = 505,
	HTTP_INSUFFICIENT_STORAGE = 507,

} http_status_code_t;

int http_encode_status(char *buf, size_t len, http_status_code_t status_code);

int http_encode_header_content_length(char *buf, size_t len, size_t content_length);

int http_encode_header_connection(char *buf, size_t len, bool keep_alive);

int http_encode_header_content_type(char *buf, size_t len, http_content_type_t type);

int http_encode_header_end(char *buf, size_t len);

bool http_code_has_payload(uint16_t status_code);

const char *http_content_type_to_str(http_content_type_t content_type);

// typedef struct
// {
// 	uint8_t *buf;
// 	uint16_t 
// 	uint16_t len;
// 	uint16_t id;
// } http_chunk_t;

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
	HTTP_TEST_RESULT_REQ_PAYLOAD_LEN_INVALID,
	HTTP_TEST_RESULT_USER_DATA_IS_NOT_NULL,
	HTTP_TEST_RESULT_USER_DATA_IS_NOT_VALID,
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
	
	union {
		/**
		 * @brief When called in stream mode, this is the 
		 * expected value for "calls_count"
		 */
		uint32_t last_call_number;

		/**
		 * @brief When called in message mode, this is the
		 * number of times the handler is called,
		 * Should be called once only.
		 */
		uint32_t calls_count;
	};

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
};

struct http_request;
struct http_response;

void http_test_init_context(struct http_test_context *ctx);

http_test_result_t http_test_run(struct http_test_context *ctx,
				 struct http_request *req,
				 struct http_response *resp);

const char *http_test_result_to_str(http_test_result_t result);

#endif