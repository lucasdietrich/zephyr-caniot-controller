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

#define HTTP_ROUTE(m, r, h, t, c, k, s) \
	{ \
		.route = r, \
		.route_len = sizeof(r) - 1, \
		.method = m, \
		.support_streaming = s, \
		.server = t, \
		.handler = h, \
		.default_content_type = c, \
		.path_args_count = k \
	}

#define MESSAGING_RESSOURCE(m, r, h, t, c, k) HTTP_ROUTE(m, r, h, t, c, k, false)
#define STREAMING_RESSOURCE(m, r, h, t, c, k) HTTP_ROUTE(m, r, h, t, c, k, true)

#define REST_RESSOURCE(m, r, h, k) MESSAGING_RESSOURCE(m, r, h, HTTP_REST_SERVER, HTTP_CONTENT_TYPE_APPLICATION_JSON, k)
#define WEB_RESSOURCE(m, r, h) MESSAGING_RESSOURCE(m, r, h, HTTP_WEB_SERVER, HTTP_CONTENT_TYPE_TEXT_HTML, 0U)
#define PROM_RESSOURCE(m, r, h) MESSAGING_RESSOURCE(m, r, h, HTTP_PROMETHEUS_CLIENT, HTTP_CONTENT_TYPE_TEXT_PLAIN, 0U)
#define FILE_RESSOURCE(m, r, h, k) STREAMING_RESSOURCE(m, r, h, HTTP_FILES_SERVER, HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA, k)

static const struct http_route routes[] = {
	WEB(GET, "", web_server_index_html),
	WEB(GET, "/", web_server_index_html),
	WEB(GET, "/index.html", web_server_index_html),

	REST(GET, "/info", rest_info, 0U),

	PROM(GET, "/metrics", prometheus_metrics),
	PROM(GET, "/metrics_controller", prometheus_metrics_controller),
	PROM(GET, "/metrics_demo", prometheus_metrics_demo),

	REST(GET, "/devices", rest_devices_list, 0U),
	REST(GET, "/devices/xiaomi", rest_xiaomi_records, 0U),
	REST(GET, "/devices/caniot", rest_caniot_records, 0U),

	FILE_RESSOURCE(POST, "/files", http_file_upload, 0U),
#if defined(CONFIG_CANIOT_CONTROLLER)
	REST(GET, "/devices/garage", rest_devices_garage_get, 0U),
	REST(POST, "/devices/garage", rest_devices_garage_post, 0U),

	REST(GET, "/devices/caniot/%u/endpoints/%u/telemetry", rest_devices_caniot_telemetry, 2U),
	REST(POST, "/devices/caniot/%u/endpoints/%u/command", rest_devices_caniot_command, 2U),

	REST(GET, "/devices/caniot/%u/attributes/%x", rest_devices_caniot_attr_read, 2U),
	REST(PUT, "/devices/caniot/%u/attributes/%x", rest_devices_caniot_attr_write, 2U),
#endif

#if defined(CONFIG_HTTP_TEST_SERVER)
	HTTP_TEST_MESSAGING_ROUTE(),
	HTTP_TEST_STREAMING_ROUTE(),
	HTTP_TEST_BIG_PAYLOAD_ROUTE(),
	HTTP_TEST_STREAMING_ROUTE_ARGS(),
	HTTP_TEST_HEADERS(),
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
			    const char *url, size_t url_len)
{
	if (res->route_len != url_len)
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

		/* use va_list */

		if (route->path_args_count == 0U) {
			if (url_match_noarg(route, url, url_len) == true) {
				return route;
			}
		} else if ((route->path_args_count == 1U) &&
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
	}

	return NULL;
}

bool route_is_valid(const struct http_route *route)
{
	return (route != NULL) && (route->handler != NULL);
}

http_content_type_t http_route_resp_default_content_type(const struct http_route *route)
{
	return route->default_content_type;
}