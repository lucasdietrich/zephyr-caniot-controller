#ifndef _CAN_CANIOT_CONTROLLER_H
#define _CAN_CANIOT_CONTROLLER_H	

#include <kernel.h>

#include <drivers/can.h>

/**
 * @brief Process the caniot frame
 * 
 * @param frame 
 * @return int 
 */
int caniot_process_can_frame(struct zcan_frame *frame);


#endif /* _CAN_CANIOT_CONTROLLER_H */