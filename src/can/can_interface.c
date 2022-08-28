/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "can/can_interface.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(if_can, LOG_LEVEL_WRN);

const struct device *can_dev = DEVICE_DT_GET(DT_NODELABEL(can1));

int if_can_send(struct zcan_frame *frame)
{
	return can_send(can_dev, frame, K_FOREVER, NULL, NULL);
}

int if_can_init(void)
{
	/* wait for device ready */
	while (!device_is_ready(can_dev)) {
		LOG_WRN("CAN: Device %s not ready.\n", can_dev->name);
		k_sleep(K_SECONDS(1));
	}

	return 0;
}

int if_can_attach_rx_msgq(struct k_msgq *rx_msgq, 
			  struct zcan_filter *filter)
{
	int ret = -EINVAL;

	if ((rx_msgq != NULL) && (filter != NULL)) {
		/* attach message q */
		ret = can_add_rx_filter_msgq(can_dev, rx_msgq, filter);
		if (ret) {
			LOG_ERR("can_add_rx_filter_msgq failed: %d", ret);
		}
	}

	return ret;
}