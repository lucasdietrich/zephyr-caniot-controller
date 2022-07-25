/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_SERVER_ROUTES_H_
#define _HTTP_SERVER_ROUTES_H_

#include <stddef.h>
#include <net/http_parser.h>

#include "http_utils.h"

#define HTTP_ROUTE_ARGS_MAX_COUNT 3

typedef uint32_t http_route_args_t[HTTP_ROUTE_ARGS_MAX_COUNT];

typedef enum {
	HTTP_REST_SERVER = 0,
	HTTP_WEB_SERVER,
	HTTP_PROMETHEUS_CLIENT,
	HTTP_FILES_SERVER,
	HTTP_TEST_SERVER,
} http_server_t;

struct http_request;
struct http_response;
 
/**
 * @brief Route handler function.
 * @param req The request (NULL when finished to process a stream request)
 * @param res The response (NULL when processing a stream chunk)
 * @param args The route arguments.
 * @return 0 on success, any other value on error.
 */
typedef int (*http_handler_t) (struct http_request *__restrict req,
			       struct http_response *__restrict resp);

struct http_route
{
	/**
	 * @brief Route pattern to match
	 * - If path_args_count = 0, the route is a static route
	 * - If path_args_count > 0, there are variables in the route that we need to parse
	 */
	const char *route;

	/**
	 * @brief Route pattern length
	 */
	size_t route_len;

	/**
	 * @brief Number of variables in the route, 0 if static route
	 * 
	 * Note: e.g. /devices/caniot/%u/endpoints/%u/telemetry
	 */
	uint32_t path_args_count; 

	/**
	 * @brief HTTP method of the request
	 */
	enum http_method method;

	/**
	 * @brief Tells if route support HTTP stream request
	 */
	uint8_t support_streaming: 1;

	/**
	 * @brief Tells if the route support HTTP message requests
	 */
	// uint8_t support_messaging: 1;

	/*___________________________________________________________________*/

	/**
	 * @brief Server to which the route belongs, in order
	 *  to forward the request to the correct service.
	 * 
	 * Note: Can be :
	 * 	- HTTP_REST_SERVER
	 * 	- HTTP_WEB_SERVER
	 * 	- HTTP_PROMETHEUS_CLIENT
	 * 	- HTTP_FILES_SERVER
	 */
	http_server_t server;

	/**
	 * @brief Handler to call to process the request
	 */
	http_handler_t handler;

	/*___________________________________________________________________*/

	/**
	 * @brief Route response default content-type
	 */
	http_content_type_t default_content_type;
};

typedef struct http_route http_route_t;

/**
 * @brief Resolve the route in function of the tuple (url, method)
 * 
 * @param method HTTP method
 * @param url Actual request URL 
 * @param url_len URL length
 * @param rargs Array of uint32_t to store the resolved arguments of the route (check http_route_args_t)
 * @return const struct http_route*
 * @retval NULL if no route match
 */
const struct http_route *route_resolve(enum http_method method,
				       const char *url,
				       size_t url_len,
				       http_route_args_t *rargs);

bool route_is_valid(const struct http_route *route);

http_content_type_t http_route_resp_default_content_type(const struct http_route *route);

#endif /* _HTTP_SERVER_ROUTES_H_ */