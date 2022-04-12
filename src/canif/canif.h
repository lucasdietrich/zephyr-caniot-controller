#ifndef _CAN_H
#define _CAN_H

#include <stdint.h>
#include <stddef.h>

#include <drivers/can.h>
#include <device.h>

/**
 * @brief Can bus index
 */
typedef enum {
	CAN_BUS_1 = 0,
	CAN_BUS_2,
} CAN_bus_t;

/**
 * @brief Queue a CAN message for transmission on the given CAN bus.
 * 
 * @param frame 
 * @return int 
 */
int can_queue(CAN_bus_t bus, struct zcan_frame *frame);

#endif