#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

#include <stdint.h>

#include "http_request.h"

int web_server_index_html(struct http_request *req,
			  struct http_response *resp);

#endif