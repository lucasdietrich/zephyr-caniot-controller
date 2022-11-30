/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LEDS_H_
#define _LEDS_H_

#include <stdint.h>

/**
 * @brief According to UM1974 (page 24) and "STM32F427xx STM32F429xx product datasheet" page 76
 * 
 * - LD1 (green user led) is connected to PB0
 * 	- AF1 (alternate function 1) : TIM1_CH2N
 * 	- AF2 : TIM3_CH3 (choosen)
 * 	- AF3 : TIM8_CH2N
 * - LD2 (blue user led) is connected to PB7
 * 	- AF2 : TIM4_CH2 (choosen)
 * - LD3 (red user led) is connected to PB14
 * 	- AF1: TIM1_CH2N
 * 	- AF3 : TIM8_CH2N (choosen) - postponed because of lack of support
 * 	  for complementary PWM channels
 * 
 * - "These user LEDs are on when the I/O is HIGH value, and are off when the I/O is LOW."
 * 
 * - https://docs.zephyrproject.org/2.7.0/reference/devicetree/bindings/pwm/st,stm32-pwm.html
 */

#define LEDS_FREQ_0_5Hz_PERIOD		500U
#define LEDS_FREQ_1Hz_PERIOD		1000U
#define LEDS_FREQ_2Hz_PERIOD		2000U
#define LEDS_FREQ_4Hz_PERIOD		4000U
#define LEDS_FREQ_5Hz_PERIOD		5000U
#define LEDS_FREQ_10Hz_PERIOD		10000U

typedef enum {
	LED_GREEN = 0,
	LED_BLUE,
	LED_RED,

	_LED_COUNT,
} led_t;

#define LED_NET LED_GREEN
#define LED_THREAD LED_BLUE
#define LED_BLE LED_BLUE
#define LED_CAN LED_RED

typedef enum {
	LED_STATE_NONE = 0, // no change
	LED_STATE_OFF,
	LED_STATE_ON,
	LED_STATE_BLINK_SLOW,
	LED_STATE_BLINK_FAST,
	LED_STATE_FLASH,
	
	_LED_STATE_COUNT,
} led_state_t;

int leds_init(void);

int leds_set_blinking_phase(led_t led, uint32_t period_us, uint32_t phase_us);

int leds_set_blinking(led_t led, uint32_t period_us);

int leds_set(led_t led, led_state_t state);

void leds_demo(void);

#endif 