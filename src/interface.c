#include "interface.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(net_if, LOG_LEVEL_INF);

static struct net_mgmt_event_callback mgmt_cb[2];

void net_event_handler(struct net_mgmt_event_callback *cb,
                       uint32_t mgmt_event,
                       struct net_if *iface)
{
        switch (mgmt_event) {
        case NET_EVENT_IF_UP:
        {
                LOG_INF("Eth if up (%u)", mgmt_event);
                break;
        }
        case NET_EVENT_IF_DOWN:
        {
                /* send k_work item to stop dhcp (cannot be called from isr)
                 * net_dhcpv4_stop(net_if_get_default()); */
                LOG_INF("Eth if down (%u)", mgmt_event);
                break;
        }
        case NET_EVENT_IPV4_ADDR_ADD:
                LOG_INF("IPV4 added (%u)", mgmt_event);
                break;
        case NET_EVENT_IPV4_ADDR_DEL:
                LOG_INF("IPV4 removed (%u)", mgmt_event);
                break;
        default:
                LOG_INF("undefined handler : (%u)", mgmt_event);
        }
}

void net_init(void)
{
        struct net_if *iface = net_if_get_default();

        /* TODO why do I need to seperate events IF and IPV4_ADDR events ? */
        net_mgmt_init_event_callback(&mgmt_cb[0], net_event_handler,
                                     NET_EVENT_IF_UP |
                                     NET_EVENT_IF_DOWN);
        net_mgmt_add_event_callback(&mgmt_cb[0]);

        net_mgmt_init_event_callback(&mgmt_cb[1], net_event_handler,
                                     NET_EVENT_IPV4_ADDR_ADD |
                                     NET_EVENT_IPV4_ADDR_DEL);
        net_mgmt_add_event_callback(&mgmt_cb[1]);

        net_dhcpv4_start(iface);
}