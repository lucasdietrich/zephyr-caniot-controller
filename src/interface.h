#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/net_config.h>
#include <net/ethernet_mgmt.h>
#include <net/net_config.h>

/**
 * @brief Initialize network management events (Ethernet, interface, DHCP)
 * 
 * https://docs.zephyrproject.org/latest/reference/networking/net_mgmt.html#listening-to-network-events
 */
void net_init(void);

#endif