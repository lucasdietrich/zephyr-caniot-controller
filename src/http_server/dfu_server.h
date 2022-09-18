/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_DFU_SERVER_H_
#define _HTTP_DFU_SERVER_H_

#include "http_request.h"
#include "http_response.h"


int http_dfu_status(struct http_request *req,
		    struct http_response *resp);

int http_dfu_image_upload(struct http_request *req,
			  struct http_response *resp);

int http_dfu_image_upload_response(struct http_request *req,
				   struct http_response *resp);

#endif /* _HTTP_DFU_SERVER_H_ */