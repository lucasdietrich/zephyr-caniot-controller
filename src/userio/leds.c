/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "leds.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(leds, LOG_LEVEL_INF);

#define PWM_GREEN_LED_NODE DT_NODELABEL(green_pwm_led)

#if DT_NODE_HAS_STATUS(PWM_GREEN_LED_NODE, okay)
#define PWM_GREEN_CTLR	  DT_PWMS_CTLR(PWM_GREEN_LED_NODE)
#define PWM_GREEN_CHANNEL DT_PWMS_CHANNEL(PWM_GREEN_LED_NODE)
#define PWM_GREEN_PERIOD  DT_PWMS_PERIOD(PWM_GREEN_LED_NODE)
#define PWM_GREEN_FLAGS	  DT_PWMS_FLAGS(PWM_GREEN_LED_NODE)
#else
#error "green led PWM device not found"
#endif

#define PWM_BLUE_LED_NODE DT_NODELABEL(blue_pwm_led)

#if DT_NODE_HAS_STATUS(PWM_BLUE_LED_NODE, okay)
#define PWM_BLUE_CTLR	 DT_PWMS_CTLR(PWM_BLUE_LED_NODE)
#define PWM_BLUE_CHANNEL DT_PWMS_CHANNEL(PWM_BLUE_LED_NODE)
#define PWM_BLUE_PERIOD	 DT_PWMS_PERIOD(PWM_BLUE_LED_NODE)
#define PWM_BLUE_FLAGS	 DT_PWMS_FLAGS(PWM_BLUE_LED_NODE)
#else
#error "blue led PWM device not found"
#endif

#define PWM_RED_LED_NODE DT_NODELABEL(red_led_1)

#if DT_NODE_HAS_STATUS(PWM_BLUE_LED_NODE, okay) == 0
#error "red led PWM device not found"
#endif

#define PWM_GREEN_DEVICE DEVICE_DT_GET(PWM_GREEN_CTLR)
// static const struct device *const green_pwm_dev =
// DEVICE_DT_GET(PWM_GREEN_CTLR);

#define PWM_BLUE_DEVICE DEVICE_DT_GET(PWM_BLUE_CTLR)
// static const struct device *const blue_pwm_dev =
// DEVICE_DT_GET(PWM_BLUE_CTLR);

/* DT_NODELABEL(red_led_1) is eq to DT_ALIAS(led_thread) */
static const struct gpio_dt_spec red_spec = GPIO_DT_SPEC_GET(PWM_RED_LED_NODE, gpios);

int leds_init(void)
{
	int ret;

	ret = pwm_set(PWM_GREEN_DEVICE, PWM_GREEN_CHANNEL, PWM_USEC(PWM_GREEN_PERIOD),
				  PWM_USEC(0U), PWM_GREEN_FLAGS);
	if (ret != 0) {
		LOG_ERR("Failed to set green PWM, ret: %d", ret);
		goto exit;
	}

	ret = pwm_set(PWM_BLUE_DEVICE, PWM_BLUE_CHANNEL, PWM_USEC(PWM_BLUE_PERIOD),
				  PWM_USEC(0U), PWM_BLUE_FLAGS);
	if (ret != 0) {
		LOG_ERR("Failed to set blue PWM, ret: %d", ret);
		goto exit;
	}

	if (red_spec.port && !device_is_ready(red_spec.port)) {
		LOG_ERR("led %p not ready = ", red_spec.port);
		goto exit;
	}

	ret = gpio_pin_configure_dt(&red_spec, GPIO_OUTPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure LED device %s pin %d", ret,
				red_spec.port->name, red_spec.pin);
		goto exit;
	}

	gpio_pin_set_dt(&red_spec, LED_OFF);
exit:
	return ret;
}

int leds_set_blinking_phase(led_t led, uint32_t period_us, uint32_t phase_us)
{
	if (led == LED_GREEN) {
		return pwm_set(PWM_GREEN_DEVICE, PWM_GREEN_CHANNEL, PWM_USEC(period_us),
					   PWM_USEC(phase_us), PWM_GREEN_FLAGS);

	} else if (led == LED_BLUE) {
		return pwm_set(PWM_BLUE_DEVICE, PWM_BLUE_CHANNEL, PWM_USEC(period_us),
					   PWM_USEC(phase_us), PWM_BLUE_FLAGS);
	}

	return -EINVAL;
}

int leds_set_blinking(led_t led, uint32_t period_us)
{
	return leds_set_blinking_phase(led, period_us, period_us / 2U);
}

int leds_set(led_t led, led_state_t state)
{
	switch (led) {
	case LED_GREEN:
		return leds_set_blinking_phase(led, PWM_GREEN_PERIOD,
									   state == LED_OFF ? 0U : PWM_GREEN_PERIOD);
	case LED_BLUE:
		return leds_set_blinking_phase(led, PWM_BLUE_PERIOD,
									   state == LED_OFF ? 0U : PWM_BLUE_PERIOD);
	case LED_RED:
		return gpio_pin_set_dt(&red_spec, state);
	}

	return -EINVAL;
}

void leds_demo(void)
{
	leds_set_blinking_phase(LED_GREEN, 1000U, 250U);
	leds_set_blinking(LED_BLUE, 250U);
	leds_set(LED_RED, LED_ON);
}