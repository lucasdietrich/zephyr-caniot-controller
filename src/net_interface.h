/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NET_INTERFACE_H_
#define _NET_INTERFACE_H_

#include <zephyr/net/net_if.h>

#define ETH_STR_LEN sizeof("FF:FF:FF:FF:FF:FF")

/**
 * @brief Initialize network management events (Ethernet, interface, DHCP)
 *
 * https://docs.zephyrproject.org/latest/reference/networking/net_mgmt.html#listening-to-network-events
 */
void net_interface_init(void);

/**
 * @brief Get interface status
 *
 * @param iface
 * @return const char* "up" if interface is up, "down" otherwise
 */
const char *net_interface_status_get(struct net_if *iface);

#endif