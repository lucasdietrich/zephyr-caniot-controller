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

#define REST REST_RESSOURCE
#define WEB WEB_RESSOURCE
#define PROM PROM_RESSOURCE

#define DELETE HTTP_DELETE
#define GET HTTP_GET
#define POST HTTP_POST
#define PUT HTTP_PUT

#define HTTP_ROUTE(m, r, h, t, c, k) \
	{ \
		.route = r, \
		.route_len = sizeof(r) - 1, \
		.method = m, \
		.server = t, \
		.handler = h, \
		.default_content_type = c, \
		.path_args_count = k \
	}

#define REST_RESSOURCE(m, r, h, k) HTTP_ROUTE(m, r, h, HTTP_REST_SERVER, HTTP_CONTENT_TYPE_APPLICATION_JSON, k)
#define WEB_RESSOURCE(m, r, h) HTTP_ROUTE(m, r, h, HTTP_WEB_SERVER, HTTP_CONTENT_TYPE_TEXT_HTML, 0U)
#define PROM_RESSOURCE(m, r, h) HTTP_ROUTE(m, r, h, HTTP_PROMETHEUS_CLIENT, HTTP_CONTENT_TYPE_TEXT_PLAIN, 0U)

static const struct http_route routes[] = {
	WEB(GET, "", web_server_index_html),
	WEB(GET, "/", web_server_index_html),
	WEB(GET, "/index.html", web_server_index_html),

	REST(GET, "/info", rest_info, 0U),

	REST(GET, "/devices", rest_devices_list, 0U),
	REST(GET, "/devices/xiaomi", rest_xiaomi_records, 0U),
	REST(GET, "/devices/caniot", rest_caniot_records, 0U),

	REST(GET, "/devices/garage", rest_devices_garage_get, 0U),
	REST(POST, "/devices/garage", rest_devices_garage_post, 0U),

	PROM(GET, "/metrics", prometheus_metrics),
	PROM(GET, "/metrics_controller", prometheus_metrics_controller),
	PROM(GET, "/metrics_demo", prometheus_metrics_demo),

	REST(GET, "/devices/caniot/%u/endpoints/%u/telemetry", rest_devices_caniot_telemetry, 2U),
	REST(GET, "/devices/caniot/%u/endpoints/%u/command", rest_devices_caniot_command, 2U),

	// REST(GET, "/devices/caniot/{did}/", rest_caniot_info),
	// REST(GET, "/devices/caniot/24/ll/3/query_telemetry", rest_caniot_query_telemetry),
	// REST(GET, "/devices/caniot/24/ll/3/command", rest_caniot_command),
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

		if (route->path_args_count == 0U) {
			if (url_match_noarg(route, url, url_len) == true) {
				return route;
			}
		} else if ((route->path_args_count == 1U) &&
			   (HTTP_ROUTE_ARGS_MAX_SIZE >= 1U)) {
			if (sscanf(url, route->route, &(*rargs)[0])
			    == route->path_args_count) {
				return route;
			}
		} else if ((route->path_args_count == 2U) &&
			   (HTTP_ROUTE_ARGS_MAX_SIZE >= 2U)) {
			if (sscanf(url, route->route, &(*rargs)[0], &(*rargs)[1])
			    == route->path_args_count) {
				return route;
			}
		} else if ((route->path_args_count == 3U) &&
			   (HTTP_ROUTE_ARGS_MAX_SIZE >= 3U)) {
			if (sscanf(url, route->route, &(*rargs)[0], &(*rargs)[1], &(*rargs)[2])
			    == route->path_args_count) {
				return route;
			}
		}
	}

	return NULL;
}

http_content_type_t http_route_get_default_content_type(const struct http_route *route)
{
	return route->default_content_type;
}