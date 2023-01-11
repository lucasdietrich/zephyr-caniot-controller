/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CAN_INTERFACE_H
#define _CAN_INTERFACE_H

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

typedef enum {
	CAN_BUS_1,
	CAN_BUS_2,
} can_bus_id_t;

#define CAN_BUS_CANIOT CAN_BUS_1

int if_can_init(void);

/**
 * @brief Attach a message queue to a CAN bus for receiving messages
 *
 * @param canbus
 * @param rx_msgq
 * @param filter
 * @return int
 */
int if_can_attach_rx_msgq(can_bus_id_t canbus,
			  struct k_msgq *rx_msgq,
			  struct can_filter *filter);

/**
 * @brief Send a CAN frame on a CAN bus
 *
 * @param canbus
 * @param frame
 * @return int
 */
int if_can_send(can_bus_id_t canbus, struct can_frame *frame);

#endif /* _CAN_INTERFACE_H */