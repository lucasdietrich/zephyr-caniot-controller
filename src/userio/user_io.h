/**
 * @file net_time.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Handle leds and user buttons
 * @version 0.1
 * @date 2021-11-07
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef _USER_IO_H_
#define _USER_IO_H_

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

int user_io_init(void);

typedef enum { OFF, STEADY, BLINKING_5Hz, BLINKING_1Hz, FLASHING} led_mode_t;

struct led
{
        struct gpio_dt_spec spec;
        led_mode_t mode;
        struct {
                uint8_t state: 1;
                uint8_t pending: 1;
                int8_t remaining: 6;
        }; /* remaining before switching state in 100ms */
};

struct leds {
        struct led net;
        struct led can;
        union {
                struct led ble;
                struct led thread;
        };
};

extern struct leds leds;

/* TODO make this function interrupt/thread safe */
void led_set_mode(struct led *led, led_mode_t mode);

#endif