#ifndef _CAN_CANIOT_CONTROLLER_H
#define _CAN_CANIOT_CONTROLLER_H	

#include <kernel.h>

#include <drivers/can.h>

#include <caniot/caniot.h>
#include <caniot/device.h>


/**
 * @brief Process the caniot frame
 * 
 * @param frame 
 * @return int 
 */
int ha_ciot_process_frame(struct zcan_frame *frame);

int ha_ciot_ctrl_init(void);

int ha_ciot_ctrl_command(caniot_did_t did,
			 caniot_endpoint_t endpoint,
			 const caniot_frame_t *query,
			 caniot_frame_t *resp,
			 uint32_t timeout_ms);

int ha_ciot_ctrl_qtelemetry(caniot_did_t did,
			    caniot_endpoint_t endpoint,
			    caniot_frame_t *resp,
			    uint32_t timeout_ms);

#endif /* _CAN_CANIOT_CONTROLLER_H */