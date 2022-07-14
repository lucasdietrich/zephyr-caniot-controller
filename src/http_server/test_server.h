#ifndef _HTTP_TEST_SERVER_H_
#define _HTTP_TEST_SERVER_H_

#include <stdint.h>
#include <stddef.h>

#include "http_utils.h"
#include "http_request.h"
#include "routes.h"

#define HTTP_TEST_MESSAGING_ROUTE MESSAGING_RESSOURCE(\
	GET, \
	"/test/messaging", \
	http_test_messaging, \
	HTTP_TEST_SERVER, \
	HTTP_CONTENT_TYPE_APPLICATION_JSON, \
	0U\
)

#define HTTP_TEST_STREAMING_ROUTE STREAMING_RESSOURCE(\
	POST, \
	"/test/streaming", \
	http_test_streaming, \
	HTTP_TEST_SERVER, \
	HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA, \
	0U\
)

#define HTTP_TEST_STREAMING_ROUTE_ARGS MESSAGING_RESSOURCE(\
	GET, \
	"/test/route_args/%u/%u/%u", \
	http_test_route_args, \
	HTTP_TEST_SERVER, \
	HTTP_CONTENT_TYPE_APPLICATION_JSON, \
	3U\
)

int http_test_messaging(struct http_request *req,
			struct http_response *resp);

int http_test_streaming(struct http_request *req,
			struct http_response *resp);

int http_test_route_args(struct http_request *req,
			 struct http_response *resp);

#endif /* _HTTP_TEST_SERVER_H_ */