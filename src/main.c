#include <zephyr.h>

#include "net_interface.h"
#include "net_time.h"
#include "crypto.h"

#include <mbedtls/memory_buffer_alloc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <devicetree.h>
#include <drivers/pwm.h>

#define PWM_GREEN_LED_NODE	DT_NODELABEL(green_pwm_led)

#if DT_NODE_HAS_STATUS(PWM_GREEN_LED_NODE, okay)
#define PWM_GREEN_CTLR		DT_PWMS_CTLR(PWM_GREEN_LED_NODE)
#define PWM_GREEN_CHANNEL	DT_PWMS_CHANNEL(PWM_GREEN_LED_NODE)
#define PWM_GREEN_PERIOD	DT_PWMS_PERIOD(PWM_GREEN_LED_NODE)
#define PWM_GREEN_FLAGS		DT_PWMS_FLAGS(PWM_GREEN_LED_NODE)
#else
#	error "green led PWM device not found"
#endif

#define PWM_BLUE_LED_NODE	DT_NODELABEL(blue_pwm_led)

#if DT_NODE_HAS_STATUS(PWM_BLUE_LED_NODE, okay)
#define PWM_BLUE_CTLR		DT_PWMS_CTLR(PWM_BLUE_LED_NODE)
#define PWM_BLUE_CHANNEL	DT_PWMS_CHANNEL(PWM_BLUE_LED_NODE)
#define PWM_BLUE_PERIOD		DT_PWMS_PERIOD(PWM_BLUE_LED_NODE)
#define PWM_BLUE_FLAGS		DT_PWMS_FLAGS(PWM_BLUE_LED_NODE)
#else
#	error "blue led PWM device not found"
#endif


static void init_pwm(void)
{
	const struct device *green_pwm_dev = DEVICE_DT_GET(PWM_GREEN_CTLR);

	int ret = pwm_pin_set_usec(green_pwm_dev, PWM_GREEN_CHANNEL, PWM_GREEN_PERIOD, 
				   PWM_GREEN_PERIOD / 2U, PWM_GREEN_FLAGS);
	LOG_INF("pwm_pin_set_usec returned %d", ret);

	const struct device *blue_pwm_dev = DEVICE_DT_GET(PWM_BLUE_CTLR);

	ret = pwm_pin_set_usec(blue_pwm_dev, PWM_BLUE_CHANNEL, PWM_BLUE_PERIOD, 
				   PWM_BLUE_PERIOD / 2U, PWM_BLUE_FLAGS);
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
