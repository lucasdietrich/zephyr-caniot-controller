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

typedef enum
{
	CAN_BUS_1,
	CAN_BUS_2,
} can_bus_id_t;

#define CAN_BUS_CANIOT CAN_BUS_1

int if_can_init(void);

int if_can_attach_rx_msgq(can_bus_id_t canbus, 
			  struct k_msgq *rx_msgq,
			  struct zcan_filter *filter);

int if_can_send(can_bus_id_t canbus, 
		struct zcan_frame *frame);

#endif /* _CAN_INTERFACE_H */