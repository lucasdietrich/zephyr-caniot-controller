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
int ha_caniot_controller_send(struct caniot_frame *__restrict req, caniot_did_t did);

/**
 * @brief Do a blocking (if timeout != 0) CANIOT query
 *
 * Note: Thread safe
 *
 * @param req
 * @param resp
 * @param did
 * @param timeout Timeout in milliseconds (on success, contain the actual time
 * spent in the call)
 * @retval 0 Query sent but not answered because of null timeout
 * @retval 1 Query sent and answered (delta is written in timeout variable)
 * @retval 2 Query sent and answered by a device caniot error (delta is written
 * in timeout variable)
 * @retval -EINVAL Invalid data supplied
 * @retval -ENOMEM No memory available for allocating context
 * @retval -EAGAIN Waiting period timed out.
 * @retval any other CANIOT error
 */
int ha_caniot_controller_query(struct caniot_frame *__restrict req,
							   struct caniot_frame *__restrict resp,
							   caniot_did_t did,
							   uint32_t *timeout);

/**
 * @brief CANIOT device discovery callback
 *
 * @param did Device ID
 * @param frame CANIOT frame
 * @param user_data User data
 */
typedef void (*ha_ciot_ctrl_did_cb_t)(caniot_did_t did,
									  const struct caniot_frame *frame,
									  void *user_data);

/**
 * @brief Discover all CANIOT devices, call cb for each one
 *
 * @param timeout
 * @param cb
 * @return int
 */
int ha_controller_caniot_discover(uint32_t timeout, ha_ciot_ctrl_did_cb_t cb);

/*
IDEAS

int ha_ciot_ctrl_register_did_cb(caniot_did_t did,
		 ha_ciot_ctrl_did_cb_t cb,
		 void *user_data);
*/

#endif /* _CAN_CANIOT_CONTROLLER_H */