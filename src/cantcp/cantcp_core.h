/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANTCP_CORE_H_
#define _CANTCP_CORE_H_

#include <kernel.h>

#include "cantcp.h"

int cantcp_core_tunnel_init(cantcp_tunnel_t *tunnel);

int cantcp_core_send_frame(cantcp_tunnel_t *tunnel, struct zcan_frame *msg);

int cantcp_core_recv_frame(cantcp_tunnel_t *tunnel, struct zcan_frame *msg);

#endif /* _CANTCP_CORE_H_ */