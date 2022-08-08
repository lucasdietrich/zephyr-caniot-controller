/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <kernel.h>

#include "http_utils.h"

#include "utils/buffers.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_utils, LOG_LEVEL_WRN);

/*____________________________________________________________________________*/

struct code_str
{
        http_status_code_t code;
        char *str;
};

static const struct code_str status[] = {
	{HTTP_OK, "OK"},
	{HTTP_CREATED, "Created"},
	{HTTP_ACCEPTED, "Accepted"},
	{HTTP_NO_CONTENT, "No Content"},

	{HTTP_BAD_REQUEST, "Bad Request"},
	{HTTP_UNAUTHORIZED, "Unauthorized"},
	{HTTP_FORBIDDEN, "Forbidden"},
	{HTTP_NOT_FOUND, "Not Found"},
	{HTTP_REQUEST_TIMEOUT, "Request Timeout"},
	{HTTP_LENGTH_REQUIRED, "Length Required"},
	{HTTP_REQUEST_ENTITY_TOO_LARGE, "Request Entity Too Large"},
	{HTTP_REQUEST_URI_TOO_LONG, "Request-URI Too Long"},
	{HTTP_UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type"},

	{HTTP_INTERNAL_SERVER_ERROR, "Internal Server Error"},
	{HTTP_NOT_IMPLEMENTED, "Not Implemented"},
	{HTTP_BAD_GATEWAY, "Bad Gateway"},
	{HTTP_SERVICE_UNAVAILABLE, "Service Unavailable"},
	{HTTP_GATEWAY_TIMEOUT, "Gateway Timeout"},
	{HTTP_HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported"},
	{HTTP_INSUFFICIENT_STORAGE, "Insufficient Storage"},
};

static const char *get_status_code_str(http_status_code_t status_code)
{
        for (const struct code_str *p = status;
             p <= &status[ARRAY_SIZE(status) - 1]; p++) {
                if (p->code == status_code) {
                        return p->str;
                }
        }
        return NULL;
}

int http_encode_status(char *buf, size_t len, http_status_code_t status_code)
{
        const char *code_str = get_status_code_str(status_code);
        if (code_str == NULL) {
                LOG_ERR("unknown status_code %d", status_code);
                return -1;
        }

        return snprintf(buf, len, "HTTP/1.1 %d %s\r\n", status_code, code_str);
}

int http_encode_header_content_length(char *buf, size_t len, size_t content_length)
{
        return snprintf(buf, len, "Content-Length: %u\r\n", content_length);
}

int http_encode_header_connection(char *buf, size_t len, bool keep_alive)
{
        static const char *connection_str[] = {
                "close",
                "keep-alive"
        };

        return snprintf(buf, len, "Connection: %s\r\n",
                        keep_alive ? connection_str[1] : connection_str[0]);
}

int http_encode_header_content_type(char *buf,
				    size_t len,
				    http_content_type_t type)
{
	char *content_type_str;

	switch (type) {
	case HTTP_CONTENT_TYPE_TEXT_HTML:
		content_type_str = "text/html";
		break;
	case HTTP_CONTENT_TYPE_APPLICATION_JSON:
		content_type_str = "application/json";
		break;
	case HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA:
		content_type_str = "multipart/form-data";
		break;
	case HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM:
		content_type_str = "application/octet-stream";
		break;
	case HTTP_CONTENT_TYPE_TEXT_PLAIN:
	default:
		content_type_str = "text/plain";
		break;
	}

	const char *parts[] = {
		"Content-Type: ",
		content_type_str,
		"\r\n"
	};

	return mem_append_strings(buf, len, parts, ARRAY_SIZE(parts));
}

int http_encode_header_end(char *buf, size_t len)
{
        return snprintf(buf, len, "\r\n");
}

bool http_code_has_payload(uint16_t status_code)
{
        return (status_code == 200);
}

const char *http_content_type_to_str(http_content_type_t content_type)
{
	switch (content_type) {
	case HTTP_CONTENT_TYPE_NONE:
		return "{notset}";
	case HTTP_CONTENT_TYPE_TEXT_HTML:
		return "text/html";
	case HTTP_CONTENT_TYPE_APPLICATION_JSON:
		return "application/json";
	case HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM:
		return "application/octet-stream";
	case HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA:
		return "multipart/form-data";
	case HTTP_CONTENT_TYPE_TEXT_PLAIN:
	default:
		return "text/plain";
	}
}

/*____________________________________________________________________________*/

#include "http_request.h"
#include "http_response.h"

void http_test_init_context(struct http_test_context *ctx)
{
	*ctx = (struct http_test_context){ 0 };
}

static uint32_t checksum_add(uint32_t checksum, const uint8_t *data, size_t len)
{
	while (len--) {
		checksum += *data++;
	}
	return checksum;
}

http_test_result_t http_test_run(struct http_test_context *ctx,
				 struct http_request *req,
				 struct http_response *resp)
{
	http_test_result_t result = HTTP_TEST_NO_CONTEXT_GIVEN;

	if (ctx == NULL) {
		goto ret;
	}

	result = ctx->result;

	if (ctx->result != HTTP_TEST_RESULT_OK) {
		goto ret;
	}

	if (req == NULL) {
		result = HTTP_TEST_RESULT_REQ_EXPECTED;
		goto exit;
	}

	if (http_request_is_discarded(req)) {
		result = HTTP_TEST_RESULT_DISCARDING_BUT_CALLED;
		goto exit;
	}

	if (!ctx->stream && !http_request_is_message(req)) {
		result = HTTP_TEST_RESULT_MESSAGE_EXPECTED;
		goto exit;
	}

	if (ctx->stream && !http_request_is_stream(req)) {
		result = HTTP_TEST_RESULT_STREAM_EXPECTED;
		goto exit;
	}

	if (req->route == NULL) {
		result = HTTP_TEST_RESULT_ROUTE_EXPECTED;
		goto exit;
	}

	if (req->route->method != req->method) {
		result = HTTP_TEST_RESULT_METHOD_UNEXPECTED;
		goto exit;
	}

	if (!req->headers_complete) {
		result = HTTP_TEST_RESULT_HEADERS_COMPLETE_EXPECTED;
		goto exit;
	}

	if (ctx->stream) {
		/* check calls count */
		if (http_stream_begins(req)) {
			if (req->calls_count != 0) {
				result = HTTP_TEST_RESULT_CALLS_COUNT_IS_NOT_ZERO;
				goto exit;
			}

			if (req->chunk.id != 0) {
				result = HTTP_TEST_RESULT_CHUNK_ID_IS_NOT_ZERO;
				goto exit;
			}

			if (req->user_data != NULL) {
				result = HTTP_TEST_RESULT_USER_DATA_IS_NOT_NULL;
				goto exit;
			}

		} else {
			if (req->calls_count != ++ctx->last_call_number) {
				/* strictly monotonic */
				result = HTTP_TEST_RESULT_CALLS_COUNT_DISCONTINUITY;
				goto exit;
			}
		}

		if (http_stream_completes(req)) {
			if (req->chunk.loc != NULL) {
				result = HTTP_TEST_RESULT_CHUNK_LOC_UNEXPECTED;
				goto exit;
			}

			if (req->chunk.len != 0) {
				result = HTTP_TEST_RESULT_CHUNK_LEN_UNEXPECTED;
				goto exit;
			}

			if (req->payload_len != ctx->received_bytes) {
				result = HTTP_TEST_RESULT_PAYLOAD_LEN_INVALID;
				goto exit;
			}

			if (resp == NULL) {
				result = HTTP_TEST_RESULT_RESP_EXPECTED;
				goto exit;
			}

			ctx->delta_ms = k_uptime_delta32(&ctx->uptime_ms);
		} else {
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
				 * */
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

			/* Calculate checksum */
			ctx->checksum = checksum_add(ctx->checksum,
						     req->chunk.loc,
						     req->chunk.len);

		}

		ctx->last_chunk_id = req->chunk.id;

		/* Check total received data size */

	} else {
		if (ctx->calls_count++ != 0) {
			result = HTTP_TEST_RESULT_MESSAGE_MODE_SINGLE_CALL_EXPECTED;
			goto exit;
		}

		if (req->payload.len != req->payload_len) {
			result = HTTP_TEST_RESULT_REQ_PAYLOAD_LEN_INVALID;
			goto exit;
		}

		if ((req->payload.len != 0) && (req->payload.loc == NULL)) {
			result = HTTP_TEST_RESULT_REQ_PAYLOAD_EXPECTED;
			goto exit;
		}

		if (resp == NULL) {
			result = HTTP_TEST_RESULT_RESP_EXPECTED;
			goto exit;
		}

		if (resp->buffer.data == NULL) {
			result = HTTP_TEST_RESULT_RESP_BUFFER_EXPECTED;;
			goto exit;
		}
	}

exit:
	ctx->result = result;
ret:
	if (result != HTTP_TEST_RESULT_OK) {
		LOG_ERR("(%p / %p) Test failed with %s (%d)", req, resp,
			log_strdup(http_test_result_to_str(result)), result);
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
	case HTTP_TEST_RESULT_REQ_PAYLOAD_LEN_INVALID:
		return "REQ_PAYLOAD_LEN_INVALID";
	case HTTP_TEST_RESULT_USER_DATA_IS_NOT_NULL:
		return "USER_DATA_IS_NOT_NULL";
	case HTTP_TEST_RESULT_USER_DATA_IS_NOT_VALID:
		return "USER_DATA_IS_NOT_VALID";
	default:
		return "UNKNOWN";
	}
}