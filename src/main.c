#include <zephyr.h>

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define LEDGREEN_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LEDGREEN_NODE, okay)
#define LED0	DT_GPIO_LABEL(LEDGREEN_NODE, gpios)
#define PIN	DT_GPIO_PIN(LEDGREEN_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LEDGREEN_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN	0
#define FLAGS	0
#endif

void main(void)
{
	const struct device *dev;
	bool led_is_on = true;
	int ret;

	dev = device_get_binding(LED0);
	if (dev == NULL) {
		return;
	}

	ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret < 0) {
		return;
	}

	while (1) {
		gpio_pin_set(dev, PIN, (int)led_is_on);
		led_is_on = !led_is_on;
		k_msleep(1000);
	}
}
