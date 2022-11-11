/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "routes.h"

#include <zephyr/kernel.h>
#include <assert.h>

#include <string.h>
#include <stdio.h>

#include <embedc/parser_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(routes, LOG_LEVEL_WRN);

extern const struct route_descr *const routes_root;
extern const size_t routes_root_size;

const struct route_descr *route_resolve(enum http_method method,
					char *url,
					struct route_parse_result *results,
					size_t *results_count,
					char **query_string)
{
	return route_tree_resolve(routes_root,
				  routes_root_size,
				  url,
				  http_method_to_route_flag(method),
				  METHODS_MASK,
				  results,
				  results_count,
				  query_string);

}

bool route_supports_streaming(const struct route_descr *route)
{
	return route->req_handler != NULL;
}

bool route_is_valid(const struct route_descr *route)
{
	return route && route->resp_handler != NULL;
}

http_content_type_t http_route_resp_default_content_type(const struct route_descr *route)
{
	http_content_type_t content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN;

	if (route) {
		switch (route->user_data & ROUTE_ATTR_MASK) {
		case ROUTE_ATTR_REST:
			content_type = HTTP_CONTENT_TYPE_APPLICATION_JSON;
			break;
		case ROUTE_ATTR_HTML:
			content_type = HTTP_CONTENT_TYPE_TEXT_HTML;
			break;
		case ROUTE_ATTR_BINARY:
			content_type = HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM;
			break;
		case ROUTE_ATTR_MULTIPART_FORM_DATA:
			content_type = HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA;
			break;
		case ROUTE_ATTR_TEXT:
		default:
			content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN;
			break;
		}
	}

	return content_type;
}

int http_routes_iterate(route_tree_iter_cb_t cb, void *arg)
{
	return route_tree_iterate(routes_root, routes_root_size, cb, arg);
}

uint32_t http_method_to_route_flag(enum http_method method)
{
	switch (method) {
	case HTTP_POST:
		return POST;
	case HTTP_GET:
		return GET;
	case HTTP_PUT:
		return PUT;
	case HTTP_DELETE:
		return DELETE;
	default:
		return 0;
	}
}

enum http_method http_route_flag_to_method(uint32_t flags)
{
	switch (flags & ROUTE_METHODS_MASK) {
	case POST:
		return HTTP_POST;
	case GET:
		return HTTP_GET;
	case PUT:
		return HTTP_PUT;
	case DELETE:
		return HTTP_DELETE;
	default:
		return 0xFFu;
	}
}
