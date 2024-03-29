/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file net_time.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Gather all functions to have a proper system time
 * @version 0.1
 * @date 2021-11-07
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef _NET_TIME_H_
#define _NET_TIME_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>

/**
 * @brief Schedule a work to sync the system time
 *
 * @return int
 */
int net_time_sync(void);

/**
 * @brief Get if the system time is synced
 *
 * @return uint64_t
 */
bool net_time_is_synced(void);

/**
 * @brief Get timestamp in seconds
 *
 * @return uint32_t
 * @retval 0 if not available
 */
uint32_t net_time_get(void);

/**
 * @brief Show the current time in format: YYYY-MM-DD HH:MM:SS
 */
void net_time_show(void);

/**
 * @brief Wait until the system time is synced.
 *
 * Note: Should be called from a thread.
 *
 * @param timeout
 * @return int
 */
int net_time_wait_synced(k_timeout_t timeout);

#endif