/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_PROMETHEUS_CLIENT_H_
#define _HTTP_PROMETHEUS_CLIENT_H_

#include <stddef.h>

#include "http_request.h"
#include "http_response.h"

#include "ha/core/devices.h"

int prometheus_metrics_demo(http_request_t *req,
			    http_response_t *resp);

int prometheus_metrics(http_request_t *req,
		       http_response_t *resp);

int prometheus_metrics_controller(http_request_t *req,
				  http_response_t *resp);

const char *prom_myd_medium_to_str(ha_dev_medium_type_t medium);

const char *prom_myd_device_type_to_str(ha_dev_type_t device_type);

const char *prom_myd_sensor_type_to_str(ha_dev_sensor_type_t sensor_type);

#endif /* _HTTP_PROMETHEUS_CLIENT_H_ */