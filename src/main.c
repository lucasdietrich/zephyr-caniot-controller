#include <zephyr.h>

#include "net_interface.h"
#include "net_time.h"
#include "crypto.h"

#include <mbedtls/memory_buffer_alloc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <devicetree.h>
#include <drivers/pwm.h>

#if DT_NODE_HAS_STATUS(DT_NODELABEL(tim3pwm), okay)
#	define PWM_DEV_NODE DT_NODELABEL(tim3pwm)
#else
#	error "PWM device not found"
#endif

static void init_pwm(void)
{
	const struct device *pwm_dev = DEVICE_DT_GET(PWM_DEV_NODE);

	int ret = pwm_pin_set_usec(pwm_dev, 3U, 1000000U, 500000U, 0);
	LOG_INF("pwm_pin_set_usec returned %d", ret);
}

static void debug_mbedtls_memory(void)
{
        size_t cur_used, cur_blocks, max_used, max_blocks;
        mbedtls_memory_buffer_alloc_cur_get(&cur_used, &cur_blocks);
        mbedtls_memory_buffer_alloc_max_get(&max_used, &max_blocks);

        LOG_DBG("MAX %u (%u) CUR %u (%u)", max_used,
                max_blocks, cur_used, cur_blocks);
}

void main(void)
{
        crypto_mbedtls_heap_init();
        net_interface_init();
        // user_io_init();

	init_pwm();

        static int counter = 0;
        
        for (;;) {
                if (counter++ % 60 == 0) {
                        net_time_show();

                        debug_mbedtls_memory();
                }

                k_msleep(1000);
        }
}
