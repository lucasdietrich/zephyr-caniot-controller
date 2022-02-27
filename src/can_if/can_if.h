#ifndef _CAN_H
#define _CAN_H

#include <stdint.h>
#include <stddef.h>

#include <drivers/can.h>
#include <device.h>

int can_queue(struct zcan_frame *frame);

#endif