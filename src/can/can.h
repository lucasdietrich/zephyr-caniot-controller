#ifndef _CAN_H
#define _CAN_H

#include <stdint.h>
#include <stddef.h>

#include <drivers/can.h>
#include <device.h>

void can_init(void);

void can_rx_thread(const struct device *dev, struct k_msgq *msgq, struct zcan_filter *filter);
void can_tx_thread(const struct device *can_dev, void *_b, void *_c);

#endif