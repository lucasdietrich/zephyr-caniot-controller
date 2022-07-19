#include "files_server.h"

#include <stdlib.h>
#include <errno.h>
#include <appfs.h>
#include <unistd.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(files_server, LOG_LEVEL_INF);

int http_file_upload(struct http_request *req,
		     struct http_response *resp)
{
	/* TODO remove file before returning in case of error */

	int rc;
	static FILE *file = NULL;
	bool close_file = true;

	if (file == NULL) {
		file = fopen("/RAM:/upload.txt", "w");
		if (file == NULL) {
			LOG_ERR("Failed to open file = %d", errno);
			return -1;
		}

		// fs_truncate(file, 0);

		/* reset file pointer to the beginning of the file */
		// rc = fseek(file, 0, SEEK_SET);
		// if (rc) {
		// 	LOG_ERR("Failed to seek file = %d", errno);
		// 	return -1;
		// }
	}

	char *buf = NULL;
	size_t buf_len = 0;

	/* No more data when request is complete */
	if (http_request_is_stream(req) && !req->complete) {
		buf = req->chunk.loc;
		buf_len = req->chunk.len;
		close_file = false;
	} else if (http_request_is_message(req)) {
		buf = req->payload.loc;
		buf_len = req->payload.len;
	}

	if (buf != NULL) {
		LOG_DBG("write loc=%p [%u] file=%p", buf, buf_len, file);
		size_t written = fwrite(buf, 1, buf_len, file);
		if (written != buf_len) {
			LOG_ERR("Failed to write file %u != %u",
				written, buf_len);
			return -1;
		}
		rc = fflush(file);
		if (rc) {
			LOG_ERR("Failed to flush file = %d", errno);
			return -1;
		}
	}

	if (close_file) {
		rc = fclose(file);
		file = NULL;
		if (rc) {
			LOG_ERR("Failed to close file = %d", errno);
			return -1;
		}

		LOG_INF("File upload.txt uploaded [size = %u]", 
			req->payload_len);

		app_fs_stats("/RAM:/");
	}

	return 0;
}
