/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DEBUG_SERVER_H_
#define _DEBUG_SERVER_H_

#include "core/http_request.h"
#include "core/http_response.h"

#include <stdint.h>

int debug_server_ha_telemetry(http_request_t *request, http_response_t *response);

#endif