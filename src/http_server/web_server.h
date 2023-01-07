/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

#include "core/http_request.h"
#include "core/http_response.h"

#include <stdint.h>

int web_server_index_html(http_request_t *req, http_response_t *resp);

int web_server_files_html(http_request_t *req, http_response_t *resp);

#endif