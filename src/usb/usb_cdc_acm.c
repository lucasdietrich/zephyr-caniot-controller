#include "usb.h"

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
LOG_MODULE_DECLARE(usb, LOG_LEVEL_DBG);

static const struct device *dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

void usb_cdc_acm_thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(usb_tid,
				1024u,
				usb_cdc_acm_thread,
				1,
				NULL,
				NULL,
				K_PRIO_COOP(4u),
				0u,
				SYS_FOREVER_MS);

void usb_cdc_acm_thread_start(void)
{
	k_thread_start(usb_tid);
}

void usb_cdc_acm_thread(void *_a, void *_b, void *_c)
{
	ARG_UNUSED(_a);
	ARG_UNUSED(_b);
	ARG_UNUSED(_c);

	while (1) {
		unsigned char c;
		int ret = uart_poll_in(dev, &c);
		if (ret == 0) {
			LOG_INF("Received: %c", c);
		} else if (ret == -1) {
			/* No data */
		} else {
			LOG_ERR("Failed to receive data ret=%d", ret);
		}
		k_sleep(K_MSEC(1000));
	}
}