#include "net_interface.h"

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/net_config.h>
#include <net/ethernet_mgmt.h>
#include <net/net_config.h>
#include <net/sntp.h>

#include "net_time.h"
#include "user_io.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ethernet_if, LOG_LEVEL_INF);

static struct net_mgmt_event_callback mgmt_cb[3];

static void show_ipv4(void)
{
        struct net_if_config *const ifcfg = &net_if_get_default()->config;

        for (uint_fast16_t i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
                char buf[NET_IPV4_ADDR_LEN];

                if (ifcfg->ip.ipv4->unicast[i].addr_type != NET_ADDR_DHCP) {
                        LOG_WRN("not NET_ADDR_DHCP address (type = %d)",
                                ifcfg->ip.ipv4->unicast[i].addr_type);
                        continue;
                }

                char *ipv4_str = net_addr_ntop(AF_INET,
                                               &ifcfg->ip.ipv4->unicast[i].address.in_addr,
                                               buf, sizeof(buf));
                LOG_INF("Your address: %s", log_strdup(ipv4_str));
                LOG_INF("Lease time: %u seconds", ifcfg->dhcpv4.lease_time);
                LOG_INF("Subnet: %s",
                        log_strdup(net_addr_ntop(AF_INET,
                                                 &ifcfg->ip.ipv4->netmask,
                                                 buf, sizeof(buf))));
                LOG_INF("Router: %s",
                        log_strdup(net_addr_ntop(AF_INET,
                                                 &ifcfg->ip.ipv4->gw,
                                                 buf, sizeof(buf))));
        }
}

static void net_event_handler(struct net_mgmt_event_callback *cb,
                       uint32_t mgmt_event,
                       struct net_if *iface)
{
        switch (mgmt_event) {

        /* doesn't have any effect
         * https://github.com/zephyrproject-rtos/zephyr/blob/main/include/net/ethernet_mgmt.h
         */
        case NET_EVENT_ETHERNET_CARRIER_ON:
                LOG_DBG("NET_EVENT_ETHERNET_CARRIER_ON (%u)", mgmt_event);
                break;
        case NET_EVENT_ETHERNET_CARRIER_OFF:
                LOG_DBG("NET_EVENT_ETHERNET_CARRIER_OFF (%u)", mgmt_event);
                break;
        
        /* https://github.com/zephyrproject-rtos/zephyr/blob/main/include/net/net_event.h */
        case NET_EVENT_IF_UP:
        {
                LOG_DBG("NET_EVENT_IF_UP (%u)", mgmt_event);
                led_set_mode(&leds.net, BLINKING_5Hz);
                break;
        }
        case NET_EVENT_IF_DOWN:
        {
                LOG_DBG("NET_EVENT_IF_DOWN (%u)", mgmt_event);
                led_set_mode(&leds.net, OFF);
                break;
        }
        case NET_EVENT_IPV4_ADDR_ADD:
        {
                LOG_DBG("NET_EVENT_IPV4_ADDR_ADD (%u)", mgmt_event);
                led_set_mode(&leds.net, STEADY);
                net_time_sync();
                show_ipv4();
                break;
        }
        case NET_EVENT_IPV4_ADDR_DEL:
                LOG_INF("NET_EVENT_IPV4_ADDR_DEL (%u)", mgmt_event);
                break;
        case NET_EVENT_IPV4_DHCP_START:
                LOG_DBG("NET_EVENT_IPV4_DHCP_START (%u)", mgmt_event);
                break;
        case NET_EVENT_IPV4_DHCP_BOUND:
                LOG_DBG("NET_EVENT_IPV4_DHCP_BOUND (%u)", mgmt_event);
                led_set_mode(&leds.net, STEADY);
                net_time_sync();
                break;
        case NET_EVENT_IPV4_DHCP_STOP:
                LOG_DBG("NET_EVENT_IPV4_DHCP_STOP (%u)", mgmt_event);
                break;        
        default:
                LOG_WRN("unhandled event : (%u)", mgmt_event);
        }
}

void net_interface_init(void)
{
        struct net_if *iface = net_if_get_default();

        /* One callback per layer */
        net_mgmt_init_event_callback(&mgmt_cb[0], net_event_handler,
                                     NET_EVENT_ETHERNET_CARRIER_ON |
                                     NET_EVENT_ETHERNET_CARRIER_OFF);
        net_mgmt_add_event_callback(&mgmt_cb[0]);

        net_mgmt_init_event_callback(&mgmt_cb[1], net_event_handler,
                                     NET_EVENT_IF_UP |
                                     NET_EVENT_IF_DOWN);
        net_mgmt_add_event_callback(&mgmt_cb[1]);

        net_mgmt_init_event_callback(&mgmt_cb[2], net_event_handler,
                                     NET_EVENT_IPV4_ADDR_ADD |
                                     NET_EVENT_IPV4_ADDR_DEL | 
                                     NET_EVENT_IPV4_DHCP_START | 
                                     NET_EVENT_IPV4_DHCP_BOUND | 
                                     NET_EVENT_IPV4_DHCP_STOP);
        net_mgmt_add_event_callback(&mgmt_cb[2]);


        while (iface && !net_if_is_up(iface)) {
                LOG_DBG("iface %p down", iface);
                k_msleep(500);
        }

        LOG_INF("net interface %p UP ! ", iface);

        net_dhcpv4_start(iface);
}