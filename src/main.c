#include <zephyr.h>

#include "net_interface.h"
#include "net_time.h"
#include "user_io.h"

#include <net/net_if.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

void main(void)
{
        net_interface_init();
        user_io_init();

        static uint32_t i = 0;
        for (;;) {
                LOG_INF("IF up ? %d i = %d", net_if_is_up(net_if_get_default()) ? 1 : 0, i++);
                k_msleep(10000);
        }
}
