/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_FILES_SERVER_H_
#define _HTTP_FILES_SERVER_H_

#include "core/http_request.h"
#include "core/http_response.h"

int http_file_upload(struct http_request *req, struct http_response *resp);

int http_file_download(struct http_request *req, struct http_response *resp);

int http_file_stats(struct http_request *req, struct http_response *resp);

int http_file_delete(struct http_request *req, struct http_response *resp);

#endif /* _HTTP_FILES_SERVER_H_ */