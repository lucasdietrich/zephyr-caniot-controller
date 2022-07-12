#include "files_server.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(files_server, LOG_LEVEL_DBG);

int http_file_upload(struct http_request *req,
		     struct http_response *resp)
{
	LOG_INF("chunk=%u len=%u (offset = %u)",
		req->chunk.id, req->chunk.len, req->chunk._offset);

	return 0;
}