/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CAN_H
#define _CAN_H

#include <stdint.h>
#include <stddef.h>

#include <drivers/can.h>
#include <device.h>

#include "dispatcher.h"

/**
 * @brief Queue a CAN message for transmission on the given CAN bus.
 * 
 * @param bus 
 * @param frame 
 * @param delay_ms delay before transmission in milliseconds
 * @return int 
 */
int can_queue(CAN_bus_t bus, struct zcan_frame *frame, uint32_t delay_ms);

#endif