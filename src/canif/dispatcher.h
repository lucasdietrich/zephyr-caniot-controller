/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CAN_DISPATCHER_H_
#define _CAN_DISPATCHER_H_

#include <zephyr.h>

#include <drivers/can.h>

/**
 * @brief Can bus index
 */
typedef enum {
	CAN_BUS_1 = 0,
	CAN_BUS_2,
} CAN_bus_t;

int can_dispatch(CAN_bus_t bus, struct zcan_frame *frame);

#endif /* _CAN_DISPATCHER_H_ */