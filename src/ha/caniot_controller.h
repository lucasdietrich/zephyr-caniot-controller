/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CAN_CANIOT_CONTROLLER_H
#define _CAN_CANIOT_CONTROLLER_H	

#include <zephyr/kernel.h>

#include <caniot/caniot.h>
#include <caniot/device.h>

/**
 * @brief Send a CANIOT query, non-blocking
 * 
 * Note: Thread safe
 * 
 * @param req 
 * @param did 
 * @retval 0 on success
 * @retval CANIOT error (errors below -CANIOT_ERROR_BASE)
 */
int ha_ciot_ctrl_send(struct caniot_frame *__restrict req,
		      caniot_did_t did);
/**
 * @brief Do a CANIOT query, blocking (if timeout != 0)
 * 
 * Note: Thread safe
 * 
 * @param req
 * @param resp
 * @param did
 * @param timeout Timeout in milliseconds (on success, contain the actual time spent in the call)
 * @retval 0 Query sent but not answered because of null timeout
 * @retval 1 Query sent and answered (delta is written in timeout variable)
 * @retval 2 Query sent and answered by a device caniot error (delta is written in timeout variable)
 * @retval -EINVAL Invalid data supplied
 * @retval -ENOMEM No memory available for allocating context
 * @retval -EAGAIN Waiting period timed out.
 * @retval any other CANIOT error
 */
int ha_ciot_ctrl_query(struct caniot_frame *__restrict req,
		       struct caniot_frame *__restrict resp,
		       caniot_did_t did,
		       uint32_t *timeout);

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