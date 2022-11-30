/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "can/can_interface.h"

#include "system.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(if_can, LOG_LEVEL_WRN);

const struct device *can_dev = DEVICE_DT_GET(DT_NODELABEL(can1));

int if_can_init(void)
{
	sysev_notify(SYSEV_IF_CAN, SYSEV_IF_PENDING, NULL);

	/* wait for device ready */
	while (!device_is_ready(can_dev)) {
		LOG_WRN("CAN: Device %s not ready.\n", can_dev->name);
		k_sleep(K_SECONDS(1));
	}

	int ret = can_start(can_dev);
	if (ret) {
		sysev_notify(SYSEV_IF_CAN, SYSEV_IF_FATAL_ERROR, NULL);
		LOG_ERR("CAN: Failed to start ret=%d", ret);
	}

	sysev_notify(SYSEV_IF_CAN, SYSEV_IF_UP, NULL);

	return 0;
}

int if_can_attach_rx_msgq(can_bus_id_t canbus,
			  struct k_msgq *rx_msgq,
			  struct can_filter *filter)
{
	int ret = -EINVAL;

	if (canbus != CAN_BUS_1) {
		goto exit;
	}

	if ((rx_msgq != NULL) && (filter != NULL)) {
		/* attach message q */
		ret = can_add_rx_filter_msgq(can_dev, rx_msgq, filter);
		if (ret < 0) {
			sysev_notify(SYSEV_IF_CAN, SYSEV_IF_FATAL_ERROR, NULL);
			LOG_ERR("can_add_rx_filter_msgq failed: %d", ret);
		}
	}

exit:
	return ret;
}

int if_can_send(can_bus_id_t canbus,
		struct can_frame *frame)
{
	int ret;

	if (canbus != CAN_BUS_1) {
		return -EINVAL;
	}

	ret = can_send(can_dev, frame, K_FOREVER, NULL, NULL);
	if (ret == 0) {
		sysev_notify(SYSEV_IF_CAN, SYSEV_IF_RX_TX, NULL);
	}

	return ret;
}