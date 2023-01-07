#include "usb.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
LOG_MODULE_REGISTER(usb, LOG_LEVEL_DBG);

int usb_init(void)
{
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0) {
		printk("Failed to enable USB");
		goto exit;
	}

#if defined(CONFIG_USB_CDC_ACM)
	usb_cdc_acm_thread_start();
#endif

exit:
	return ret;
}