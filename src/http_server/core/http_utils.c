/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_request.h"
#include "http_response.h"
#include "http_utils.h"
#include "utils/buffers.h"
#include "utils/misc.h"

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(http_utils, LOG_LEVEL_WRN);

#define HTTP_UTILS_MAX_EXTENSION_LEN 8u

struct code_str {
	http_status_code_t code;
	char *str;
};

static const struct code_str status[] = {
	{HTTP_STATUS_OK, "OK"},
	{HTTP_STATUS_CREATED, "Created"},
	{HTTP_STATUS_ACCEPTED, "Accepted"},
	{HTTP_STATUS_NO_CONTENT, "No Content"},

	{HTTP_STATUS_BAD_REQUEST, "Bad Request"},
	{HTTP_STATUS_UNAUTHORIZED, "Unauthorized"},
	{HTTP_STATUS_FORBIDDEN, "Forbidden"},
	{HTTP_STATUS_NOT_FOUND, "Not Found"},
	{HTTP_STATUS_REQUEST_TIMEOUT, "Request Timeout"},
	{HTTP_STATUS_LENGTH_REQUIRED, "Length Required"},
	{HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE, "Request Entity Too Large"},
	{HTTP_STATUS_REQUEST_URI_TOO_LONG, "Request-URI Too Long"},
	{HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type"},

	{HTTP_STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error"},
	{HTTP_STATUS_NOT_IMPLEMENTED, "Not Implemented"},
	{HTTP_STATUS_BAD_GATEWAY, "Bad Gateway"},
	{HTTP_STATUS_SERVICE_UNAVAILABLE, "Service Unavailable"},
	{HTTP_STATUS_GATEWAY_TIMEOUT, "Gateway Timeout"},
	{HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported"},
	{HTTP_STATUS_INSUFFICIENT_STORAGE, "Insufficient Storage"},
};

static const char *get_status_code_str(http_status_code_t status_code)
{
	for (const struct code_str *p = status; p <= &status[ARRAY_SIZE(status) - 1];
	     p++) {
		if (p->code == status_code) {
			return p->str;
		}
	}
	return NULL;
}

int http_encode_status(buffer_t *buf, http_status_code_t status_code)
{
	const char *code_str = get_status_code_str(status_code);
	if (code_str == NULL) {
		LOG_ERR("unknown status_code %d", status_code);
		return -1;
	}
	return buffer_snprintf(buf, "HTTP/1.1 %d %s\r\n", status_code, code_str);
}

int http_encode_header_content_length(buffer_t *buf, ssize_t content_length)
{
	int ret = 0;
	if (content_length >= 0) {
		ret = buffer_snprintf(buf, "Content-Length: %u\r\n", content_length);
	}
	return ret;
}

int http_encode_header_transer_encoding_chunked(buffer_t *buf)
{
	return buffer_snprintf(buf, "Transfer-Encoding: chunked\r\n");
}

int http_encode_header_connection(buffer_t *buf, bool keep_alive)
{
	static const char *connection_str[] = {"close", "keep-alive"};

	return buffer_snprintf(buf,
			       "Connection: %s\r\n",
			       keep_alive ? connection_str[1] : connection_str[0]);
}

int http_encode_header_content_type(buffer_t *buf, http_content_type_t type)
{
	return buffer_snprintf(
		buf, "Content-Type: %s\r\n", http_content_type_to_str(type));
}

int http_encode_endline(buffer_t *buf)
{
	return buffer_snprintf(buf, "\r\n");
}

bool http_code_has_payload(uint16_t status_code)
{
	return (status_code == HTTP_STATUS_OK || status_code == HTTP_STATUS_BAD_REQUEST);
}

const char *http_content_type_to_str(http_content_type_t content_type)
{
	const char *strs[] = {
		[HTTP_CONTENT_TYPE_TEXT_PLAIN]		     = "text/plain",
		[HTTP_CONTENT_TYPE_TEXT_HTML]		     = "text/html",
		[HTTP_CONTENT_TYPE_TEXT_CSS]		     = "text/css",
		[HTTP_CONTENT_TYPE_TEXT_JAVASCRIPT]	     = "text/javascript",
		[HTTP_CONTENT_TYPE_APPLICATION_JSON]	     = "application/json",
		[HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA]	     = "multipart/form-data",
		[HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM] = "application/octet-stream",
	};

	if (content_type >= ARRAY_SIZE(strs)) {
		content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN;
	}

	return strs[content_type];
}

const char *http_filepath_get_extension(const char *filepath)
{
	const char *ext = filepath;
	const char *p	= filepath;

	while (*p != '\0') {
		if (*p == '.') {
			ext = p + 1;
		}
		p++;
	}

	return ext;
}

/**
 * @brief Get the mime type for the given file extension
 *
 * Extension is case insensitive.
 *
 * @param filepath
 * @return const char*
 */
http_content_type_t http_get_content_type_from_extension(const char *extension)
{
	http_content_type_t content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN;

	if (extension != NULL) {
		char extension_lower[HTTP_UTILS_MAX_EXTENSION_LEN + 1];
		strncpy(extension_lower, extension, HTTP_UTILS_MAX_EXTENSION_LEN);
		str_tolower(extension_lower);

		if (strcmp(extension_lower, "html") == 0) {
			content_type = HTTP_CONTENT_TYPE_TEXT_HTML;
		} else if (strcmp(extension_lower, "css") == 0) {
			content_type = HTTP_CONTENT_TYPE_TEXT_CSS;
		} else if (strcmp(extension_lower, "js") == 0) {
			content_type = HTTP_CONTENT_TYPE_TEXT_JAVASCRIPT;
		} else if (strcmp(extension_lower, "xml") == 0) {
			content_type = HTTP_CONTENT_TYPE_TEXT_XML;
		} else if (strcmp(extension_lower, "json") == 0) {
			content_type = HTTP_CONTENT_TYPE_APPLICATION_JSON;
		} else if (strcmp(extension_lower, "gif") == 0) {
			content_type = HTTP_CONTENT_TYPE_APPLICATION_GIF;
		} else if (strcmp(extension_lower, "jpeg") == 0) {
			content_type = HTTP_CONTENT_TYPE_APPLICATION_JPEG;
		} else if (strcmp(extension_lower, "jpg") == 0) {
			content_type = HTTP_CONTENT_TYPE_APPLICATION_JPEG;
		} else if (strcmp(extension_lower, "png") == 0) {
			content_type = HTTP_CONTENT_TYPE_APPLICATION_PNG;
		} else if (strcmp(extension_lower, "tiff") == 0) {
			content_type = HTTP_CONTENT_TYPE_APPLICATION_TIFF;
		}
	}

	return content_type;
}

/*____________________________________________________________________________*/

#if defined(CONFIG_APP_HTTP_TEST)

void http_test_init_context(struct http_test_context *ctx)
{
	*ctx = (struct http_test_context){0};
}

static uint32_t checksum_add(uint32_t checksum, const uint8_t *data, size_t len)
{
	while (len--) {
		checksum += *data++;
	}
	return checksum;
}

#define HTTP_RESP_BUFFER_MINIMUM_SIZE (512)

static http_test_result_t test_req_handler(struct http_test_context *ctx,
					   struct http_request *req,
					   struct http_response *resp)
{
	http_test_result_t result = ctx->result;

	if (resp != NULL) {
		result = HTTP_TEST_RESULT_RESP_HANDLER_UNEXPECTED;
		goto exit;
	}

	if (http_request_complete(req)) {
		result = HTTP_TEST_RESULT_REQ_COMPLETE_UNEXPECTED;
		goto exit;
	}

	if (ctx->req_calls_count == 0 && req->calls_count != 0) {
		result = HTTP_TEST_RESULT_REQ_CALLS_COUNT_IS_NOT_ZERO;
		goto exit;
	}

	if (!http_request_is_stream(req)) {
		result = HTTP_TEST_RESULT_STREAM_EXPECTED;
		goto exit;
	}

	/* check calls count */
	if (http_stream_begins(req)) {
		/* On first call (streaming): "calls_count" needs to be 0 */
		if (req->calls_count != 0) {
			result = HTTP_TEST_RESULT_CALLS_COUNT_IS_NOT_ZERO;
			goto exit;
		}

		/* First received chunk id should be 0 */
		if (req->chunk.id != 0) {
			result = HTTP_TEST_RESULT_CHUNK_ID_IS_NOT_ZERO;
			goto exit;
		}

		/* user_data should be NULL on first call */
		if (req->user_data != NULL) {
			result = HTTP_TEST_RESULT_USER_DATA_IS_NOT_NULL;
			goto exit;
		}

	} else {
		/* Gap on subsequent calls of the function */
		if (req->calls_count != ctx->req_calls_count) {
			/* strictly monotonic */
			result = HTTP_TEST_RESULT_REQ_CALLS_COUNT_DISCONTINUITY;
			goto exit;
		}
	}

	/* For chunked encoding */
	if (req->chunked_encoding) {
		if (req->chunk.loc == NULL) {
			result = HTTP_TEST_RESULT_CHUNK_LOC_EXPECTED;
			goto exit;
		}

		if (req->chunk.len == 0U) {
			result = HTTP_TEST_RESULT_CHUNK_LEN_EXPECTED;
			goto exit;
		}

		if (req->chunk.id > ctx->last_chunk_id + 1U) {
			/* monotonic is sufficient
			 * Several calls can be made on the same chunk,
			 * but chunks cannot be skipped.
			 */
			result = HTTP_TEST_RESULT_CHUNK_ID_DISCONTINUITY;
			goto exit;
		}

		if (req->chunk.id == ctx->last_chunk_id + 1U) {
			ctx->chunk_received_bytes = 0U;
		}
		if (ctx->chunk_received_bytes != req->chunk._offset) {
			result = HTTP_TEST_RESULT_CHUNK_OFFSET_INVALID;
			goto exit;
		}

		ctx->chunk_received_bytes += req->chunk.len;

		/* Increment received bytes */
		ctx->received_bytes += req->chunk.len;
	} else { /* Non chunked encoding */

		if (req->chunk.loc != NULL) {
			result = HTTP_TEST_RESULT_CHUNK_LOC_UNEXPECTED;
			goto exit;
		}

		if (req->chunk.len != 0U) {
			result = HTTP_TEST_RESULT_CHUNK_LEN_UNEXPECTED;
			goto exit;
		}

		if (req->chunk.id != 0U) {
			result = HTTP_TEST_RESULT_CHUNK_ID_UNEXPECTED;
			goto exit;
		}

		/* Increment received bytes */
		ctx->received_bytes += req->payload.len;
	}

	/* Payload infos should always be set */
	if (req->payload.loc == NULL) {
		result = HTTP_TEST_RESULT_PAYLOAD_LOC_EXPECTED;
		goto exit;
	}

	if (req->payload_len != ctx->received_bytes) {
		result = HTTP_TEST_RESULT_PAYLOAD_LEN_INVALID;
		goto exit;
	}

	/* Calculate checksum */
	ctx->checksum = checksum_add(ctx->checksum, req->chunk.loc, req->chunk.len);

	ctx->last_chunk_id = req->chunk.id;

	/* Check total received data size */
exit:
	ctx->req_calls_count++;
	return result;
}

static http_test_result_t test_resp_handler(struct http_test_context *ctx,
					    struct http_request *req,
					    struct http_response *resp)
{
	http_test_result_t result = ctx->result;

	if (resp == NULL) {
		result = HTTP_TEST_RESULT_RESP_EXPECTED;
		goto exit;
	}

	if (!http_request_complete(req)) {
		result = HTTP_TEST_RESULT_REQ_NOT_COMPLETE;
		goto exit;
	}

	/* Response is assumed to be complete by default, the application
	 * has to explicitly set it to incomplete if more data need
	 * to be sent.
	 */
	if (resp->complete != 1u) {
		result = HTTP_TEST_RESULT_RESP_NOT_ASSUMED_COMPLETE_BY_DEFAULT;
		goto exit;
	}

	if (http_request_is_stream(req)) {
		if ((req->payload.len != 0) || (req->payload.loc != NULL)) {
			result = HTTP_TEST_RESULT_PAYLOAD_UNEXPECTED_AFTER_STREAMING;
			goto exit;
		}
	} else {
		ctx->received_bytes += req->payload.len;

		if (req->payload.len != req->payload_len) {
			result = HTTP_TEST_RESULT_PAYLOAD_LEN_INVALID;
			goto exit;
		}

		// ctx->delta_ms = k_uptime_delta32(&ctx->uptime_ms);
	}

	if (req->payload_len != ctx->received_bytes) {
		result = HTTP_TEST_RESULT_PAYLOAD_LEN_INVALID;
		goto exit;
	}

	if (req->chunk.loc != NULL) {
		result = HTTP_TEST_RESULT_CHUNK_LOC_UNEXPECTED;
		goto exit;
	}

	if (req->chunk.len != 0) {
		result = HTTP_TEST_RESULT_CHUNK_LEN_UNEXPECTED;
		goto exit;
	}

	if (resp->buffer.data == NULL) {
		result = HTTP_TEST_RESULT_RESP_BUFFER_EXPECTED;
		;
		goto exit;
	}

	if (resp->buffer.data == NULL) {
		result = HTTP_TEST_RESULT_RESP_BUFFER_EXPECTED;
		goto exit;
	}

	if (resp->buffer.filling != 0) {
		result = HTTP_TEST_RESULT_RESP_BUFFER_NOT_EMPTY;
		goto exit;
	}

	if (resp->buffer.size < HTTP_RESP_BUFFER_MINIMUM_SIZE) {
		result = HTTP_TEST_RESULT_RESP_BUFFER_TOO_SMALL;
		goto exit;
	}

	if (ctx->resp_calls_count == 0u && resp->calls_count != 0) {
		result = HTTP_TEST_RESULT_RESP_CALLS_COUNT_IS_NOT_ZERO;
		goto exit;
	}

	if (http_response_is_first_call(resp)) {
		if (resp->content_length != 0) {
			result = HTTP_TEST_RESULT_RESP_CONTENT_LENGTH_INVALID;
			goto exit;
		}

		if (resp->headers_sent != 0) {
			result = HTTP_TEST_RESULT_RESP_HEADERS_SENT_INVALID;
			goto exit;
		}

		if (resp->payload_sent != 0) {
			result = HTTP_TEST_RESULT_RESP_HEADERS_RECEIVED_INVALID;
			goto exit;
		}

		if (resp->status_code != HTTP_DEFAULT_RESP_STATUS_CODE) {
			result = HTTP_TEST_RESULT_RESP_DEFAULT_STATUS_CODE_INVALID;
			goto exit;
		}

		if (resp->chunked != 0u) {
			result = HTTP_TEST_RESULT_RESP_DEFAULT_NO_STREAM_BY_DEFAULT;
			goto exit;
		}
	} else {
		/* Gap on subsequent calls of the function */
		if (resp->calls_count != ctx->resp_calls_count) {
			/* strictly monotonic */
			result = HTTP_TEST_RESULT_RESP_CALLS_COUNT_DISCONTINUITY;
			goto exit;
		}
	}
exit:
	ctx->resp_calls_count++;
	return result;
}

http_test_result_t http_test_run(struct http_test_context *ctx,
				 struct http_request *req,
				 struct http_response *resp,
				 enum http_test_handler cur_handler)
{
	http_test_result_t result = HTTP_TEST_NO_CONTEXT_GIVEN;

	if (ctx == NULL) {
		goto ret;
	}

	result = ctx->result;

	if (ctx->result != HTTP_TEST_RESULT_OK) {
		goto ret;
	}

	/* http_request should be kept valid for the whole duration of the
	 * HTTP request (i.e. Until response is completely sent) */
	if (req == NULL) {
		result = HTTP_TEST_RESULT_REQ_EXPECTED;
		goto exit;
	}

	/**
	 * If request is mark as discarded, the application handler should not
	 * be called anymore.
	 */
	if (http_request_is_discarded(req)) {
		result = HTTP_TEST_RESULT_DISCARDING_BUT_CALLED;
		goto exit;
	}

	/* If request is a stream, proper handling method should be selected */
	if (!ctx->stream && http_request_is_stream(req)) {
		result = HTTP_TEST_RESULT_MESSAGE_EXPECTED;
		goto exit;
	}

	/* If request is a message, proper handling method should be selected */
	if (ctx->stream && !http_request_is_stream(req)) {
		result = HTTP_TEST_RESULT_STREAM_EXPECTED;
		goto exit;
	}

	/* Application handler shouldn't be called if route is not found */
	if (req->route == NULL) {
		result = HTTP_TEST_RESULT_ROUTE_EXPECTED;
		goto exit;
	}

	/* Application handler shouldn't be called if HTTP method doesn't
	 * match what the route expects */
	if ((req->route->flags & ROUTE_METHODS_MASK) !=
	    http_method_to_route_flag(req->method)) {
		result = HTTP_TEST_RESULT_METHOD_UNEXPECTED;
		goto exit;
	}

	/* headers_complete flag should be set when application handler is
	 * called */
	if (!req->headers_complete) {
		result = HTTP_TEST_RESULT_HEADERS_COMPLETE_EXPECTED;
		goto exit;
	}

	if (cur_handler == HTTP_TEST_HANDLER_REQ) {
		result = test_req_handler(ctx, req, resp);
	} else {
		result = test_resp_handler(ctx, req, resp);
	}

exit:
	ctx->result = result;
ret:
	if (result != HTTP_TEST_RESULT_OK) {
		LOG_ERR("(%p / %p) Test failed with %s (%d)",
			req,
			resp,
			http_test_result_to_str(result),
			result);
	}
	return result;
}

const char *http_test_result_to_str(http_test_result_t result)
{
	switch (result) {
	case HTTP_TEST_RESULT_OK:
		return "OK";
	case HTTP_TEST_NO_CONTEXT_GIVEN:
		return "NO_CONTEXT_GIVEN";
	case HTTP_TEST_RESULT_MESSAGE_EXPECTED:
		return "MESSAGE_EXPECTED";
	case HTTP_TEST_RESULT_STREAM_EXPECTED:
		return "STREAM_EXPECTED";
	case HTTP_TEST_RESULT_DISCARDING_BUT_CALLED:
		return "DISCARDING_BUT_CALLED";
	case HTTP_TEST_RESULT_REQ_EXPECTED:
		return "REQ_EXPECTED";
	case HTTP_TEST_RESULT_RESP_EXPECTED:
		return "RESP_EXPECTED";
	case HTTP_TEST_RESULT_ROUTE_EXPECTED:
		return "ROUTE_EXPECTED";
	case HTTP_TEST_RESULT_METHOD_UNEXPECTED:
		return "METHOD_UNEXPECTED";
	case HTTP_TEST_RESULT_HEADERS_COMPLETE_EXPECTED:
		return "HEADERS_COMPLETE_EXPECTED";
	case HTTP_TEST_RESULT_CALLS_COUNT_IS_NOT_ZERO:
		return "CALLS_COUNT_IS_NOT_ZERO";
	case HTTP_TEST_RESULT_CALLS_COUNT_DISCONTINUITY:
		return "CALLS_COUNT_DISCONTINUITY";
	case HTTP_TEST_RESULT_CHUNK_ID_IS_NOT_ZERO:
		return "CHUNK_ID_IS_NOT_ZERO";
	case HTTP_TEST_RESULT_CHUNK_ID_DISCONTINUITY:
		return "CHUNK_ID_DISCONTINUITY";
	case HTTP_TEST_RESULT_CHUNK_LOC_EXPECTED:
		return "CHUNK_LOC_EXPECTED";
	case HTTP_TEST_RESULT_CHUNK_LEN_EXPECTED:
		return "CHUNK_LEN_EXPECTED";
	case HTTP_TEST_RESULT_CHUNK_LOC_UNEXPECTED:
		return "CHUNK_LOC_UNEXPECTED";
	case HTTP_TEST_RESULT_CHUNK_LEN_UNEXPECTED:
		return "CHUNK_LEN_UNEXPECTED";
	case HTTP_TEST_RESULT_CHUNK_OFFSET_INVALID:
		return "CHUNK_OFFSET_INVALID";
	case HTTP_TEST_RESULT_PAYLOAD_LEN_INVALID:
		return "PAYLOAD_LEN_INVALID";
	case HTTP_TEST_RESULT_RESP_BUFFER_EXPECTED:
		return "RESP_BUFFER_EXPECTED";
	case HTTP_TEST_RESULT_REQ_PAYLOAD_EXPECTED:
		return "REQ_PAYLOAD_EXPECTED";
	case HTTP_TEST_RESULT_MESSAGE_MODE_SINGLE_CALL_EXPECTED:
		return "MESSAGE_MODE_SINGLE_CALL_EXPECTED";
	case HTTP_TEST_RESULT_USER_DATA_IS_NOT_NULL:
		return "USER_DATA_IS_NOT_NULL";
	case HTTP_TEST_RESULT_USER_DATA_IS_NOT_VALID:
		return "USER_DATA_IS_NOT_VALID";
	case HTTP_TEST_RESULT_RESP_CALLS_COUNT_INVALID:
		return "HTTP_TEST_RESULT_RESP_CALLS_COUNT_INVALID";
	case HTTP_TEST_RESULT_RESP_COMPLETE_INVALID:
		return "HTTP_TEST_RESULT_RESP_COMPLETE_INVALID";
	case HTTP_TEST_RESULT_REQ_CALLS_COUNT_IS_NOT_ZERO:
		return "HTTP_TEST_RESULT_REQ_CALLS_COUNT_IS_NOT_ZERO";
	case HTTP_TEST_RESULT_REQ_CALLS_COUNT_DISCONTINUITY:
		return "HTTP_TEST_RESULT_REQ_CALLS_COUNT_DISCONTINUITY";
	case HTTP_TEST_RESULT_REQ_NOT_COMPLETE:
		return "HTTP_TEST_RESULT_REQ_NOT_COMPLETE";
	case HTTP_TEST_RESULT_RESP_NOT_ASSUMED_COMPLETE_BY_DEFAULT:
		return "HTTP_TEST_RESULT_RESP_NOT_ASSUMED_COMPLETE_BY_DEFAULT";
	case HTTP_TEST_RESULT_RESP_BUFFER_NOT_EMPTY:
		return "HTTP_TEST_RESULT_RESP_BUFFER_NOT_EMPTY";
	case HTTP_TEST_RESULT_RESP_BUFFER_TOO_SMALL:
		return "HTTP_TEST_RESULT_RESP_BUFFER_TOO_SMALL";
	case HTTP_TEST_RESULT_RESP_CALLS_COUNT_IS_NOT_ZERO:
		return "HTTP_TEST_RESULT_RESP_CALLS_COUNT_IS_NOT_ZERO";
	case HTTP_TEST_RESULT_RESP_CONTENT_LENGTH_INVALID:
		return "HTTP_TEST_RESULT_RESP_CONTENT_LENGTH_INVALID";
	case HTTP_TEST_RESULT_RESP_HEADERS_SENT_INVALID:
		return "HTTP_TEST_RESULT_RESP_HEADERS_SENT_INVALID";
	case HTTP_TEST_RESULT_RESP_HEADERS_RECEIVED_INVALID:
		return "HTTP_TEST_RESULT_RESP_HEADERS_RECEIVED_INVALID";
	case HTTP_TEST_RESULT_RESP_DEFAULT_STATUS_CODE_INVALID:
		return "HTTP_TEST_RESULT_RESP_DEFAULT_STATUS_CODE_INVALID";
	case HTTP_TEST_RESULT_RESP_DEFAULT_NO_STREAM_BY_DEFAULT:
		return "HTTP_TEST_RESULT_RESP_DEFAULT_NO_STREAM_BY_DEFAULT";
	case HTTP_TEST_RESULT_RESP_CALLS_COUNT_DISCONTINUITY:
		return "HTTP_TEST_RESULT_RESP_CALLS_COUNT_DISCONTINUITY";
	case HTTP_TEST_RESULT_PAYLOAD_LOC_EXPECTED:
		return "HTTP_TEST_RESULT_PAYLOAD_LOC_EXPECTED";
	case HTTP_TEST_RESULT_CHUNK_ID_UNEXPECTED:
		return "HTTP_TEST_RESULT_CHUNK_ID_UNEXPECTED";
	case HTTP_TEST_RESULT_PAYLOAD_UNEXPECTED_AFTER_STREAMING:
		return "HTTP_TEST_RESULT_PAYLOAD_UNEXPECTED_AFTER_STREAMING";
	default:
		return "UNKNOWN";
	}
}

#endif /* CONFIG_APP_HTTP_TEST */