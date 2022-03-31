#ifndef _HTTP_PROMETHEUS_CLIENT_H_
#define _HTTP_PROMETHEUS_CLIENT_H_

#include <stddef.h>

#include "http_request.h"

#include "mydevices.h"

int prometheus_metrics_demo(struct http_request *req,
			    struct http_response *resp);

int prometheus_metrics(struct http_request *req,
		       struct http_response *resp);

const char *prom_myd_medium_to_str(mydevice_medium_type_t medium);

const char *prom_myd_device_type_to_str(mydevice_type_t device_type);

const char *prom_myd_sensor_type_to_str(mydevice_sensor_type_t sensor_type);

#endif /* _HTTP_PROMETHEUS_CLIENT_H_ */