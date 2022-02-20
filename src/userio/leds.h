#ifndef _LEDS_H_
#define _LEDS_H_

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
 * 	- AF3 : TIM8_CH2N (choosen)
 * 
 * - "These user LEDs are on when the I/O is HIGH value, and are off when the I/O is LOW."
 * 
 * - https://docs.zephyrproject.org/2.7.0/reference/devicetree/bindings/pwm/st,stm32-pwm.html
 */

#endif 