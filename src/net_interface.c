/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "net_interface.h"
#include "net_time.h"
#include "userio/leds.h"

#include <zephyr/logging/log.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/sntp.h>

/* Increase Zephyr MGMT task size through CONFIG_NET_MGMT_EVENT_STACK_SIZE to 1024 when
 * increasing log level */
LOG_MODULE_REGISTER(netif, LOG_LEVEL_DBG);

#define NET_ETH_ADDR_STR_LEN sizeof("xx:xx:xx:xx:xx:xx")

#define ETH0 "eth0"
#define USB0 "netusb0"

static struct net_if *eth_iface;
#if defined(CONFIG_ETH_STM32_HAL)
static char mac_eth0_str[NET_ETH_ADDR_STR_LEN] = "00:80:E1:77:77:77";
#endif

#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
static struct net_if *usb_iface;
static char mac_usb0_str[NET_ETH_ADDR_STR_LEN] = "00:00:5E:00:53:00";
#endif

#define MGMT_ETHERNET_CB_INDEX	0u
#define MGMT_INTERFACE_CB_INDEX 1u
#define MGMT_IP_CB_INDEX		2u
#define MGMT_L4_CB_INDEX		3u
static struct net_mgmt_event_callback mgmt_cb[4u];

/* Forward declarations */
static void show_ipv4(struct net_if *iface);
static const char *net_mgmt_event_to_str(uint32_t mgmt_event);

static void eth_interface_up(struct net_if *iface)
{
#if !defined(CONFIG_QEMU_TARGET)
	leds_set_blinking(LED_NET, 200U * USEC_PER_MSEC);
#endif

#if defined(CONFIG_NET_DHCPV4)
	net_dhcpv4_start(iface);
#elif defined(CONFIG_QEMU_TARGET)
	net_time_sync();
#endif
}

static void eth_interface_down(struct net_if *iface)
{
#if defined(CONFIG_NET_DHCPV4)
	// net_dhcpv4_stop(iface);
#endif

#ifndef CONFIG_QEMU_TARGET
	leds_set(LED_NET, LED_OFF);
#endif
}

#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
static void usb_interface_up(struct net_if *iface)
{
	struct in_addr gw, my, nm;

	/* Configure the interface with a static IP address */
	net_addr_pton(AF_INET, "192.0.3.2", &my);
	net_addr_pton(AF_INET, "255.255.255.0", &nm);
	net_addr_pton(AF_INET, "192.0.3.1", &gw);


	net_if_ipv4_addr_add(iface, &my, NET_ADDR_MANUAL, 0);
	net_if_ipv4_set_netmask_by_addr(iface, &my, &nm);
	net_if_ipv4_set_gw(iface, &gw);
}

static void usb_interface_down(struct net_if *iface)
{
	(void)iface;
}

#endif

static void net_event_handler(struct net_mgmt_event_callback *cb,
							  uint32_t mgmt_event,
							  struct net_if *iface)
{
	LOG_DBG("[face: %p] event: %s (%x)", iface, net_mgmt_event_to_str(mgmt_event),
			mgmt_event);

	switch (mgmt_event) {

	/* doesn't have any effect
	 * https://github.com/zephyrproject-rtos/zephyr/blob/main/include/net/ethernet_mgmt.h
	 */
	case NET_EVENT_ETHERNET_CARRIER_ON:
		break;
	case NET_EVENT_ETHERNET_CARRIER_OFF:
		break;

	/* https://github.com/zephyrproject-rtos/zephyr/blob/main/include/net/net_event.h
	 */
	case NET_EVENT_IF_UP:
		if (iface == eth_iface) {
			eth_interface_up(eth_iface);
#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
		} else if (iface == usb_iface) {
			usb_interface_up(usb_iface);
#endif
		}
		break;

	case NET_EVENT_IF_DOWN:
		if (iface == eth_iface) {
			eth_interface_down(eth_iface);
#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
		} else if (iface == usb_iface) {
			usb_interface_down(usb_iface);
#endif
		}
		break;

	case NET_EVENT_IPV4_ADDR_ADD:
#ifndef CONFIG_QEMU_TARGET
		if (iface == eth_iface) {
			leds_set(LED_NET, LED_ON);
			net_time_sync();
		}
#endif
		show_ipv4(iface);
		break;
	case NET_EVENT_IPV4_ADDR_DEL:
		break;
	case NET_EVENT_IPV4_DHCP_START:
		break;
	case NET_EVENT_IPV4_DHCP_BOUND:
		break;
	case NET_EVENT_IPV4_DHCP_STOP:
		break;
	default:
		break;
	}
}

static int eth_mac_str_to_addr(struct net_linkaddr *ll_addr, const char *mac_str)
{
	int i;
	char *endptr;
	uint8_t mac[6];

	for (i = 0; i < 6; i++) {
		mac[i] = strtol(mac_str, &endptr, 16);
		if (endptr == mac_str || *endptr != (i < 5 ? ':' : '\0')) {
			return -EINVAL;
		}

		mac_str = endptr + 1;
	}

	memcpy(ll_addr->addr, mac, sizeof(mac));

	return 0;
}

#if defined(CONFIG_ETH_STM32_HAL) || defined(CONFIG_USB_DEVICE_NETWORK_ECM)
static struct net_if *net_if_get_by_mac_str(const char *mac_str)
{
	struct net_if *iface = NULL;
	struct net_linkaddr_storage ll_addr_storage;
	struct net_linkaddr ll_addr;

	ll_addr.len	 = 6u;
	ll_addr.type = NET_LINK_ETHERNET;
	ll_addr.addr = ll_addr_storage.addr;

	int ret = eth_mac_str_to_addr(&ll_addr, mac_str);
	if (ret == 0) {
		iface = net_if_get_by_link_addr(&ll_addr);
	} else {
		LOG_ERR("Invalid MAC address %s", mac_str);
	}

	return iface;
}
#endif

void net_interface_init(void)
{
	/* eth0 */

#if defined(CONFIG_ETH_STM32_HAL)
	eth_iface = net_if_get_by_mac_str(mac_eth0_str);

	LOG_DBG("[%u] mac: %s iface: %p up: %u", net_if_get_by_iface(eth_iface), mac_eth0_str, eth_iface,
			(uint32_t)net_if_flag_is_set(eth_iface, NET_IF_UP));

	/* Set default */
	net_if_set_default(eth_iface);

#elif defined(CONFIG_QEMU_TARGET)
	eth_iface = net_if_get_default();
#endif

	/* Manually triggers "IF_UP" events, because not triggered by net_mgmt
	 * if the interfaces were already up */
	if (net_if_flag_is_set(eth_iface, NET_IF_UP)) {
		eth_interface_up(eth_iface);
	}

	/* usb */
#if defined(CONFIG_USB_DEVICE_NETWORK)
	usb_iface = net_if_get_by_mac_str(mac_usb0_str);

	LOG_DBG("[%u] mac: %s iface: %p up: %u", net_if_get_by_iface(usb_iface), mac_usb0_str, usb_iface,
			(uint32_t)net_if_flag_is_set(usb_iface, NET_IF_UP));

	/* Manually triggers "IF_UP" events, because not triggered by net_mgmt
	 * if the interfaces were already up */
	if (net_if_flag_is_set(usb_iface, NET_IF_UP)) {
		usb_interface_up(usb_iface);
	}
#endif

	/* One callback per layer */
	net_mgmt_init_event_callback(&mgmt_cb[MGMT_ETHERNET_CB_INDEX], net_event_handler,
								 NET_EVENT_ETHERNET_CARRIER_ON |
									 NET_EVENT_ETHERNET_CARRIER_OFF);
	net_mgmt_add_event_callback(&mgmt_cb[MGMT_ETHERNET_CB_INDEX]);

	net_mgmt_init_event_callback(&mgmt_cb[MGMT_INTERFACE_CB_INDEX], net_event_handler,
								 NET_EVENT_IF_UP | NET_EVENT_IF_DOWN);
	net_mgmt_add_event_callback(&mgmt_cb[MGMT_INTERFACE_CB_INDEX]);

	net_mgmt_init_event_callback(
		&mgmt_cb[MGMT_IP_CB_INDEX], net_event_handler,
		NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL | NET_EVENT_IPV4_DHCP_START |
			NET_EVENT_IPV4_DHCP_BOUND | NET_EVENT_IPV4_DHCP_STOP);
	net_mgmt_add_event_callback(&mgmt_cb[MGMT_IP_CB_INDEX]);

	net_mgmt_init_event_callback(&mgmt_cb[MGMT_L4_CB_INDEX], net_event_handler,
								 NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED);
	net_mgmt_add_event_callback(&mgmt_cb[MGMT_L4_CB_INDEX]);

	conn_mgr_mon_resend_status();
}

static const char *addr_type_to_str(enum net_addr_type addr_type)
{
	switch (addr_type) {
	case NET_ADDR_ANY:
		return "NET_ADDR_ANY";
	case NET_ADDR_AUTOCONF:
		return "NET_ADDR_AUTOCONF";
	case NET_ADDR_DHCP:
		return "NET_ADDR_DHCP";
	case NET_ADDR_MANUAL:
		return "NET_ADDR_MANUAL";
	case NET_ADDR_OVERRIDABLE:
		return "NET_ADDR_OVERRIDABLE";
	default:
		return "<unknown>";
	}
}

static void show_ipv4(struct net_if *iface)
{
	struct net_if_config *const ifcfg = &iface->config;

	for (uint_fast16_t i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		char buf[NET_IPV4_ADDR_LEN];

		LOG_INF("=== NET interface %p ===", iface);

		LOG_INF("Address: %s [addr type %s]",
				net_addr_ntop(AF_INET, &ifcfg->ip.ipv4->unicast[i].ipv4.address.in_addr, buf,
							  sizeof(buf)),
				addr_type_to_str(ifcfg->ip.ipv4->unicast[i].ipv4.addr_type));

		LOG_INF("Subnet:  %s",
				net_addr_ntop(AF_INET, &ifcfg->ip.ipv4->unicast->netmask, buf, sizeof(buf)));
		LOG_INF("Router:  %s",
				net_addr_ntop(AF_INET, &ifcfg->ip.ipv4->gw, buf, sizeof(buf)));

#if defined(CONFIG_NET_DHCPV4)
		if (ifcfg->ip.ipv4->unicast[i].ipv4.addr_type == NET_ADDR_DHCP) {
			LOG_INF("DHCPv4 Lease time: %u seconds [state: %s]", ifcfg->dhcpv4.lease_time,
					net_dhcpv4_state_name(ifcfg->dhcpv4.state));
		}
#endif
	}
}

static const char *net_mgmt_event_to_str(uint32_t mgmt_event)
{
	switch (mgmt_event) {
	case NET_EVENT_ETHERNET_CARRIER_ON:
		return "NET_EVENT_ETHERNET_CARRIER_ON";
	case NET_EVENT_ETHERNET_CARRIER_OFF:
		return "NET_EVENT_ETHERNET_CARRIER_OFF";
	case NET_EVENT_IF_UP:
		return "NET_EVENT_IF_UP";
	case NET_EVENT_IF_DOWN:
		return "NET_EVENT_IF_DOWN";
	case NET_EVENT_IPV4_ADDR_ADD:
		return "NET_EVENT_IPV4_ADDR_ADD";
	case NET_EVENT_IPV4_ADDR_DEL:
		return "NET_EVENT_IPV4_ADDR_DEL";
	case NET_EVENT_IPV4_DHCP_START:
		return "NET_EVENT_IPV4_DHCP_START";
	case NET_EVENT_IPV4_DHCP_BOUND:
		return "NET_EVENT_IPV4_DHCP_BOUND";
	case NET_EVENT_IPV4_DHCP_STOP:
		return "NET_EVENT_IPV4_DHCP_STOP";
	case NET_EVENT_L4_CONNECTED:
		return "NET_EVENT_L4_CONNECTED";
	case NET_EVENT_L4_DISCONNECTED:
		return "NET_EVENT_L4_DISCONNECTED";
	default:
		return "<unknown net event>";
	}
}

const char *net_interface_status_get(struct net_if *iface)
{
	if (net_if_flag_is_set(iface, NET_IF_UP)) {
		return "up";
	} else {
		return "down";
	}
}