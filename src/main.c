#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include <device.h>
#include <drivers/can.h>

void main(void)
{
	struct zcan_frame frame = {
		.id_type = CAN_STANDARD_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.id = 0x123,
		.dlc = 8,
		.data = {1,2,3,4,5,6,7,8}
	};
	const struct device *can_dev;
	int ret;

	can_dev = device_get_binding("CAN_1");

	while (!device_is_ready(can_dev)) {
		printk("CAN: Device %s not ready.\n", can_dev->name);
		k_sleep(K_SECONDS(1));
	}

	for (;;) {
		ret = can_send(can_dev, &frame, K_MSEC(100), NULL, NULL);
		if (ret != CAN_TX_OK) {
			LOG_ERR("Sending failed [%d]", ret);
		}
		
		k_sleep(K_SECONDS(5));
	}

	k_sleep(K_FOREVER);
}
