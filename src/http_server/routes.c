#include "routes.h"

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

#define HTTP_ROUTE(m, r, h, t, c) \
	{ \
		.route = r, \
		.route_len = sizeof(r) - 1, \
		.method = m, \
		.server = t, \
		.handler = h, \
		.default_content_type = c \
	}

#define REST_RESSOURCE(m, r, h) HTTP_ROUTE(m, r, h, HTTP_REST_SERVER, HTTP_CONTENT_TYPE_APPLICATION_JSON)
#define WEB_RESSOURCE(m, r, h) HTTP_ROUTE(m, r, h, HTTP_WEB_SERVER, HTTP_CONTENT_TYPE_TEXT_HTML)
#define PROM_RESSOURCE(m, r, h) HTTP_ROUTE(m, r, h, HTTP_PROMETHEUS_CLIENT, HTTP_CONTENT_TYPE_TEXT_PLAIN)

static const struct http_route routes[] = {
	WEB(GET, "", web_server_index_html),
	WEB(GET, "/", web_server_index_html),
	WEB(GET, "/index.html", web_server_index_html),

	REST(GET, "/info", rest_info),

	REST(GET, "/devices/caniot/{did}/", rest_caniot_info),
	REST(GET, "/devices/caniot/24/ll/3/query_telemetry", rest_caniot_query_telemetry),
	REST(GET, "/devices/caniot/24/ll/3/command", rest_caniot_command),

	REST(GET, "/devices", rest_devices_list),
	REST(GET, "/devices/xiaomi", rest_xiaomi_records),
	REST(GET, "/devices/caniot", rest_caniot_records),

	PROM(GET, "/metrics", prometheus_metrics),
	PROM(GET, "/metrics_controller", prometheus_metrics_controller),
	PROM(GET, "/metrics_demo", prometheus_metrics_demo),
};

static inline const struct http_route *first(void)
{
	return routes;
}

static inline const struct http_route *last(void)
{
	return &routes[ARRAY_SIZE(routes) - 1];
}

static bool url_match(const struct http_route *res,
		      const char *url, size_t url_len)
{
	if (res->route_len != url_len)
		return false;

	return strncmp(res->route, url, res->route_len) == 0;
}

const struct http_route *route_resolve(struct http_request *req)
{
	for (const struct http_route *route = first(); route <= last(); route++) {
		if (route->method != req->method) {
			continue;
		}

		if (url_match(route, req->url, req->url_len)) {

			return route;
		}
	}

	return NULL;
}

http_content_type_t http_get_route_default_content_type(const struct http_route *route)
{
	return route->default_content_type;
}