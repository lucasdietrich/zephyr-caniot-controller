#include "user_io.h"

#include <kernel.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(user_io, LOG_LEVEL_INF);

static void led_process(struct k_timer *timer_id);

static K_MUTEX_DEFINE(mutex);

K_TIMER_DEFINE(timer, led_process, NULL);

struct leds leds = {
        .net = {GPIO_DT_SPEC_GET(DT_ALIAS(led_net), gpios), OFF, {0, 0}},
        .can = {GPIO_DT_SPEC_GET(DT_ALIAS(led_can), gpios), OFF, {0, 0}},
        .ble = {GPIO_DT_SPEC_GET(DT_ALIAS(led_thread), gpios), OFF, {0, 0}}
};

static k_timeout_t led_prepare_next(struct led *led)
{
        switch (led->mode) {
        case OFF:
        case STEADY:
                led->pending = 0;
                return K_FOREVER;
        case FLASHING:
                if (led->state) {
                        led->remaining = 1;
                } else {
                        led->pending = 0;
                        return K_FOREVER;
                }
        case BLINKING_1Hz:
                led->remaining = 5;
                break;
        case BLINKING_5Hz:
                led->remaining = 1;
                break;
        }
        led->pending = 1;
        led->state = 1 - led->state;

        return K_MSEC(100 * led->remaining);
}

int user_io_init(void)
{
        int ret;

        k_mutex_lock(&mutex, K_FOREVER);

        for (struct led *led = &leds.net; led <= &leds.thread; led++) {
                if (led->spec.port && !device_is_ready(led->spec.port)) {
                        LOG_ERR("led %p not ready = ", led->spec.port);
                        goto exit;
                }

                ret = gpio_pin_configure_dt(&led->spec, GPIO_OUTPUT);
                if (ret != 0) {
                        LOG_ERR("Error %d: failed to configure LED device %s pin %d\n",
                                ret, led->spec.port->name, led->spec.pin);
                        goto exit;
                }
        }

        k_mutex_unlock(&mutex);

        return 0;

exit:
        return -EIO;
}

void led_set_mode(struct led *led, led_mode_t mode)
{
        k_mutex_lock(&mutex, K_FOREVER);

        if (mode == led->mode) {
                return;
        }

        led->mode = mode;
        led->state = mode != OFF;

        gpio_pin_set_dt(&led->spec, (int) led->state);
        k_timeout_t near_process = led_prepare_next(led);
        if (!K_TIMEOUT_EQ(near_process, K_FOREVER)) {
                if (k_timer_remaining_get(&timer) == 0) {
                        k_timer_start(&timer, near_process, near_process);
                }
        }

        k_mutex_unlock(&mutex);
}

static void led_process(struct k_timer *timer)
{
        uint64_t ms = k_ticks_to_ms_near64(timer->period.ticks);
        LOG_DBG("Process timer %p, period = %llu", timer, ms);

        k_timeout_t next_process = K_FOREVER;
        for (struct led *led = &leds.net; led <= &leds.thread; led++) {
                if (led->pending) {
                        if (K_TIMEOUT_EQ(next_process, K_FOREVER)) {
                                next_process = K_MSEC(500);
                        }
                        led->remaining -= ms / 100;
                        if (led->remaining <= 0) {
                                gpio_pin_set_dt(&led->spec, led->state);
                                k_timeout_t near_process = led_prepare_next(led);
                                
                                if (!K_TIMEOUT_EQ(near_process, K_FOREVER) &&
                                    (near_process.ticks < next_process.ticks)) {
                                        next_process = near_process;
                                }
                        }
                }
        }

        if (K_TIMEOUT_EQ(next_process, K_FOREVER)) {
                k_timer_stop(timer);
        } else if (!K_TIMEOUT_EQ(next_process, timer->period)) {
                k_timer_start(timer, next_process, next_process);
        }
}