#ifndef _HTTP_SERVER_ROUTES_H_
#define _HTTP_SERVER_ROUTES_H_

#include <stddef.h>
#include <net/http_parser.h>

typedef enum {
	HTTP_REST_SERVER = 0,
	HTTP_WEB_SERVER
} http_server_t;

struct http_response;
struct http_request;

typedef int (*http_handler_t) (struct http_request *req,
			       struct http_response *resp);

struct http_route
{
	const char *route;
	size_t route_len;
	enum http_method method;
	http_server_t server;
	http_handler_t handler;
};

#define HTTP_ROUTE(m, r, h, t) \
	{ \
		.route = r, \
		.route_len = sizeof(r) - 1, \
		.method = m, \
		.server = t, \
		.handler = h \
	}

#define REST_RESSOURCE(m, r, h) HTTP_ROUTE(m, r, h, HTTP_REST_SERVER)
#define WEB_RESSOURCE(m, r, h) HTTP_ROUTE(m, r, h, HTTP_WEB_SERVER)

http_handler_t route_resolve(struct http_request *req);

#endif /* _HTTP_SERVER_ROUTES_H_ */