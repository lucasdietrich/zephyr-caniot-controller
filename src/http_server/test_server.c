#include "test_server.h"

#include <data/json.h>
#include <net/http_parser.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(test_server, LOG_LEVEL_INF);

static enum {
	TEST_OK,
	TEST_PROCESSING_ERROR,
	TEST_SLOW_PROCESSING,
} test_type = TEST_OK;

int http_test_messaging(struct http_request *req,
			struct http_response *resp)
{
	return 0;
}

int http_test_streaming(struct http_request *req,
			struct http_response *resp)
{
	static uint32_t zeros = 0;

	if (req->complete == 0) {
		/* We are receiving a chunk */
		LOG_DBG("STREAM receiving chunk=%u len=%u (offset = %u)",
			req->chunk.id, req->chunk.len, req->chunk._offset);

		if (req->calls_count == 0) {
			zeros = 0;
		}

		for (int i = 0; i < req->chunk.len; i++) {
			if (req->chunk.loc[i] == 0) {
				zeros++;
			}
		}

		/* Emulate problem */
		switch (test_type) {
		case TEST_PROCESSING_ERROR:
			if (req->payload_len > 30000) {
				return -1;
			}
		case TEST_SLOW_PROCESSING:
			k_sleep(K_MSEC(100));
		default:
			break;
		}

	} else {
		LOG_INF("STREAM complete received=%u, encoding response", req->payload_len);
		LOG_INF("zeros = %u / %u", zeros, req->payload_len);

		__ASSERT(resp != NULL, "Resp should be set to prepare response");
	}

	return 0;
}

int http_test_route_args(struct http_request *req,
			 struct http_response *resp)
{
	LOG_INF("arg1=%u, arg2=%u arg3=%u",
		req->route_args[0], req->route_args[1], req->route_args[2]);

	return 0;
}

int http_test_big_payload(struct http_request *req,
			  struct http_response *resp)
{
	LOG_INF("handling_method=%u", req->handling_method);

	return 0;
}