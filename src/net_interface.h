/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NET_INTERFACE_H_
#define _NET_INTERFACE_H_

/**
 * @brief Initialize network management events (Ethernet, interface, DHCP)
 * 
 * https://docs.zephyrproject.org/latest/reference/networking/net_mgmt.html#listening-to-network-events
 */
void net_interface_init(void);

/* TODO add function to get the ethernet interface directly without using net_if_get_default() */


#endif