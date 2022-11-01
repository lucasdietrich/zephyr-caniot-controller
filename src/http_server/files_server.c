/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "files_server.h"

#include <libgen.h>
#include <appfs.h>
#include <zephyr/fs/fs.h>

#include <zephyr/data/json.h>
#include <zephyr/net/http_parser.h>

#include "http_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(files_server, LOG_LEVEL_INF);

typedef enum {
	FILE_UPLOAD_OK = 0,
	FILE_UPLOAD_MISSING_FILEPATH_HEADER,
	// FILE_UPLOAD_DIR_CREATION_FAILED,
	// FILE_UPLOAD_FILE_OPEN_FAILED,
	// FILE_UPLOAD_FILE_TRUNCATE_FAILED,
	// FILE_UPLOAD_FILE_WRITE_FAILED,
	// FILE_UPLOAD_FILE_CLOSE_FAILED,
	
	// FILE_UPLOAD_FATAL,
} file_upload_err_t;

struct file_upload_context
{
	const char *basename;
	const char *dirname;
	char filepath[40];
	struct fs_file_t file;

	file_upload_err_t error;
};

/* TODO, dynamically allocate the context */
static struct file_upload_context *sync_file_upload_context(struct http_request *req)
{
	static struct file_upload_context upload;

	if (req->user_data == NULL) {
		req->user_data = &upload;
	}

	return (struct file_upload_context *)req->user_data;
}

/* Non-standard but convenient way to upload a file
 * Send it by chunks as "application/octet-stream"
 * File name to be created is in the header "App-Upload-Filepath"
 * 
 * TODO: Change function return value, we should be able to discard the request
 *  from the handler, but the handler should still be called the discarded request 
 *  is complete. In order to build a proper response (e.g. with a payload
 *  indicating why the request was discarded)
 * 
 * TODO : In the case the file cannot be added to the FS, 
 *  the status code of the http response should be 400 with a payload
 *  indicating the reason.
 */
int http_file_upload(struct http_request *req,
		     struct http_response *resp)
{
	int rc = 0;
	bool close_file = true;
	struct file_upload_context *u = sync_file_upload_context(req);

	if (http_stream_begins(req)) {
		u->error = FILE_UPLOAD_OK;

		/* Get being uploaded file name */
		const char *reqpath = http_header_get_value(req, "App-Upload-Filepath");
		if (reqpath == NULL) {
			http_request_discard(req, HTTP_REQUEST_BAD);
			u->error = FILE_UPLOAD_MISSING_FILEPATH_HEADER;
			rc = 0;
			goto exit;
		}
		
		u->basename = basename((char *) reqpath);
		u->dirname = dirname((char *) reqpath);

		if ((u->basename == NULL) || (u->basename[0] == '\0')) {
			/* TODO generate an available filename */
			u->basename = "UPLOAD.TXT";
		}

		/* Create file path */
		if (u->dirname[0] == '.') {
			snprintf(u->filepath, sizeof(u->filepath),
				 CONFIG_LUA_FS_SCRIPTS_DIR "/%s", u->basename);
		} else {
			char dirpath[40];
			snprintf(dirpath, sizeof(dirpath),
				 CONFIG_FILE_UPLOAD_MOUNT_POINT "/%s", 
				 u->dirname);

			/* Create directory if it doesn't exists */
			struct fs_dirent dir;
			if (fs_stat(dirpath, &dir) == -ENOENT) {
				rc = fs_mkdir(dirpath);
				if (rc != 0) {
					// u->error = FILE_UPLOAD_DIR_CREATION_FAILED;
					LOG_ERR("Failed to create directory %s", dirpath);
					goto exit;
				}
			}

			snprintf(u->filepath, sizeof(u->filepath),
				 CONFIG_FILE_UPLOAD_MOUNT_POINT "/%s/%s", 
				 u->dirname, u->basename);
		}

		LOG_INF("Filepath: %s", u->filepath);

		/* Prepare the file in the filesystem */
		fs_file_t_init(&u->file);
		rc = fs_open(&u->file,
			     u->filepath,
			     FS_O_CREATE | FS_O_WRITE);
		if (rc != 0) {
			// u->error = FILE_UPLOAD_FILE_OPEN_FAILED;
			LOG_ERR("fs_open failed: %d", rc);
			goto exit;
		}

		/* Truncate file in case it already exists */
		rc = fs_truncate(&u->file, 0U);
		if (rc != 0) {
			// u->error = FILE_UPLOAD_FILE_TRUNCATE_FAILED;
			LOG_ERR("fs_truncate failed: %d", rc);
			goto exit;
		}
	}

	/* Prepare data to be written */
	char *data = NULL;
	size_t data_len = 0;

	/* No more data when request is complete */
	if (http_request_has_chunk_data(req)) {
		data = req->chunk.loc;
		data_len = req->chunk.len;
		close_file = false;
	} else if (http_request_is_message(req)) {
		/* In case we support message-based uploads,
		 * we can use the payload */
		data = req->payload.loc;
		data_len = req->payload.len;
	}

	if (data != NULL) {
		LOG_DBG("write loc=%p [%u] file=%p", data, data_len, &u->file);
		ssize_t written = fs_write(&u->file, data, data_len);
		if (written != data_len) {
			// u->error = FILE_UPLOAD_FILE_WRITE_FAILED;
			rc = written;
			LOG_ERR("Failed to write file %d != %u",
				written, data_len);
			goto exit;
		}
	}

	if (close_file) {
		rc = fs_close(&u->file);
		if (rc) {
			// u->error = FILE_UPLOAD_FILE_CLOSE_FAILED;
			LOG_ERR("Failed to close file = %d", rc);
			goto ret;
		}

		LOG_INF("File %s upload succeeded [size = %u]",
			u->filepath, req->payload_len);
		
		if (Z_LOG_CONST_LEVEL_CHECK(LOG_LEVEL_DBG)) {
			app_fs_stats(CONFIG_FILE_UPLOAD_MOUNT_POINT);
		}
	}

	if (http_request_complete(req)) {
		/* Encode message */
		if (u->error == FILE_UPLOAD_OK) {
			buffer_append_string(&resp->buffer, "Upload succeeded");
		} else if (u->error == FILE_UPLOAD_MISSING_FILEPATH_HEADER) {
			buffer_append_string(&resp->buffer, 
					     "Upload failed: FILE_UPLOAD_MISSING_FILEPATH_HEADER");
		} else {
			buffer_append_string(&resp->buffer, "Unknown error");
		}
		resp->content_length = resp->buffer.filling;
	}

exit:
	if (rc != 0) {
		fs_close(&u->file);
	}

ret:
	return rc;
}

/* TODO a file descriptor is not closed somewhere bellow, which causes 
 * file downloads to fails after ~4 downloads. */
int http_file_download(struct http_request *req,
		       struct http_response *resp)
{
	int ret;
	static struct fs_file_t file;

	if (http_response_is_first_call(resp)) {
		/* Check and extract filepath */
		const char *subpath = http_route_extract_subpath(req);
		if (subpath == NULL || strlen(subpath) == 0) {
			http_response_set_status_code(resp, HTTP_STATUS_BAD_REQUEST);
			return 0;
		}

		/* Normalize filepath */
		char filepath[128u];
		if (app_fs_filepath_normalize(subpath, filepath, sizeof(filepath)) < 0) {
			http_response_set_status_code(resp, HTTP_STATUS_BAD_REQUEST);
			return 0;
		}

		/* Get file stats */
		struct fs_dirent dirent;
		ret = fs_stat(filepath, &dirent);
		if (ret == -ENOENT) {
			http_response_set_status_code(resp,
						      HTTP_STATUS_NOT_FOUND);
			return 0;
		} else if (ret != 0) {
			http_response_set_status_code(resp,
						      HTTP_STATUS_INTERNAL_SERVER_ERROR);
			return 0;
		} else if (dirent.type == FS_DIR_ENTRY_DIR) {
			http_response_set_status_code(resp,
						      HTTP_STATUS_BAD_REQUEST);
			return 0;
		}

		LOG_INF("File=%s size=%u", filepath, dirent.size);
		http_response_set_content_length(resp, dirent.size);

		fs_file_t_init(&file);
		ret = fs_open(&file, filepath, FS_O_READ);
		if (ret != 0) {
			http_response_set_status_code(
				resp, HTTP_STATUS_INTERNAL_SERVER_ERROR);
			return 0;
		}

		/* Reference context */
		req->user_data = &file;
	}

	if (req->user_data != NULL) {
		ret = fs_read(&file, resp->buffer.data, resp->buffer.size);
		if (ret < 0) {
			fs_close(&file);
			http_response_set_status_code(
				resp, HTTP_STATUS_INTERNAL_SERVER_ERROR);
			return 0;
		} else if (ret == 0) {
			/* EOF */
			fs_close(&file);
			req->user_data = NULL;
			return 0;
		}

		resp->buffer.filling = ret;

		/* If more that are excepted */
		if (ret == resp->buffer.size) {
			http_response_more_data(resp);
		} else {
			fs_close(&file);
		}
	}

	return 0;
}