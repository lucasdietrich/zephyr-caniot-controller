/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "leds.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(leds, LOG_LEVEL_DBG);

#define LED_FLASH_DURATION_MS 	50u

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

#define PWM_RED_LED_NODE	DT_NODELABEL(red_led_1)

#if DT_NODE_HAS_STATUS(PWM_BLUE_LED_NODE, okay) == 0
#	error "red led PWM device not found"
#endif

#define PWM_GREEN_DEVICE	DEVICE_DT_GET(PWM_GREEN_CTLR)
// static const struct device *const green_pwm_dev = DEVICE_DT_GET(PWM_GREEN_CTLR);

#define PWM_BLUE_DEVICE		DEVICE_DT_GET(PWM_BLUE_CTLR)
// static const struct device *const blue_pwm_dev = DEVICE_DT_GET(PWM_BLUE_CTLR);

/* DT_NODELABEL(red_led_1) is eq to DT_ALIAS(led_thread) */
static const struct gpio_dt_spec red_spec = GPIO_DT_SPEC_GET(PWM_RED_LED_NODE, gpios);

struct led_flash
{
	uint32_t duration_ms;
	struct k_work_delayable _dwork;
};

static void led_flash_handler(struct k_work *work);

static struct led_flash leds_flash_context[_LED_COUNT] = {
	{
		.duration_ms = LED_FLASH_DURATION_MS,
		._dwork = Z_WORK_DELAYABLE_INITIALIZER(led_flash_handler)
	},
	{
		.duration_ms = LED_FLASH_DURATION_MS,
		._dwork = Z_WORK_DELAYABLE_INITIALIZER(led_flash_handler)
	},
	{
		.duration_ms = LED_FLASH_DURATION_MS,
		._dwork = Z_WORK_DELAYABLE_INITIALIZER(led_flash_handler)
	}
};

int leds_init(void)
{
	int ret;

	ret = pwm_set(PWM_GREEN_DEVICE, PWM_GREEN_CHANNEL,
		      PWM_USEC(PWM_GREEN_PERIOD), PWM_USEC(0U), PWM_GREEN_FLAGS);
	if (ret != 0) {
		LOG_ERR("Failed to set green PWM, ret: %d", ret);
		goto exit;
	}

	ret = pwm_set(PWM_BLUE_DEVICE, PWM_BLUE_CHANNEL,
		      PWM_USEC(PWM_BLUE_PERIOD), PWM_USEC(0U), PWM_BLUE_FLAGS);
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
		LOG_ERR("Error %d: failed to configure LED device %s pin %d",
			ret, red_spec.port->name, red_spec.pin);
		goto exit;
	}

	gpio_pin_set_dt(&red_spec, LED_STATE_OFF);
exit:
	return ret;
}

int leds_set_blinking_phase(led_t led, uint32_t period_us, uint32_t phase_us)
{
	switch (led) {
	case LED_GREEN:
		return pwm_set(PWM_GREEN_DEVICE, PWM_GREEN_CHANNEL,
			       PWM_USEC(period_us), PWM_USEC(phase_us), PWM_GREEN_FLAGS);
	case LED_BLUE:
		return pwm_set(PWM_BLUE_DEVICE, PWM_BLUE_CHANNEL,
			       PWM_USEC(period_us), PWM_USEC(phase_us), PWM_BLUE_FLAGS);
	case LED_RED:
		if (phase_us == period_us) {
			return gpio_pin_set_dt(&red_spec, 1u);
		} else if (phase_us == 0) {
			return gpio_pin_set_dt(&red_spec, 0u);
		} else {
			return -ENOTSUP;
		}
	default:
		return -EINVAL;
	}
}

int leds_set_blinking(led_t led, uint32_t period_us)
{
	return leds_set_blinking_phase(led, period_us, period_us / 2U);
}

#define PWM_FAST_BLINK_PERIOD_US	200000U
#define PWM_FAST_BLINK_PHASE_US		(PWM_FAST_BLINK_PERIOD_US / 2U)
#define PWM_SLOW_BLINK_PERIOD_US	1000000U
#define PWM_SLOW_BLINK_PHASE_US		(PWM_SLOW_BLINK_PERIOD_US / 2U)

static const char *led_state_to_str(led_state_t state)
{
	switch (state) {
	case LED_STATE_NONE:
		return "none";
	case LED_STATE_OFF:
		return "off";
	case LED_STATE_ON:
		return "on";
	case LED_STATE_BLINK_SLOW:
		return "blink slow";
	case LED_STATE_BLINK_FAST:
		return "blink fast";
	case LED_STATE_FLASH:
		return "flash";
	default:
		return "<unknown>";
	}
}

static const char *led_to_str(led_t led)
{
	switch (led) {
	case LED_GREEN:
		return "green";
	case LED_BLUE:
		return "blue";
	case LED_RED:
		return "red";
	default:
		return "<unknown>";
	}
}

int leds_set(led_t led, led_state_t state)
{
	if (led >= _LED_COUNT) {
		return -EINVAL;
	}

	LOG_DBG("# %s -> %s", led_state_to_str(state), led_to_str(led));

	switch (state) {
	case LED_STATE_NONE:
		/* Do nothing */
		break;

	/* TODO, improve blinking cases, make sure on and off states are performed 
	 * immediately */
	case LED_STATE_OFF:
		return leds_set_blinking_phase(led, 1000u,
					       0u);
	case LED_STATE_ON:
		return leds_set_blinking_phase(led, 1000u,
					       1000u);
	case LED_STATE_BLINK_FAST:
		return leds_set_blinking_phase(led, PWM_FAST_BLINK_PERIOD_US,
					       PWM_FAST_BLINK_PHASE_US);
	case LED_STATE_BLINK_SLOW:
		return leds_set_blinking_phase(led, PWM_SLOW_BLINK_PERIOD_US,
					       PWM_SLOW_BLINK_PHASE_US);
	case LED_STATE_FLASH:
	{
		/* Turn on and schedule off */

		/* TODO find a way to delay end of flash without too heavy processing */
		int ret = leds_set(led, LED_STATE_ON);
		if (ret == 0u) {
			ret = k_work_schedule(&leds_flash_context[led]._dwork,
					      K_MSEC(leds_flash_context->duration_ms));
		}

		// k_sleep(K_MSEC(LED_FLASH_DURATION_MS));
		// ret = leds_set(led, LED_STATE_OFF);
		break;
	}
	default:
		return -EINVAL;
	}

	return 0u;
}

static void led_flash_handler(struct k_work *work)
{
	struct k_work_delayable *const dwork = k_work_delayable_from_work(work);
	struct led_flash *const led_ctx = CONTAINER_OF(dwork, struct led_flash, _dwork);
	const led_t led = led_ctx - leds_flash_context;
	leds_set(led, LED_STATE_OFF);
}

void leds_demo(void)
{
	leds_set_blinking_phase(LED_GREEN, 1000U, 250U);
	leds_set_blinking(LED_BLUE, 250U);
	leds_set(LED_RED, LED_STATE_ON);
}