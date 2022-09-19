/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_server.h"

#include <data/json.h>
#include <net/http_parser.h>

#include "utils/misc.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(test_server, LOG_LEVEL_INF);

static uint32_t test_checksum(uint32_t checksum, const char *str, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		checksum += str[i];
	}
	return checksum;
}

extern int rest_encode_response_json(http_response_t *resp, const void *val,
				     const struct json_obj_descr *descr,
				     size_t descr_len);

struct json_test_result
{
	uint32_t ok;
	uint32_t payload_len;
	uint32_t payload_checksum;
	uint32_t route_args[HTTP_ROUTE_ARGS_MAX_COUNT];
	uint32_t route_args_count;

	/* time from first chunk received and until last chunk received */
	uint32_t duration_ms;
};

static const struct json_obj_descr json_test_result_message_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_test_result, ok, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_test_result, payload_len, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_test_result, payload_checksum, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_ARRAY(struct json_test_result, route_args, HTTP_ROUTE_ARGS_MAX_COUNT,
	route_args_count, JSON_TOK_NUMBER),
};

int http_test_messaging(struct http_request *req,
			struct http_response *resp)
{
	bool req_ok = true;
	req_ok &= req->method == req->route->method;
	req_ok &= req->handling_mode == HTTP_REQUEST_MESSAGE;
	req_ok &= req->payload.len == req->payload_len;

	struct json_test_result tr = {
		.ok = (uint32_t)req_ok,
		.payload_len = req->payload_len,
		.payload_checksum = test_checksum(0U, req->payload.loc, req->payload_len),
		.route_args_count = 0U,
	};

	for (uint32_t i = 0; i < HTTP_ROUTE_ARGS_MAX_COUNT; i++) {
		if (http_request_route_arg_get(req, i, &tr.route_args[i]) == 0) {
			tr.route_args_count++;
		} else {
			break;
		}
	}

	return rest_encode_response_json(resp, &tr, json_test_result_message_descr,
					 ARRAY_SIZE(json_test_result_message_descr));
}

typedef enum {
	TEST_OK,
	TEST_PROCESSING_ERROR,
	TEST_SLOW_PROCESSING,
} http_test_type_t;

static const struct json_obj_descr json_test_result_stream_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_test_result, ok, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_test_result, payload_len, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_test_result, payload_checksum, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_ARRAY(struct json_test_result, route_args, HTTP_ROUTE_ARGS_MAX_COUNT,
	route_args_count, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_test_result, duration_ms, JSON_TOK_NUMBER),
};

int http_test_streaming(struct http_request *req,
			struct http_response *resp)
{
	static uint32_t checksum = 0U;
	static uint32_t received_bytes = 0U;
	static uint32_t last_chunk_id = -1;
	static uint32_t uptime_ms;
	static bool req_ok = true;

	/* Always check */
	req_ok &= req->method == req->route->method;
	req_ok &= req->handling_mode == HTTP_REQUEST_STREAM;

	if (req->complete == 0) {
		/* We are receiving a chunk */
		LOG_DBG("STREAM receiving chunk=%u len=%u (offset = %u)",
			req->chunk.id, req->chunk.len, req->chunk._offset);

		if (req->calls_count == 0) {
			checksum = 0;
			received_bytes = 0;
			last_chunk_id = -1;
			req_ok = true;
			uptime_ms = k_uptime_get_32();
		}

		checksum = test_checksum(checksum, req->chunk.loc, req->chunk.len);
		received_bytes += req->chunk.len;

		req_ok &= received_bytes == req->payload_len;

		/* check that there are not gaps in the chunks */
		if (req->chunk.id > last_chunk_id + 1) {
			req_ok = false;
		}
		last_chunk_id = req->chunk.id;
	} else {
		struct json_test_result tr = {
			.ok = (uint32_t)req_ok,
			.payload_len = req->payload_len,
			.payload_checksum = checksum,
			.route_args_count = 0U,
			.duration_ms = k_uptime_delta32(&uptime_ms),
		};

		for (uint32_t i = 0; i < HTTP_ROUTE_ARGS_MAX_COUNT; i++) {
			if (http_request_route_arg_get(req, i, &tr.route_args[i]) == 0) {
				tr.route_args_count++;
			} else {
				break;
			}
		}

		return rest_encode_response_json(resp, &tr, json_test_result_stream_descr,
						 ARRAY_SIZE(json_test_result_stream_descr));
	}

	return 0;
}

int http_test_route_args(struct http_request *req,
			 struct http_response *resp)
{
	LOG_INF("arg1=%u, arg2=%u arg3=%u",
		req->route_args[0], req->route_args[1], req->route_args[2]);

	/* Parse the query string */
	struct query_arg qal[10u];
	int ret = parse_url_query_args(req->url, qal, ARRAY_SIZE(qal));
	if (ret < 0) {
		LOG_WRN("Failed to parse query string ret=%d", ret);
	} else if (ret >= 0) {
		LOG_INF("%d query args found", ret);

		struct query_arg *arg;
		for (int i = 0; i < ret; i++) {
			arg = &qal[i];
			LOG_DBG("%u: %s : %s", i, log_strdup(arg->key),
				arg->value ? log_strdup(arg->value) : "(null)");
		}

		char *val = query_arg_get("name", qal, ret);
		LOG_DBG("Parameter 'name' = %s", val ? log_strdup(val) : "(null)");
	}

	

	return 0;
}

int http_test_big_payload(struct http_request *req,
			  struct http_response *resp)
{
	LOG_INF("handling_mode=%u", req->handling_mode);

	return 0;
}

int http_test_headers(struct http_request *req,
		      struct http_response *resp)
{
	sys_dnode_t *node;
	SYS_DLIST_FOR_EACH_NODE(&req->headers, node) {
		struct http_header *header = HTTP_HEADER_FROM_HANDLE(node);
		LOG_INF("[header] %s: %s", log_strdup(header->name), 
			log_strdup(header->value));
	}

	return 0;
}

int http_test_payload(struct http_request *req,
		      struct http_response *resp)
{
	static uint32_t it = 0;
	const uint32_t n = 10u;

	size_t s = MIN(1000u, resp->buffer.size);

	if (http_response_is_first_call(resp)) {
		it = 0u;

		// http_response_set_content_length(resp, -1);
		// http_response_set_content_length(resp, n * s);
		http_response_enable_chunk_encoding(resp);
		http_response_set_content_type(resp, HTTP_CONTENT_TYPE_TEXT_PLAIN);
	}

	if (++it < n) {
		// buffer_snprintf(&resp->buffer, "Hello, world! %u", it);
		http_response_more_data(resp);
	}

	memset(resp->buffer.data, 'A', s);
	resp->buffer.filling = s;

	return 0;
}