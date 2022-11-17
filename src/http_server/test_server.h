/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file test_server.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Test server, for testing HTTP server features
 * @version 0.1
 * @date 2022-07-15
 * 
 * @copyright Copyright (c) 2022
 * 
 * Tests to implement:
 * 	- Route matching (args, length, method) : write arguments back to the client
 * 	- File transfer (stream / chunk) : calculate checksum of payload and send it back to the client
 * 	- Payload size
 * 	- TLS
 * 	- Keep-alive
 */

#ifndef _HTTP_TEST_SERVER_H_
#define _HTTP_TEST_SERVER_H_

#include <stdint.h>
#include <stddef.h>

#include "http_utils.h"
#include "http_request.h"
#include "http_response.h"
#include "routes.h"

int http_test_any(struct http_request *req,
		  struct http_response *resp);
		  
int http_test_messaging(struct http_request *req,
			struct http_response *resp);

int http_test_streaming(struct http_request *req,
			struct http_response *resp);

int http_test_route_args(struct http_request *req,
			 struct http_response *resp);

int http_test_big_payload(struct http_request *req,
			  struct http_response *resp);

int http_test_headers(struct http_request *req,
		      struct http_response *resp);

int http_test_payload(struct http_request *req,
		      struct http_response *resp);

#endif /* _HTTP_TEST_SERVER_H_ */