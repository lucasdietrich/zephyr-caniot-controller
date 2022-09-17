

/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include <sys/util.h>
#include <unistd.h>

#include <poll.h>

#include <net/mqtt.h>
#include <net/socket.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/dns_resolve.h>
#include <net/tls_credentials.h>

#include "utils/misc.h"
#include "utils/buffers.h"

#include "app_sections.h"

#include "creds/credentials.h"
#include "creds/manager.h"

#include "cloud/cloud.h"
#include "cloud/backoff.h"
#include "cloud/utils.h"
#include "cloud/mqttc.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(mqttc, LOG_LEVEL_INF);

#include "cloud_internal.h"

#define MQTT_PAYLOAD_BUFFER_SIZE 4096u
#define MQTT_RX_BUFFER_SIZE 256u
#define MQTT_TX_BUFFER_SIZE 256u

__buf_noinit_section char mqtt_rx_buf[MQTT_RX_BUFFER_SIZE];
__buf_noinit_section char mqtt_tx_buf[MQTT_TX_BUFFER_SIZE];
__buf_noinit_section char payload_buf[MQTT_PAYLOAD_BUFFER_SIZE];
static cursor_buffer_t buffer = CUR_BUFFER_STATIC_INIT(payload_buf, sizeof(payload_buf));

static mqttc_on_publish_cb_t on_publish_cb;
static void *on_publish_user_data;

#define MQTTC_BIT_CONNECTED 	(0u)
#define MQTTC_BIT_INPROGRESS 	(1u)
#define MQTTC_FLAG_CONNECTED 	(1u << MQTTC_BIT_CONNECTED)
#define MQTTC_FLAG_INPROGRESS 	(1u << MQTTC_BIT_INPROGRESS)
static atomic_t state = ATOMIC_INIT(0u);

static uint32_t message_id;

static struct mqtt_client mqtt;

// K_SEM_DEFINE(mqtt_lock, 0u, 1u);
// K_SEM_DEFINE(mqtt_onpub, 0u, 1u);

/* Forward declarations */
static void mqtt_event_cb(struct mqtt_client *client,
			  const struct mqtt_evt *evt);

static void handle_published_message(const struct mqtt_publish_param *msg)
{
	size_t received = 0U;
	char topic[256];
	const size_t topic_size = msg->message.topic.topic.size;
	const size_t message_size = msg->message.payload.len;

	/* Validate and copy the topic */
	if (topic_size >= sizeof(topic)) {
		LOG_ERR("Topic is too long %u > %u",
			topic_size,
			sizeof(topic));
		return;
	}

	strncpy((char *)topic, (char *)msg->message.topic.topic.utf8, topic_size);
	topic[topic_size] = '\0';

	LOG_INF("Received %u B message on topic %s", message_size, log_strdup(topic));

	/* Check if the message fit into the buffer */
	cursor_buffer_reset(&buffer);

	const bool discarded = message_size > buffer.size;
	if (discarded) {
		LOG_WRN("Published messaged too big %u > %u, discarding ...",
			message_size, buffer.size);
	}

	/* Receive the message */
	while (received < message_size) {
		ssize_t ret;

		ret = mqtt_read_publish_payload(&mqtt, buffer.cursor,
						cursor_buffer_remaining(&buffer));

		if (ret < 0) {
			LOG_ERR("Failed to read payload: %d", ret);
			break;
		} else if (discarded == false) {
			buffer.cursor += ret;
		}

		received += ret;
	}

	/* Answer to the broker if requested */
	if (msg->message.topic.qos >= MQTT_QOS_1_AT_LEAST_ONCE) {
		const struct mqtt_puback_param param = {
			.message_id = msg->message_id
		};

		int ret = mqtt_publish_qos1_ack(&mqtt, &param);
		if (ret < 0) {
			LOG_ERR("Failed to send PUBACK: %d", ret);
		}
	}

	/* Call application handler */
	if (discarded == false) {
		
		if (on_publish_cb != NULL) {
			on_publish_cb(topic,
				      payload_buf,
				      message_size,
				      on_publish_user_data);
		}

		LOG_HEXDUMP_INF(buffer.buffer, cursor_buffer_filling(&buffer),
				"Received message");
	}

}

static void mqtt_event_cb(struct mqtt_client *client,
			  const struct mqtt_evt *evt)
{
	LOG_INF("mqtt_evt=%hhx [ %s ]", (uint8_t)evt->type,
		mqtt_evt_get_str(evt->type));

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
	{
		atomic_set_bit(&state, MQTTC_BIT_CONNECTED);
		break;
	}

	case MQTT_EVT_DISCONNECT:
		atomic_clear_bit(&state, MQTTC_BIT_CONNECTED);
		break;

	case MQTT_EVT_PUBLISH:
	{
		atomic_set_bit(&state, MQTTC_BIT_INPROGRESS);

		const struct mqtt_publish_param *pub = &evt->param.publish;
		handle_published_message(pub);

		atomic_clear_bit(&state, MQTTC_BIT_INPROGRESS);
		break;
	}

	case MQTT_EVT_PUBACK:
	{
		atomic_clear_bit(&state, MQTTC_BIT_INPROGRESS);

		LOG_DBG("Publish acknowledged %d",
			evt->param.puback.message_id);
		break;
	}

	case MQTT_EVT_PUBREC:
		break;

	case MQTT_EVT_PUBREL:
		break;

	case MQTT_EVT_PUBCOMP:
		break;

	case MQTT_EVT_SUBACK:
	{
		atomic_clear_bit(&state, MQTTC_BIT_INPROGRESS);
		LOG_DBG("Subscription acknowledged %d",
			evt->param.suback.message_id);
		break;
	}

	case MQTT_EVT_UNSUBACK:
	{
		LOG_DBG("Unsubscription acknowledged %d",
			evt->param.unsuback.message_id);
		break;
	}

	case MQTT_EVT_PINGRESP:
		break;
	}
}

static int wait_for_input(int timeout)
{
	int res;
	struct zsock_pollfd fds = {
		.fd = mqtt.transport.tls.sock,
		.events = ZSOCK_POLLIN,
		.revents = 0
	};

	res = poll(&fds, 1, timeout);
	if (res < 0) {
		LOG_ERR("poll read event error, res=%d", res);
		return -errno;
	}

	return res;
}

int mqttc_try_connect(uint32_t max_attempts)
{
	int ret;
	struct backoff bo;

	backoff_init(&bo, BACKOFF_METHOD_DECORR_JITTER);

	do {
		LOG_DBG("MQTT %p try connect", &mqtt);

		ret = mqtt_connect(&mqtt);
		if (ret == 0) {
			/* Wait for CONNACK */
			ret = wait_for_input(5000);

			if (ret >= 0) {
				mqtt_input(&mqtt);
				if (atomic_test_bit(&state, MQTTC_BIT_CONNECTED)) {
					ret = 0;
				} else {
					ret = -ECONNREFUSED;
				}
			}

			if (ret != 0) {
				mqtt_abort(&mqtt);
			}
		}

		if (ret != 0) {
			const uint32_t delay_ms = backoff_next(&bo);

			LOG_ERR("MQTT %p connect failed ret=%d retry in %u ms",
				&mqtt, ret, delay_ms);

			k_sleep(K_MSEC(delay_ms));
		}
	} while ((ret != 0) && (bo.attempts < max_attempts));

	return ret;
}

// static void platform_config_clear(struct cloud_platform_config *c)
// {
// 	memset(c, 0, sizeof(struct cloud_platform_config));
// }

int mqttc_init(void)
{
	int ret;

	struct cloud_platform *const p = cloud_platform_get();
	if (p == NULL) {
		return -ENOTSUP;
	}

	/* Initialize the MQTT client structure */
	mqtt_client_init(&mqtt);

	/* Platform initialization */
	ret = p->init(&p->config);
	if (ret != 0) {
		return ret;
	}

	/* Set MQTT client for platform */
	p->mqtt = &mqtt;

	/* Alloc broker hostname */
	mqtt.broker = &p->broker;

	/* Generic MQTT client configuration */
	mqtt.evt_cb = mqtt_event_cb;

	mqtt.rx_buf = mqtt_rx_buf;
	mqtt.rx_buf_size = MQTT_RX_BUFFER_SIZE;
	mqtt.tx_buf = mqtt_tx_buf;
	mqtt.tx_buf_size = MQTT_TX_BUFFER_SIZE;

	if (p->config.clientid) {
		mqtt.client_id.utf8 = (uint8_t *)p->config.clientid;
		mqtt.client_id.size = strlen(p->config.clientid);
	}

	if (p->config.password) {
		mqtt.password->utf8 = p->config.password;
		mqtt.password->size = strlen(p->config.password);
	}

	if (p->config.user) {
		mqtt.user_name->utf8 = p->config.user;
		mqtt.user_name->size = strlen(p->config.user);
	}

	mqtt.keepalive = CONFIG_MQTT_KEEPALIVE;

	mqtt.protocol_version = MQTT_VERSION_3_1_1;

	// setup TLS
	mqtt.transport.type = MQTT_TRANSPORT_SECURE;

	struct mqtt_sec_config *const tls_config = &mqtt.transport.tls.config;

	tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
	tls_config->cipher_list = NULL;
	tls_config->sec_tag_list = p->config.sec_tag_list;
	tls_config->sec_tag_count = p->config.sec_tag_count;
	tls_config->hostname = p->config.endpoint;
	tls_config->cert_nocopy = TLS_CERT_NOCOPY_OPTIONAL;

	/* Message id 0 not permitted */
	message_id = 1u;

	atomic_clear(&state);

	return ret;
}

int mqttc_resolve_broker(void)
{
	struct cloud_platform *const p = cloud_platform_get();
	if (p == NULL) {
		return -ENOTSUP;
	}

	/* resolve broker hostname */
	return resolve_hostname(&p->broker, p->config.endpoint, p->config.port);
}

int mqttc_cleanup(void)
{
	struct cloud_platform *const p = cloud_platform_get();
	if (p == NULL) {
		return -ENOTSUP;
	}

	return p->deinit(&p->config);
}

int mqttc_disconnect(void)
{
	int ret = mqtt_disconnect(&mqtt);
	if (ret == 0) {
		ret = wait_for_input(5000);
		if (ret >= 0) {
			mqtt_input(&mqtt);
		} else {
			mqtt_abort(&mqtt);
			atomic_clear_bit(&state, MQTTC_BIT_CONNECTED);
		}
	} else {
		LOG_ERR("Failed to disconnect from broker: %d", ret);
	}
	return ret;
}

int mqttc_set_publish_cb(mqttc_on_publish_cb_t cb,
			 void *user_data)
{
	if (!cb) {
		return -EINVAL;
	}

	on_publish_cb = cb;
	on_publish_user_data = user_data;

	return 0;
}

int mqttc_subscribe(const char *topic, uint8_t qos)
{
	if (!topic) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&state, MQTTC_BIT_INPROGRESS) == true) {
		return -EBUSY;
	}

	struct mqtt_topic top;
	top.topic.utf8 = topic;
	top.topic.size = strlen(topic);
	top.qos = qos;

	struct mqtt_subscription_list sub_list = {
		.list = &top,
		.list_count = 1u,
		.message_id = message_id,
	};

	message_id++;

	int ret = mqtt_subscribe(&mqtt, &sub_list);
	if (ret != 0) {
		atomic_clear_bit(&state, MQTTC_BIT_INPROGRESS);

		LOG_ERR("Failed to subscribe to topic %s: %d", topic, ret);
	}

	return ret;
}

int mqttc_publish(const char *topic,
		  const char *payload,
		  size_t len,
		  int qos)
{
	if (!topic || !payload) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&state, MQTTC_BIT_INPROGRESS) == true) {
		return -EBUSY;
	}

	struct mqtt_publish_param msg;

	msg.retain_flag = 0u;
	msg.message.topic.qos = qos;
	msg.message.topic.topic.utf8 = topic;
	msg.message.topic.topic.size = strlen(topic);
	msg.message.payload.data = (char *) payload;
	msg.message.payload.len = len;
	msg.message_id = message_id;

	message_id++;

	int ret = mqtt_publish(&mqtt, &msg);
	if (ret != 0) {
		atomic_clear_bit(&state, MQTTC_BIT_INPROGRESS);

		LOG_ERR("Failed to publish to topic %s: %d", log_strdup(topic), ret);
	}

	return ret;
}

int mqttc_set_pollfd(struct pollfd *fds)
{
	fds->fd = mqtt.transport.tls.sock;
	fds->events = POLLIN | POLLHUP | POLLERR;

	return 0;
}

int mqttc_process(struct pollfd *fds)
{
	int ret;

	if (fds->revents & POLLIN) {
		ret = mqtt_input(&mqtt);
		if (ret != 0) {
			LOG_ERR("Failed to read MQTT input: %d", ret);
		}
	} else if (fds->revents & (POLLHUP | POLLERR)) {
		LOG_ERR("MQTT %x connection error/closed", (uint32_t)&mqtt);
		ret = -ENOTCONN;
	}

	ret = mqtt_live(&mqtt);
	if ((ret != 0) && (ret != -EAGAIN)) {
		LOG_ERR("Failed to live MQTT: %d", ret);
		goto exit;
	} else {
		ret = 0;
	}

exit:
	return ret;
}

int mqttc_keepalive_time_left(void)
{
	return mqtt_keepalive_time_left(&mqtt);
}

cursor_buffer_t *mqttc_get_payload_buffer(void)
{
	return &buffer;
}

bool mqttc_ready(void)
{
	const atomic_val_t mask = atomic_get(&state) &
		(MQTTC_FLAG_CONNECTED | MQTTC_FLAG_INPROGRESS);

	/* Ready if connected and not in progress */
	return mask == MQTTC_FLAG_CONNECTED;
}