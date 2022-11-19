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

#define FILES_SERVER_MOUNT_POINT 	CONFIG_FILES_SERVER_MOUNT_POINT
#define FILES_SERVER_MOUNT_POINT_SIZE 	(sizeof(FILES_SERVER_MOUNT_POINT) - 1u)

#define FILES_SERVER_CREATE_DIR_IF_NOT_EXISTS 	1u

#define FILES_SERVER_FILEPATH_MAX_DEPTH 	4u

#define FILE_FILEPATH_MAX_LEN 128u

#if defined(CONFIG_FILE_ACCESS_HISTORY)

struct file_access_history {
    char *path;
    uint32_t count;

    uint32_t read: 1u;
    uint32_t write: 1u;
};

static struct file_access_history history[CONFIG_FILE_ACCESS_HISTORY_SIZE];
static uint32_t history_count = 0u;

#endif /* CONFIG_FILE_ACCESS_HISTORY */

// TODO
// static int get_arg_path(http_request_t *req, size_t nargs, ...)

static const char *route_path_arg_names[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };

static int req_get_arg_path_parts(http_request_t *req, char *parts[], size_t size)
{
	__ASSERT_NO_MSG(req != NULL);
	__ASSERT_NO_MSG(parts != NULL);
	__ASSERT_NO_MSG(size != 0u);

	size = MIN(size, ARRAY_SIZE(route_path_arg_names));
	size = MIN(size, req->route_parse_results_len);

	uint32_t i;
	for (i = 0; i < size; i++) {
		int ret = http_req_route_arg_get_string(req,
							route_path_arg_names[i],
							&parts[i]);
		if (ret == -ENOENT) {
			break;
		} else if (ret < 0) {
			return ret;
		}
	}

	return (int)i;
}

static int filepath_build(char *path_parts[],
			  size_t path_parts_size,
			  char filepath[],
			  size_t size)
{
	if (size <= FILES_SERVER_MOUNT_POINT_SIZE) {
		return -ENOMEM;
	}
	
	strcpy(filepath, FILES_SERVER_MOUNT_POINT);
	size -= FILES_SERVER_MOUNT_POINT_SIZE;

	/* Reconstruct filepath */
	char *p = &filepath[FILES_SERVER_MOUNT_POINT_SIZE];

	for (int i = 0; i < path_parts_size; i++) {
		const uint32_t pp_len = strlen(path_parts[i]);
		const int remaining = size - (p - filepath);
		if (pp_len >= remaining - 1) {
			LOG_ERR("Given filepath too long");
			return -ENOMEM;
		}

		*p++ = '/';
		strncpy(p, path_parts[i], pp_len);
		p += pp_len;
	}

	*p = '\0';

	return 0;
}

/**
 * @brief Open file corresponding to the requested filepath.
 * 
 * Note: If size argument is not NULL, the file size is retrieved and returned.
 * 
 * @param file 
 * @param size 
 * @param filepath 
 * @return int 
 */
static int open_r_file(struct fs_file_t *file, size_t *size, char *filepath)
{
	int ret;

	fs_file_t_init(file);

	/* Get file size */
	if (size != NULL) {
		/* Get file stats */
		struct fs_dirent dirent;
		ret = fs_stat(filepath, &dirent);
		if (ret == 0) {
			if (dirent.type != FS_DIR_ENTRY_FILE) {
				ret = -ENOENT;
				goto exit;
			}

			*size = dirent.size;
		} else {
			LOG_ERR("Failed to get file stats ret=%d", ret);
			goto exit;
		}

	}

	/* Open file */
	ret = fs_open(file, filepath, FS_O_READ);
	if (ret != 0) {
		LOG_ERR("Failed to open file ret=%d", ret);
		goto exit;
	}

exit:

	return ret;
}

static int open_w_file(struct fs_file_t *file, char *filepath)
{
	int ret;

	/* Prepare the file in the filesystem */
	fs_file_t_init(file);

	ret = fs_open(file,
		      filepath,
		      FS_O_CREATE | FS_O_WRITE);
	if (ret != 0) {
		LOG_ERR("fs_open failed: %d", ret);
		goto ret;
	}

	/* Truncate file in case it already exists */
	ret = fs_truncate(file, 0U);
	if (ret != 0) {
		LOG_ERR("fs_truncate failed: %d", ret);
		goto exit;
	}

exit:
	if (ret != 0) {
		fs_close(file);
	}
ret:
	return ret;
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
 * 
 * TODO: Make this function compatible with non-stream requests
 */
int http_file_upload(struct http_request *req,
		     struct http_response *resp)
{
	static struct fs_file_t file;

	int ret = 0;

	/**
	 * @brief First call to the handler
	 */
	if (http_request_begins(req))
	{
		char *pp[FILES_SERVER_FILEPATH_MAX_DEPTH];
		char filepath[FILE_FILEPATH_MAX_LEN];

		/* Parse filepath in URL */
		int ret = req_get_arg_path_parts(req, pp, FILES_SERVER_FILEPATH_MAX_DEPTH);
		if (ret < 0) {
			http_request_discard(req, HTTP_REQUEST_BAD);
			goto exit;
		}

		ret = filepath_build(pp, ret, filepath, sizeof(filepath));
		if (ret < 0) {
			http_request_discard(req, HTTP_REQUEST_BAD);
			goto exit;
		}

#if FILES_SERVER_CREATE_DIR_IF_NOT_EXISTS
		ret = app_fs_mkdir_intermediate(filepath, true);
		if (ret < 0) {
			LOG_ERR("Failed to create intermediate directories: %s", filepath);
			http_request_discard(req, HTTP_REQUEST_PROCESSING_ERROR);
			goto exit;
		}
#endif /* FILES_SERVER_CREATE_DIR_IF_NOT_EXISTS */

		ret = open_w_file(&file, filepath);
		if (ret == -ENOENT) {
			http_request_discard(req, HTTP_REQUEST_BAD);
			ret = 0;
			goto exit;
		} else if (ret != 0) {
			http_request_discard(req, HTTP_REQUEST_BAD);
			ret = 0;
			goto exit;
		}
		
		/* Reference context */
		req->user_data = &file;

		LOG_INF("Start upload to %s", filepath);
	}

	/* Prepare data to be written */

	if (req->payload.loc != NULL) {
		LOG_HEXDUMP_INF(req->payload.loc, req->payload.len, "Payload");

		LOG_DBG("write loc=%p [%u] file=%p", req->payload.loc, req->payload.len, &file);
		ssize_t written = fs_write(&file, req->payload.loc, req->payload.len);
		if (written != req->payload.len) {
			ret = written;
			LOG_ERR("Failed to write file %d != %u",
				written, req->payload.len);
			http_request_discard(req, HTTP_REQUEST_BAD);
			goto exit;
		}
	}

	bool complete = http_request_complete(req);
	if (complete) {
		/* Close file */
		ret = fs_close(&file);
		if (ret) {
			// u->error = FILE_UPLOAD_FILE_CLOSE_FAILED;
			LOG_ERR("Failed to close file = %d", ret);
			goto ret;
		}

		LOG_INF("Upload of %u B succeeded", req->payload_len);
		
		if (Z_LOG_CONST_LEVEL_CHECK(LOG_LEVEL_DBG)) {
			app_fs_stats(CONFIG_FILES_SERVER_MOUNT_POINT);
		}

		/* TPDP Encode response payload */
	}

exit:
	/* In case of fatal error, properly close file */
	if (ret != 0) {
		fs_close(&file);
	}

ret:
	return ret;
}

/* TODO a file descriptor is not closed somewhere bellow, which causes 
 * file downloads to fails after ~4 downloads. */
int http_file_download(struct http_request *req,
		       struct http_response *resp)
{
	int ret = 0;
	static struct fs_file_t file;

	/* Init */
	if (http_response_is_first_call(resp)) {
		size_t filesize;

		char *pp[FILES_SERVER_FILEPATH_MAX_DEPTH];
		char filepath[FILE_FILEPATH_MAX_LEN];

		/* Parse filepath in URL */
		int ret = req_get_arg_path_parts(req, pp, FILES_SERVER_FILEPATH_MAX_DEPTH);
		if (ret < 0) {
			http_request_discard(req, HTTP_REQUEST_BAD);
			goto exit;
		}

		ret = filepath_build(pp, ret, filepath, sizeof(filepath));
		if (ret != 0) {
			http_response_set_status_code(resp,
						      HTTP_STATUS_BAD_REQUEST);
			ret = 0;
			goto exit;
		}

		ret = open_r_file(&file, &filesize, filepath);
		if (ret == -ENOENT) {
			http_response_set_status_code(resp,
						      HTTP_STATUS_NOT_FOUND);
			ret = 0;
			goto exit;
		} else if (ret != 0) {
			http_response_set_status_code(resp,
						      HTTP_STATUS_INTERNAL_SERVER_ERROR);
			ret = 0;
			goto exit;
		}

		http_response_set_content_length(resp, filesize);

		LOG_INF("Download %s [size=%u]", filepath, filesize);

		/* Reference context */
		req->user_data = &file;
	}

	/* Read & close */
	if (req->user_data != NULL) {
		ret = fs_read(&file, resp->buffer.data, resp->buffer.size);
		if (ret < 0) {
			fs_close(&file);
			http_response_set_status_code(
				resp, HTTP_STATUS_INTERNAL_SERVER_ERROR);
			ret = 0;
			goto exit;
		}

		resp->buffer.filling = ret;

		/* If more data are expected */
		if (ret == resp->buffer.size) {
			http_response_mark_not_complete(resp);
		} else {
			fs_close(&file);
			req->user_data = NULL;
		}

		ret = 0;
	}

exit:
	return ret;
}

int http_file_stats(struct http_request *req,
		    struct http_response *resp)
{
	return 0;
}