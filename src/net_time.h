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

#include <stdint.h>

void net_time_sync(void);

void net_time_show(void);

/**
 * @brief Get timestamp in seconds
 * 
 * @return uint32_t 
 */
uint32_t net_time_get(void);

#endif