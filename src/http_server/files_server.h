/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_FILES_SERVER_H_
#define _HTTP_FILES_SERVER_H_

#include <stdint.h>

#include <data/json.h>
#include <net/http_parser.h>

#include "http_utils.h"
#include "http_request.h"

int http_file_upload(struct http_request *req,
		     struct http_response *resp);

#endif /* _HTTP_FILES_SERVER_H_ */