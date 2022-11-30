/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "userio.h"

#include "userio/leds.h"
#include "userio/button.h"

#include "system.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(userio, LOG_LEVEL_DBG);

typedef enum {
	USERIO_MODE_STATUS,
	USERIO_MODE_NET,
	USERIO_MODE_CAN,
	USERIO_MODE_BLE,

	_USERIO_MODE_COUNT,
} userio_mode_t;

static const led_t iface_led[_SYSEV_IF_COUNT] = {
	[SYSEV_IF_NET] = LED_NET,
	[SYSEV_IF_CAN] = LED_CAN,
	[SYSEV_IF_BLE] = LED_BLE,
};

static const userio_mode_t iface_get_assoc_mode[_SYSEV_IF_COUNT] = {
	[SYSEV_IF_NET] = USERIO_MODE_NET,
	[SYSEV_IF_CAN] = USERIO_MODE_CAN,
	[SYSEV_IF_BLE] = USERIO_MODE_BLE,
};

static const led_state_t mode_status_leds_states[_SYSEV_IF_EV_COUNT] = {
	[SYSEV_IF_DOWN] = LED_STATE_OFF,
	[SYSEV_IF_UP] = LED_STATE_ON,
	[SYSEV_IF_FATAL_ERROR] = LED_STATE_BLINK_FAST,
	[SYSEV_IF_PENDING] = LED_STATE_BLINK_SLOW,
	[SYSEV_IF_RX_TX] = LED_STATE_FLASH,
};

static const led_state_t mode_if_leds_states[_LED_COUNT][_SYSEV_IF_EV_COUNT] = {
	[LED_GREEN] = {
		[SYSEV_IF_DOWN] = LED_STATE_OFF,
		[SYSEV_IF_PENDING] = LED_STATE_BLINK_SLOW,
		[SYSEV_IF_UP] = LED_STATE_ON,
	},
	[LED_RED] = {
		[SYSEV_IF_DOWN] = LED_STATE_OFF,
		[SYSEV_IF_FATAL_ERROR] = LED_STATE_ON,
	},
	[LED_BLUE] = {
		[SYSEV_IF_DOWN] = LED_STATE_OFF,
		[SYSEV_IF_RX_TX] = LED_STATE_FLASH,
	},
};

static userio_mode_t userio_mode = USERIO_MODE_STATUS;

static void userio_apply_new_mode(userio_mode_t new_mode)
{
	if (new_mode == userio_mode) {
		return;
	}

	userio_mode = new_mode;

	for (uint8_t iface = 0; iface < _SYSEV_IF_COUNT; iface++) {
		userio_iface_show_event(iface, sysev_iface_get_state(iface));
	}
}

static void userio_mode_rotate(void)
{
	userio_apply_new_mode((userio_mode + 1u) % _USERIO_MODE_COUNT);
}

void userio_init(void)
{
	leds_init();

	button_init(userio_mode_rotate);

	userio_mode = USERIO_MODE_STATUS;	
}

int userio_iface_show_event(sysev_if_t iface, sysev_if_ev_t ev)
{
	if ((iface >= _SYSEV_IF_COUNT) || (ev >= _SYSEV_IF_EV_COUNT))
		return -EINVAL;

	int ret = -EINVAL;

	if (userio_mode == USERIO_MODE_STATUS) {
		ret = leds_set(iface_led[iface], mode_status_leds_states[ev]);
	} else if (iface_get_assoc_mode[iface] == userio_mode) {
		for (uint8_t led = 0; led < _LED_COUNT; led++) {
			ret = leds_set(led, mode_if_leds_states[led][ev]);
			if (ret) {
				goto exit;
			}
		}
	}
exit:
	return ret;
}