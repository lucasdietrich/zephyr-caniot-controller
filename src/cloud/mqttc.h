/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CLOUD_MQTTC_H_
#define _CLOUD_MQTTC_H_

#include <zephyr/kernel.h>

#include "utils/buffers.h"
#include "poll.h"

#define MQTTC_TRY_CONNECT_FOREVER (-1)

typedef void (*mqttc_on_publish_cb_t)(const char *topic,
				      const char *payload,
				      size_t payload_len,
				      void *user_data);

int mqttc_init(void);

int mqttc_resolve_broker(void);

int mqttc_cleanup(void);

int mqttc_try_connect(uint32_t attempts);

int mqttc_disconnect(void);

int mqttc_set_pollfd(struct pollfd *fds);

int mqttc_keepalive_time_left(void);

int mqttc_process(struct pollfd *fds);

int mqttc_set_publish_cb(mqttc_on_publish_cb_t cb,
			    void *user_data);

int mqttc_subscribe(const char *topic, uint8_t qos);

int mqttc_publish(const char *topic, 
		  const char *payload, 
		  size_t len,
		  int qos);

/**
 * @brief Return whether the MQTT client is connected to the broker 
 * and ready to publish or subscribe.
 * 
 * @return true 
 * @return false 
 */
bool mqttc_ready(void);

cursor_buffer_t *mqttc_get_payload_buffer(void);

#endif /* _CLOUD_MQTT_H_ */