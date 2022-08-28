/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "routes.h"

#include <zephyr.h>
#include <assert.h>

#include <string.h>
#include <stdio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(routes, LOG_LEVEL_WRN);

#include "rest_server.h"
#include "web_server.h"
#include "prometheus_client.h"
#include "files_server.h"
#include "test_server.h"
#include "http_utils.h"

#define REST REST_RESSOURCE
#define WEB WEB_RESSOURCE
#define PROM PROM_RESSOURCE
#define FILE FILE_RESSOURCE

#define DELETE HTTP_DELETE
#define GET HTTP_GET
#define POST HTTP_POST
#define PUT HTTP_PUT

#define HTTP_ROUTE(_m, _r, _qh, _rh, _t, _c, _k, _mt) \
	{ \
		.route = _r, \
		.route_len = sizeof(_r) - 1, \
		.method = _m, \
		.server = _t, \
		.req_handler = _qh, \
		.resp_handler = _rh, \
		.default_content_type = _c, \
		.path_args_count = _k, \
		.match_type = _mt \
	}

#define MESSAGING_RESSOURCE(m, r, h, t, c, k, mt) \
	HTTP_ROUTE(m, r, NULL, h, t, c, k, mt)
#define STREAMING_RESSOURCE(m, r, qh, rh, t, c, k, mt) \
	HTTP_ROUTE(m, r, qh, rh, t, c, k, mt)

#define REST_RESSOURCE(m, r, h, k, mt) \
	MESSAGING_RESSOURCE(m, r, h, HTTP_REST_SERVER, HTTP_CONTENT_TYPE_APPLICATION_JSON, k, mt)
#define WEB_RESSOURCE(m, r, h, mt) \
	MESSAGING_RESSOURCE(m, r, h, HTTP_WEB_SERVER, HTTP_CONTENT_TYPE_TEXT_HTML, 0, mt)
#define PROM_RESSOURCE(m, r, h) \
	MESSAGING_RESSOURCE(m, r, h, HTTP_PROMETHEUS_CLIENT, HTTP_CONTENT_TYPE_TEXT_PLAIN, 0, HTTP_ROUTE_MATCH_EXACT_NOARGS)
#define FILE_RESSOURCE(m, r, qh, rh, k, mt) \
	STREAMING_RESSOURCE(m, r, qh, rh, HTTP_FILES_SERVER, HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA, k, mt)

/**
 * @brief TODO represent the routes as a tree
 */
static const struct http_route routes[] = {
	WEB(GET, "", web_server_index_html, HTTP_ROUTE_MATCH_EXACT_NOARGS),
	WEB(GET, "/", web_server_index_html, HTTP_ROUTE_MATCH_EXACT_NOARGS),
	WEB(GET, "/index.html", web_server_index_html, HTTP_ROUTE_MATCH_EXACT_NOARGS),
	WEB(GET, "/fetch", web_server_files_html, HTTP_ROUTE_MATCH_LEASE_NOARGS),

	REST(GET, "/info", rest_info, 0U, HTTP_ROUTE_MATCH_EXACT_NOARGS),

	PROM(GET, "/metrics", prometheus_metrics),
	PROM(GET, "/metrics_controller", prometheus_metrics_controller),
	PROM(GET, "/metrics_demo", prometheus_metrics_demo),

	REST(GET, "/devices", rest_devices_list, 0U, HTTP_ROUTE_MATCH_EXACT_NOARGS),
	REST(GET, "/room/%u", rest_room_devices_list, 1U, HTTP_ROUTE_MATCH_EXACT_WITHARGS),
	REST(GET, "/devices/xiaomi", rest_xiaomi_records, 0U, HTTP_ROUTE_MATCH_EXACT_NOARGS),
	REST(GET, "/devices/caniot", rest_caniot_records, 0U, HTTP_ROUTE_MATCH_EXACT_NOARGS),

	FILE_RESSOURCE(POST, "/files", http_file_upload, http_file_upload, 0U, HTTP_ROUTE_MATCH_LEASE_NOARGS),
	FILE_RESSOURCE(POST, "/files/", http_file_upload, http_file_upload, 0U, HTTP_ROUTE_MATCH_LEASE_NOARGS),
	FILE_RESSOURCE(GET, "/files/", NULL, http_file_download, 0u, HTTP_ROUTE_MATCH_LEASE_NOARGS),

	REST(GET, "/files/lua", rest_fs_list_lua_scripts, 0U, HTTP_ROUTE_MATCH_EXACT_NOARGS),
	REST(DELETE, "/files/lua", rest_fs_remove_lua_script, 0U, HTTP_ROUTE_MATCH_EXACT_NOARGS),

	REST(POST, "/lua/execute", rest_lua_run_script, 0U, HTTP_ROUTE_MATCH_EXACT_NOARGS),

#if defined(CONFIG_CANIOT_CONTROLLER)
	REST(GET, "/devices/garage", rest_devices_garage_get, 0U, HTTP_ROUTE_MATCH_EXACT_NOARGS),
	REST(POST, "/devices/garage", rest_devices_garage_post, 0U, HTTP_ROUTE_MATCH_EXACT_NOARGS),

	REST(GET, "/devices/caniot/%u/endpoint/%u/telemetry", rest_devices_caniot_telemetry, 2U, HTTP_ROUTE_MATCH_EXACT_WITHARGS),
	REST(POST, "/devices/caniot/%u/endpoint/%u/command", rest_devices_caniot_command, 2U, HTTP_ROUTE_MATCH_EXACT_WITHARGS),
	REST(POST, "/devices/caniot/%u/endpoint/blc/command", rest_devices_caniot_blc_command, 1U, HTTP_ROUTE_MATCH_EXACT_WITHARGS),

	REST(GET, "/devices/caniot/%u/attribute/%x", rest_devices_caniot_attr_read, 2U, HTTP_ROUTE_MATCH_EXACT_WITHARGS),
	REST(PUT, "/devices/caniot/%u/attribute/%x", rest_devices_caniot_attr_write, 2U, HTTP_ROUTE_MATCH_EXACT_WITHARGS),
#endif

#if defined(CONFIG_HTTP_TEST_SERVER)
	HTTP_TEST_MESSAGING_ROUTE(),
	HTTP_TEST_STREAMING_ROUTE(),
	HTTP_TEST_BIG_PAYLOAD_ROUTE(),
	HTTP_TEST_STREAMING_ROUTE_ARGS(),
	HTTP_TEST_HEADERS(),
	HTTP_TEST_PAYLOAD(),
#endif 
};



static inline const struct http_route *first(void)
{
	return routes;
}

static inline const struct http_route *last(void)
{
	return &routes[ARRAY_SIZE(routes) - 1];
}

static bool url_match_noarg(const struct http_route *res,
			    const char *url, size_t url_len,
			    bool exact)
{
	if (exact && (res->route_len != url_len))
		return false;

	return strncmp(res->route, url, res->route_len) == 0;
}

const struct http_route *route_resolve(enum http_method method,
				       const char *url,
				       size_t url_len,
				       http_route_args_t *rargs)
{
	__ASSERT_NO_MSG(url != NULL);
	__ASSERT_NO_MSG(rargs != NULL);

	/* regular routes */
	for (const struct http_route *route = first(); route <= last(); route++) {
		if (route->method != method) {
			continue;
		}

		switch (route->match_type) {
		case HTTP_ROUTE_MATCH_EXACT_NOARGS:
			if (url_match_noarg(route, url, url_len, true)) {
				return route;
			}
			break;
		case HTTP_ROUTE_MATCH_LEASE_NOARGS:
			if (url_match_noarg(route, url, url_len, false)) {
				return route;
			}
			break;
		case HTTP_ROUTE_MATCH_EXACT_WITHARGS:
			if ((route->path_args_count == 1U) &&
			    (HTTP_ROUTE_ARGS_MAX_COUNT >= 1U)) {
				if (sscanf(url, route->route, &(*rargs)[0])
				    == route->path_args_count) {
					return route;
				}
			} else if ((route->path_args_count == 2U) &&
				   (HTTP_ROUTE_ARGS_MAX_COUNT >= 2U)) {
				if (sscanf(url, route->route, &(*rargs)[0], &(*rargs)[1])
				    == route->path_args_count) {
					return route;
				}
			} else if ((route->path_args_count == 3U) &&
				   (HTTP_ROUTE_ARGS_MAX_COUNT >= 3U)) {
				if (sscanf(url, route->route, &(*rargs)[0], &(*rargs)[1], &(*rargs)[2])
				    == route->path_args_count) {
					return route;
				}
			}
			break;
		}
	}

	return NULL;
}

bool route_is_valid(const struct http_route *route)
{
	return (route != NULL) && (route->resp_handler != NULL);
}

http_content_type_t http_route_resp_default_content_type(const struct http_route *route)
{
	return route->default_content_type;
}

int http_route_iterate(bool(*cb)(const struct http_route *route, void *arg),
		       void *arg)
{
	uint32_t count = 0u;
	for (const struct http_route *route = first(); route <= last(); route++) {
		if (cb(route, arg) == false) {
			return -EINVAL;
		}
		count++;
	}

	return count;
}