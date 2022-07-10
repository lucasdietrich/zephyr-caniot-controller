#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

#include <stdint.h>

#include "http_request.h"

int web_server_index_html(http_request_t *req,
			  http_response_t *resp);

#endif