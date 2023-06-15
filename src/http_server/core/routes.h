/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_SERVER_ROUTES_H_
#define _HTTP_SERVER_ROUTES_H_

#include "http_utils.h"

#include <stddef.h>

#include <zephyr/net/http/parser.h>

#include <embedc-url/parser.h>

#define ROUTE_ATTR_REST				   (0x0u << 0u)
#define ROUTE_ATTR_HTML				   (0x1u << 0u)
#define ROUTE_ATTR_TEXT				   (0x2u << 0u)
#define ROUTE_ATTR_FORM				   (0x3u << 0u)
#define ROUTE_ATTR_MULTIPART_FORM_DATA (0x4u << 0u)
#define ROUTE_ATTR_BINARY			   (0x5u << 0u)

#define ROUTE_ATTR_SECURE (0x8u << 0u)

#define ROUTE_ATTR_MASK (0xFu << 0u)

struct http_request;
struct http_response;

/**
 * @brief Route handler function.
 * @param req The request (NULL when finished to process a stream request)
 * @param res The response (NULL when processing a stream chunk)
 * @param args The route arguments.
 * @return 0 on success, any other value on error.
 */
typedef int (*http_handler_t)(struct http_request *__restrict req,
							  struct http_response *__restrict resp);

/**
 * @brief Resolve the route in function of the tuple (url, method)
 *
 * @param method HTTP method
 * @param url Actual request URL
 * @param results Array of uint32_t to store the resolved arguments of the route
 * @return size_t *results size, number of resolved parts is returned in
 * *results
 * @retval NULL if no route match
 */
const struct route_descr *route_resolve(enum http_method method,
										char *url,
										struct route_parse_result *results,
										size_t *results_count,
										char **query_string);

/**
 * @brief Check if the route supports streaming (Chunked Transfer Encoding)
 *
 * @param route
 * @return true
 * @return false
 */
bool route_supports_streaming(const struct route_descr *route);

/**
 * @brief Check if the route is valid
 *
 * @param route
 * @return true
 * @return false
 */
bool route_is_valid(const struct route_descr *route);

/**
 * @brief Get route default response content type
 *
 * @param route
 * @return http_content_type_t
 */
http_content_type_t http_route_resp_default_content_type(const struct route_descr *route);

/**
 * @brief Iterate over all routes
 *
 * @param route
 * @return http_content_type_t
 */
int http_routes_iterate(route_tree_iter_cb_t cb, void *arg);

static inline http_handler_t route_get_req_handler(const struct route_descr *route)
{
	return (http_handler_t)route->req_handler;
}

static inline http_handler_t route_get_resp_handler(const struct route_descr *route)
{
	return (http_handler_t)route->resp_handler;
}

uint32_t http_method_to_route_flag(enum http_method method);

enum http_method http_route_flag_to_method(uint32_t flags);

static inline enum http_method http_route_get_method(const struct route_descr *route)
{
	return http_route_flag_to_method(route->flags);
}

#endif /* _HTTP_SERVER_ROUTES_H_ */