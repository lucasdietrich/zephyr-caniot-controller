#include <zephyr.h>

#include "net_interface.h"
#include "user_io.h"
#include "net_time.h"
#include "udp_discovery.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

void main(void)
{
        net_interface_init();
        user_io_init();
        
        for (;;) {
                net_time_show();

                k_msleep(60000);
        }
}
