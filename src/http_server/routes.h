#ifndef _HTTP_SERVER_ROUTES_H_
#define _HTTP_SERVER_ROUTES_H_

#include <stddef.h>
#include <net/http_parser.h>

#include "http_utils.h"

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
};

const struct http_route *route_resolve(struct http_request *req);

http_content_type_t http_get_route_default_content_type(const struct http_route *route);


#endif /* _HTTP_SERVER_ROUTES_H_ */