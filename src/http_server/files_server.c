/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "files_server.h"

#include <libgen.h>

#include <appfs.h>
#include <fs/fs.h>

#include <logging/log.h>
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

		LOG_INF("Filepath: %s", log_strdup(u->filepath));

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
			log_strdup(u->filepath), req->payload_len);

		if (_LOG_LEVEL() >= LOG_LEVEL_DBG) {
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
