#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/net_config.h>

void net_init(void);

static inline bool net_is_if_up(void)
{
        return net_if_up(net_if_get_default()) == 0;
}

#endif