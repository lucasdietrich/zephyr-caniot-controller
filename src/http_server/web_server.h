#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

#include <stdint.h>

#include "http_request.h"

int prometheus_metrics(struct http_request *req,
		       struct http_response *resp);

#endif