/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CAN_INTERFACE_H
#define _CAN_INTERFACE_H

#include <zephyr.h>

#include <drivers/can.h>

#include <caniot/caniot.h>
#include <caniot/device.h>

int if_can_send(struct zcan_frame *frame);

int if_can_init(void);

int if_can_attach_rx_msgq(struct k_msgq *rx_msgq, 
			  struct zcan_filter *filter);

#endif /* _CAN_INTERFACE_H */