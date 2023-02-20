/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include "http_utils.h"
#include "routes.h"
#include "utils/buffers.h"

#include <stdint.h>
#include <stdio.h>

#include <zephyr/net/http/parser.h>

#include <embedc-url/parser.h>

struct http_session;
typedef struct http_session http_session_t;

#define HTTP_URL_MAX_LEN 128u

typedef enum {
	/**
	 * @brief Route not in the list or failed to match
	 * Note: status code 404
	 */
	HTTP_REQUEST_ROUTE_UNKNOWN,

	/**
	 * @brief Bad request, handler still called to encode the response
	 * Note: status code 400
	 */
	HTTP_REQUEST_BAD,

	/**
	 * @brief Route matched, but no handler was found
	 * Note: status code 501
	 */
	HTTP_REQUEST_ROUTE_NO_HANDLER,

	/**
	 * @brief Streaming not supported for the route or the global request
	 * Note: status code 501
	 */
	HTTP_REQUEST_STREAMING_UNSUPPORTED,

	/**
	 * @brief Route OK, but payload is too large to be handled as
	 *  as a single message (consider using chunk encoding = streaming)
	 * Note: status code 413
	 */
	HTTP_REQUEST_PAYLOAD_TOO_LARGE,

	/**
	 * @brief Processing of the request failed
	 * Note: status code 500
	 */
	HTTP_REQUEST_PROCESSING_ERROR,

	/**
	 * @brief Trying to request a protected resource without authentication
	 * Note: status code 403
	 */
	HTTP_REQUEST_UNSECURE_ACCESS,

} http_request_discard_reason_t;

#define HTTP_HEADER_FROM_HANDLE(hp) CONTAINER_OF(hp, struct http_header, handle)

struct http_header {
	sys_dnode_t handle;
	const char *name;
	char value[];
};

struct http_request {
	/**
	 * @brief Flag telling whether keep-alive is set in the request
	 * Note: set in header "Connection"
	 */
	uint8_t keep_alive : 1u;

	/**
	 * @brief Flag telling whether HTTP headers are complete
	 * Note: set in "on_headers_complete" callback
	 */
	uint8_t headers_complete : 1u;

	/**
	 * @brief Flag telling whether HTTP request is complete,
	 * set in "on_message_complete" callback
	 *
	 * Note:
	 * - No more parsing is done after this flag is set
	 * - This flag can be used to determine if a stream is complete
	 */
	uint8_t complete : 1u;

	/* Tells whether the request can be streamed (depends on the route)
	 * This flag is set in "on_headers_complete" callback
	 * and is never changed afterwards
	 */
	uint8_t streaming : 1u;

	/* Tells whether the request is currently discarded */
	uint8_t discarded : 1u;

	/* Tells whether the request is being sent with chunked encoding */
	uint8_t chunked_encoding : 1u;

	/* Tells whether the request is in a secure context */
	uint8_t secure;

	/**
	 * @brief Reason why the request is discarded
	 *
	 * Valid in HTTP_REQUEST_DISCARD mode.
	 */
	http_request_discard_reason_t discard_reason : 3u;

	/**
	 * @brief Request method (GET, POST, PUT, DELETE)
	 */
	enum http_method method;

	/* Header currently being parsed */
	const struct http_header_handler *_parsing_cur_header;

	/* TODO headers values (dynamically allocated and freed, using
	 * HEAP/MEMSLAB ) like authentication, etc ... */
	sys_dlist_t headers;

	/**
	 * @brief Request content type
	 */
	http_content_type_t content_type;

	/**
	 * @brief Timeout of the request in ms (to be used by the app)
	 * Note: Timeout of 0 means no timeout value defined (FOREVER or NONE)
	 */
	uint32_t timeout_ms;

	/* parsed url */
	char url[HTTP_URL_MAX_LEN];

	/* Internal buffer, containing the original url */
	char *_url_copy;

	/* URL len */
	size_t url_len;

	/* Pointer to the start of the query string arguments  */
	char *query_string;

	/* route for the current request */
	const struct route_descr *route;

	/* Route parse result array */
	struct route_parse_result route_parse_results[CONFIG_APP_ROUTE_MAX_DEPTH];

	/* Current route depth */
	uint32_t route_depth;

	/* Number of parts in the route */
	size_t route_parse_results_len;

	/**
	 * @brief Number of times the request route handler has been called
	 * (particulary useful for stream handling)
	 *
	 * Note: Can help to used determine if the handler is called
	 *  to process a new request (= 0).
	 *
	 * Note: It important to not use the chunk ID for this purpose,
	 * because the chunk ID is not necessarily passed to the application as
	 * a whole so the chunk.id is only incremented when the actual HTTP
	 * chunk is complete.
	 */
	size_t calls_count;

	/* Chunk if "stream" is set */
	struct {
		/**
		 * @brief Current chunk of data being parsed
		 */
		uint16_t id;

		/**
		 * @brief Current chunk data buffer location
		 */
		char *loc;

		/**
		 * @brief Number of bytes in the data buffer
		 */
		uint16_t len;

		/**
		 * @brief Offset of the data being parsed within the HTTP chunk
		 */
		uint16_t _offset;
	} chunk;

	/* Complete message payload if "stream" is not set */
	struct {
		/**
		 * @brief Current payload data buffer location
		 *
		 *
		 *
		 * Note: Can be null if no payload is present.
		 *  In this case payload_len and payload.len are 0.
		 */
		char *loc;

		/**
		 * @brief Number of bytes in the data buffer
		 */
		uint32_t len;
	} payload;

	/* Buffer for handling the request data */
	// buffer_t _buffer;

	/**
	 * @brief Total HTTP payload length (in both stream and message mode)
	 *
	 * Note: In streaming mode, it represents the length of the
	 *  payload that has been received so far.
	 */
	size_t payload_len;

	/**
	 * @brief Tells at what buffer fill level the handler should be called
	 *
	 * If flush_len=0 and route supports streaming, handler is called
	 * immediately.
	 *
	 * This value can be tuned during streaming handling.
	 */
	size_t flush_len;

	/**
	 * @brief One parser per connection
	 * - In order to process connections asynchronously
	 * - And a parser can parse several requests in a row
	 */
	struct http_parser parser;

#if defined(CONFIG_APP_HTTP_TEST)
	/**
	 * @brief Test context for CONFIG_APP_HTTP_TEST
	 */
	struct http_test_context _test_ctx;
#endif /* CONFIG_APP_HTTP_TEST */

	/**
	 * @brief User data, which can be used by the application
	 * - Particulary useful for stream handling
	 */
	void *user_data;
};

typedef struct http_request http_request_t;

void http_request_init(http_request_t *req);

static inline bool http_request_is_discarded(const http_request_t *req)
{
	return req->discarded;
}

static inline bool http_request_is_stream(http_request_t *req)
{
	return req->streaming;
}

/**
 * @brief Retrieve argument at index "idx" from the request route
 *
 * If rel_index is a negative value, the index is relative to the end of the
 * route.
 *
 * @param req HTTP request
 * @param rel_index Relative index of the argument to retrieve
 * @param value Pointer to the value to be stored
 * @return int 0 if success, -1 if error
 */
int http_req_route_arg_get_number_by_index(http_request_t *req,
					   int32_t rel_index,
					   uint32_t *value);

/**
 * @brief Retrieve number argument with given name from the request route
 *
 * @param req HTTP request
 * @param name Argument name
 * @param value Pointer to the value to be stored
 * @return int int 0 if success, -1 if error
 */
int http_req_route_arg_get(http_request_t *req, const char *name, uint32_t *value);

/**
 * @brief Retrieve string argument with given name from the request route
 *
 * @param req HTTP request
 * @param name Argument name
 * @param value Pointer to the value to be stored
 * @return int int 0 if success, -1 if error
 */
int http_req_route_arg_get_string(http_request_t *req, const char *name, char **value);

/**
 * @brief Parse the received buffer as a HTTP request
 *
 * @param req Current HTTP request
 * @param data Received data
 * @param len Length of the received data
 * @return true On success
 * @return false On error
 */
bool http_request_parse_buf(http_request_t *req, char *buf, size_t len);

/**
 * @brief Mark the request as discarded
 *
 * @param req
 * @param reason Discard reason
 */
void http_request_discard(http_request_t *req, http_request_discard_reason_t reason);

static inline bool http_request_begins(http_request_t *req)
{
	return req->calls_count == 0;
}

static inline bool http_stream_begins(http_request_t *req)
{
	return http_request_is_stream(req) && (req->complete == 0U) &&
	       (req->calls_count == 0);
}

static inline bool http_request_has_chunk_data(http_request_t *req)
{
	return http_request_is_stream(req) && !req->complete && req->chunked_encoding;
}

static inline bool http_request_has_chunked_encoding(http_request_t *req)
{
	return req->chunked_encoding;
}

static inline bool http_request_complete(http_request_t *req)
{
	return req->complete == 1u;
}

const char *http_header_get_value(http_request_t *req, const char *hdr_name);

/**
 * @brief Write the appropriate HTTP status code in function of the given
 * discard reason.
 *
 * @param reason
 * @param status_code
 * @return true If modified
 * @return false If not modified
 */
bool http_discard_reason_to_status_code(http_request_discard_reason_t reason,
					uint16_t *status_code);

static inline enum http_method http_req_get_method(http_request_t *req)
{
	return http_route_get_method(req->route);
}

#endif