#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

void main(void)
{
        static uint32_t i = 0;
        for (;;) {
                LOG_INF("Main i = %d", i++);
                k_sleep(K_MSEC(1000));
        }
}
