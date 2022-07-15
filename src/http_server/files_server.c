#include "files_server.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(files_server, LOG_LEVEL_INF);

int http_file_upload(struct http_request *req,
		     struct http_response *resp)
{
	if (req->complete == 0) {
		/* We are receiving a chunk */
		LOG_DBG("STREAM receiving chunk=%u len=%u (offset = %u)",
			req->chunk.id, req->chunk.len, req->chunk._offset);

		if (req->payload_len > 30000) {
			return -1;
		}
	} else {
		LOG_INF("STREAM complete received=%u, encoding response", req->payload_len);

		__ASSERT(resp != NULL, "Resp should be set to prepare response");
	}

	return 0;
}