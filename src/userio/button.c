/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "button.h"

#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(button, LOG_LEVEL_INF);

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_NODELABEL(user_button), gpios);
static struct gpio_callback button_cb_data;

void (*user_callback)(void) = NULL;

static void button_pressed(const struct device *dev,
			   struct gpio_callback *cb,
			   uint32_t pins)
{
	uint64_t ms = k_uptime_get();
	LOG_INF("Button pressed at %llu ms", ms);

	if (user_callback != NULL) {
		user_callback();
	}
}

int button_init(void (*callback)(void))
{
        int ret = -EIO;

        /* set up button */
        if (!device_is_ready(button.port)) {
                LOG_ERR("Error: button device %s is not ready",
                        button.port->name);
                goto exit;
        }

        ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
        if (ret != 0) {
                LOG_ERR("Error %d: failed to configure %s pin %d",
                        ret, button.port->name, button.pin);
                goto exit;
        }

        ret = gpio_pin_interrupt_configure_dt(&button,
                                              GPIO_INT_EDGE_TO_ACTIVE);
        if (ret != 0) {
                LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
                        ret, button.port->name, button.pin);
                goto exit;
        }

	user_callback = callback;

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	LOG_DBG("Set up button at %s pin %d", button.port->name, button.pin);

exit:
	return ret;
}

int button_register_callback(void (*callback)(void))
{
	user_callback = callback;
	return 0;
}