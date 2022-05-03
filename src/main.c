#include <zephyr.h>

#include <device.h>
#include <drivers/sensor.h>

#include "net_interface.h"
#include "net_time.h"
#include "crypto.h"

#include "userio/leds.h"
#include "userio/button.h"

#include "ha/devices.h"

#include <mbedtls/memory_buffer_alloc.h>

#include <stdio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_NONE);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_temp)
#define TEMP_NODE DT_INST(0, st_stm32_temp)
#else
#error "Could not find a compatible temperature sensor"
#endif

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
        leds_init();
	button_init();

        crypto_mbedtls_heap_init();
        net_interface_init();
	ha_dev_init();

	int rc;
	struct sensor_value val;
	const struct device *dev = DEVICE_DT_GET(TEMP_NODE);

	if (!device_is_ready(dev)) {
		LOG_ERR("(%p) Temperature sensor is not ready", dev);
		return;
	}

        static int counter = 0;
        
	for (;;) {
		rc = sensor_sample_fetch(dev);
		if (rc) {
			LOG_ERR("Failed to fetch sample (%d)", rc);
			continue;
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &val);
		if (rc) {
			LOG_ERR("Failed to get data (%d)", rc);
			continue;
		}

		const float temperature = (float)sensor_value_to_double(&val);
		if (temperature > -276.0) {
			ha_dev_register_die_temperature(net_time_get(), temperature);

			LOG_DBG("Current DIE temperature: %.1f °C ", temperature);
		} else {
			LOG_WRN("Invalid DIE temperature: %.1f °C", temperature);
		}

                if (counter++ % 600 == 0) {
                        net_time_show();

                        debug_mbedtls_memory();
                }

                k_msleep(5000);
        }
}
