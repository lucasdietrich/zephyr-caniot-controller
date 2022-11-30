/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <stdint.h>

#include <zephyr/kernel.h>

#include "net_time.h"

static inline uint32_t sys_time_get(void)
{
	return net_time_get();
}

typedef enum {
	SYSEV_IF_NET = 0u,
	SYSEV_IF_CAN,
	SYSEV_IF_BLE,

	_SYSEV_IF_COUNT
} sysev_if_t;

typedef enum {

	SYSEV_IF_DOWN = 0u,
	SYSEV_IF_UP,
	SYSEV_IF_FATAL_ERROR,
	SYSEV_IF_PENDING, /* transitionnal state */
	SYSEV_IF_RX_TX,

	_SYSEV_IF_EV_COUNT
} sysev_if_ev_t;

/**
 * @brief Notify a system event
 * 
 * Note: Cannot be called from ISR context
 * 
 * @param sysev Interface
 * @param if Event
 * @param data Related event data
 */
void sysev_notify(sysev_if_t iface, sysev_if_ev_t ev, void *data);

sysev_if_ev_t sysev_iface_get_state(sysev_if_t iface);

const char *sysev_iface_to_str(sysev_if_t iface);

const char *sysev_iface_ev_to_str(sysev_if_ev_t ev);

#endif