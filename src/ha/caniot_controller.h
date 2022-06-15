#ifndef _CAN_CANIOT_CONTROLLER_H
#define _CAN_CANIOT_CONTROLLER_H	

#include <kernel.h>

#include <drivers/can.h>

#include <caniot/caniot.h>
#include <caniot/device.h>


/**
 * @brief Send a CANIOT query, non-blocking
 * 
 * @param req 
 * @param did 
 * @return int 
 */
int ha_ciot_ctrl_send(struct caniot_frame *__RESTRICT req,
		      caniot_did_t did);
/**
 * @brief Do a CANIOT query, blocking (if timeout != 0)
 * 
 * @param frame 
 * @return int 
 */
int ha_ciot_ctrl_query(struct caniot_frame *__RESTRICT req,
		       struct caniot_frame *__RESTRICT resp,
		       caniot_did_t did,
		       uint32_t timeout);

typedef void (*ha_ciot_ctrl_did_cb_t)(caniot_did_t did,
				      const struct caniot_frame *frame,
				      void *user_data);
				      
/* 
IDEAS

int ha_ciot_ctrl_register_did_cb(caniot_did_t did,
				 ha_ciot_ctrl_did_cb_t cb,
				 void *user_data);
*/

#endif /* _CAN_CANIOT_CONTROLLER_H */