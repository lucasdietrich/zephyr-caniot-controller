#ifndef _HTTP_PROMETHEUS_CLIENT_H_
#define _HTTP_PROMETHEUS_CLIENT_H_

#include <stddef.h>

#include "http_request.h"

int prometheus_metrics(struct http_request *req,
		       struct http_response *resp);

#endif /* _HTTP_PROMETHEUS_CLIENT_H_ */