/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file http_server.h
 * @brief
 * @version 0.1
 * @date 2021-11-07
 *
 * Requirements
 *
 * - webserver
 * - REST/JSON
 * - keep-alive
 * - client and server certificate
 */

#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include "http_utils.h"

#include <zephyr/kernel.h>

void http_server_get_stats(struct http_stats *dest);

#endif