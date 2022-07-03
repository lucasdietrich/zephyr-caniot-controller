#ifndef _HTTP_SERVER_ROUTES_H_
#define _HTTP_SERVER_ROUTES_H_

#include <stddef.h>
#include <net/http_parser.h>

#include "http_utils.h"

typedef uint32_t http_route_args_t[3];

#define HTTP_ROUTE_ARGS_MAX_SIZE (sizeof(http_route_args_t)  / sizeof(uint32_t))

typedef enum {
	HTTP_REST_SERVER = 0,
	HTTP_WEB_SERVER,
	HTTP_PROMETHEUS_CLIENT
} http_server_t;

struct http_response;
struct http_request;

typedef int (*http_handler_t) (struct http_request *req,
			       struct http_response *resp);

struct http_route
{
	const char *route; /* route to match */
	size_t route_len; /* without the trailing \0 */
	enum http_method method; /* HTTP method */
	http_server_t server; /* HTTP_REST_SERVER, HTTP_WEB_SERVER, HTTP_PROMETHEUS_CLIENT */
	http_handler_t handler; /* handler to call */
	http_content_type_t default_content_type; /* route default content type */
	uint32_t path_args_count; /* number of arguments in path e.g. "/devices/caniot/%u/endpoints/%u/telemetry" */
};

typedef struct http_route http_route_t;

const struct http_route *route_resolve(enum http_method method,
				       const char *url,
				       size_t url_len,
				       http_route_args_t *rargs);

http_content_type_t http_route_get_default_content_type(const struct http_route *route);


#endif /* _HTTP_SERVER_ROUTES_H_ */