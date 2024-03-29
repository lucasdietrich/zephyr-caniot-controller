/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "net_time.h"

#include <stdint.h>

#include <zephyr/kernel.h>

static inline uint32_t sys_time_get(void)
{
	return net_time_get();
}

typedef union {
	atomic_t atomic;
	atomic_val_t atomic_val;
	struct {
		uint32_t has_ipv4_addr : 1;
		uint32_t valid_system_time : 1;
	};
} controller_status_t;

extern controller_status_t controller_status;

#endif