

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
#include "creds/manager.h"

#include "app_sections.h"

#include "creds/credentials.h"
#include "creds/manager.h"

#include "cloud/cloud.h"
#include "cloud/aws.h"
#include "cloud/backoff.h"
#include "cloud/utils.h"
#include "cloud/mqttc.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(mqttc, LOG_LEVEL_DBG);

#define TLS_TAG_DEVICE_CERT 2u
#define TLS_TAG_DEVICE_KEY 2u
#define TLS_TAG_CA_CERT 2u

static sec_tag_t sec_tls_tags[] = {
	TLS_TAG_DEVICE_CERT,
};

#define AWS_CERT_DER 1

#define AWS_ENDPOINT_PORT 8883u

#define AWS_PAYLOAD_BUFFER_SIZE 4096u
#define AWS_MQTT_RX_BUFFER_SIZE 256u
#define AWS_MQTT_TX_BUFFER_SIZE 256u

__buf_noinit_section char mqtt_rx_buf[AWS_MQTT_RX_BUFFER_SIZE];
__buf_noinit_section char mqtt_tx_buf[AWS_MQTT_TX_BUFFER_SIZE];
__buf_noinit_section char payload_buf[AWS_PAYLOAD_BUFFER_SIZE];
static cursor_buffer_t buffer = CUR_BUFFER_STATIC_INIT(payload_buf, sizeof(payload_buf));

static struct sockaddr_in addr;
static struct mqtt_client mqtt;

static mqttc_on_publish_cb_t on_publish_cb;
static void *on_publish_user_data;

static uint32_t message_id;

K_SEM_DEFINE(mqtt_lock, 0u, 1u);
K_SEM_DEFINE(mqtt_onpub, 0u, 1u);

/* Forward declarations */
static void mqtt_event_cb(struct mqtt_client *client,
			  const struct mqtt_evt *evt);

static int resolve_hostname(const char *hostname, uint16_t port)
{
	int ret = 0U;
	struct zsock_addrinfo *ai = NULL;

	const struct zsock_addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0
	};
	ret = zsock_getaddrinfo(hostname, "8883", &hints, &ai);
	if (ret != 0) {
		LOG_ERR("failed to resolve hostname err = %d (errno = %d)",
			ret, errno);
	} else {
		memcpy(&addr, ai->ai_addr,
		       MIN(ai->ai_addrlen,
			   sizeof(struct sockaddr_storage)));

		addr.sin_port = htons(port);

		char addr_str[INET_ADDRSTRLEN];
		zsock_inet_ntop(AF_INET,
				&addr.sin_addr,
				addr_str,
				sizeof(addr_str));
		LOG_INF("Resolved %s -> %s", log_strdup(hostname), log_strdup(addr_str));
	}

	zsock_freeaddrinfo(ai);

	return ret;
}

static int setup_credentials(void)
{
	int ret;
	struct cred cert, key, ca;

	/* Setup TLS credentials */
#if AWS_CERT_DER
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_CERTIFICATE_DER, &cert)) == 0);
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_PRIVATE_KEY_DER, &key)) == 0);
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_ROOT_CA1_DER, &ca)) == 0);
#else
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_CERTIFICATE, &cert)) == 0);
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_PRIVATE_KEY, &key)) == 0);
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_ROOT_CA1, &ca)) == 0);
#endif 

	ret = tls_credential_add(TLS_TAG_DEVICE_CERT,
				 TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 cert.data, cert.len);
	CHECK_OR_EXIT(ret == 0);

	ret = tls_credential_add(TLS_TAG_DEVICE_KEY,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 key.data, key.len);
	CHECK_OR_EXIT(ret == 0);

	ret = tls_credential_add(TLS_TAG_CA_CERT,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca.data, ca.len);
	CHECK_OR_EXIT(ret == 0);

exit:
	if (ret != 0) {
		LOG_ERR("Failed to setup credentials ret=%d", ret);
	}
	return ret;
}

static int clear_credentials(void)
{
	int ret;

	ret = tls_credential_delete(TLS_TAG_DEVICE_CERT,
				    TLS_CREDENTIAL_SERVER_CERTIFICATE);
	CHECK_OR_EXIT(ret == 0);

	ret = tls_credential_delete(TLS_TAG_DEVICE_KEY,
				    TLS_CREDENTIAL_PRIVATE_KEY);
	CHECK_OR_EXIT(ret == 0);

	ret = tls_credential_delete(TLS_TAG_CA_CERT,
				    TLS_CREDENTIAL_CA_CERTIFICATE);
	CHECK_OR_EXIT(ret == 0);

exit:
	if (ret != 0) {
		LOG_ERR("Failed to clear credentials ret=%d", ret);
	}
	return ret;
}

static void aws_client_setup(void)
{
	// broker is properly defined at this point
	mqtt_client_init(&mqtt);

	mqtt.broker = &addr;
	mqtt.evt_cb = mqtt_event_cb;

	mqtt.client_id.utf8 = (uint8_t *)CONFIG_AWS_THING_NAME;
	mqtt.client_id.size = sizeof(CONFIG_AWS_THING_NAME) - 1;
	mqtt.password = NULL;
	mqtt.user_name = NULL;

	mqtt.keepalive = CONFIG_MQTT_KEEPALIVE;

	mqtt.protocol_version = MQTT_VERSION_3_1_1;

	mqtt.rx_buf = mqtt_rx_buf;
	mqtt.rx_buf_size = AWS_MQTT_RX_BUFFER_SIZE;
	mqtt.tx_buf = mqtt_tx_buf;
	mqtt.tx_buf_size = AWS_MQTT_TX_BUFFER_SIZE;

	// setup TLS
	mqtt.transport.type = MQTT_TRANSPORT_SECURE;
	struct mqtt_sec_config *const tls_config = &mqtt.transport.tls.config;

	tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
	tls_config->cipher_list = NULL;
	tls_config->sec_tag_list = sec_tls_tags;
	tls_config->sec_tag_count = ARRAY_SIZE(sec_tls_tags);
	tls_config->hostname = CONFIG_AWS_ENDPOINT;
	tls_config->cert_nocopy = TLS_CERT_NOCOPY_OPTIONAL;
}

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
	LOG_DBG("mqtt_evt=%hhx [ %s ]", (uint8_t)evt->type,
		mqtt_evt_get_str(evt->type));

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
	{
		k_sem_give(&mqtt_lock);
		break;
	}

	case MQTT_EVT_DISCONNECT:
		break;

	case MQTT_EVT_PUBLISH:
	{
		const struct mqtt_publish_param *pub = &evt->param.publish;
		handle_published_message(pub);
		k_sem_give(&mqtt_onpub);
		break;
	}

	case MQTT_EVT_PUBACK:
	{
		k_sem_give(&mqtt_lock);
		LOG_INF("Publish acknowledged %d",
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
		LOG_INF("Subscription acknowledged %d",
			evt->param.suback.message_id);
		break;
	}

	case MQTT_EVT_UNSUBACK:
	{
		LOG_INF("Unsubscription acknowledged %d",
			evt->param.unsuback.message_id);
		break;
	}

	case MQTT_EVT_PINGRESP:
		break;
	}
}

static int mqtt_client_try_connect(uint32_t max_attempts)
{
	int ret;
	struct backoff bo;

	backoff_init(&bo, BACKOFF_METHOD_DECORR_JITTER);

	do {
		LOG_DBG("MQTT %p try connect", &mqtt);
		ret = mqtt_connect(&mqtt);
		if (ret < 0) {
			uint32_t delay_ms = backoff_next(&bo);
			LOG_ERR("MQTT %p connect failed ret=%d retry in %u ms",
				&mqtt, ret, delay_ms);
			k_sleep(K_MSEC(delay_ms));
		}
	} while ((ret != 0) && (bo.attempts < max_attempts));

	return ret;
}

int mqttc_init(void)
{
	int ret = setup_credentials();
	if (ret != 0) {
		goto cleanup;
	}

	ret = resolve_hostname(CONFIG_AWS_ENDPOINT, AWS_ENDPOINT_PORT);

	aws_client_setup();

cleanup:
	return ret;
}

int mqttc_cleanup(void)
{
	int ret = mqttc_disconnect();
	LOG_INF("MQTT cleanup %d", ret);

	clear_credentials();

	return 0;
}

int mqttc_try_connect(uint32_t attempts)
{
	return mqtt_client_try_connect(attempts);
}

int mqttc_disconnect(void)
{
	int ret = mqtt_disconnect(&mqtt);
	if (ret != 0) {
		LOG_ERR("Failed to disconnect from broker: %d", ret);
	}
	return ret;
}

int mqttc_set_on_publish_cb(mqttc_on_publish_cb_t cb,
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

	int ret = 0;

	k_sem_take(&mqtt_lock, K_FOREVER);

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

	ret = mqtt_subscribe(&mqtt, &sub_list);
	if (ret != 0) {
		LOG_ERR("Failed to subscribe to topic %s: %d", topic, ret);
		k_sem_give(&mqtt_lock);
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

	int ret = 0;

	k_sem_take(&mqtt_lock, K_FOREVER);

	struct mqtt_publish_param msg;

	msg.retain_flag = 0u;
	msg.message.topic.qos = qos;
	msg.message.topic.topic.utf8 = topic;
	msg.message.topic.topic.size = strlen(topic);
	msg.message.payload.data = (char *) payload;
	msg.message.payload.len = len;
	msg.message_id = message_id;

	message_id++;

	ret = mqtt_publish(&mqtt, &msg);
	if (ret != 0) {
		LOG_ERR("Failed to publish to topic %s: %d", topic, ret);
		k_sem_give(&mqtt_lock);
	}

	return ret;
}

int mqttc_set_pollfd(struct pollfd *fds)
{
	fds->fd = mqtt.transport.tcp.sock;
	fds->events = POLLIN | POLLHUP | POLLERR;

	return 0;
}

int mqttc_process(struct pollfd *fds)
{
	int ret;

	if ((fds->revents & POLLIN) != 0) {
		ret = mqtt_input(&mqtt);
		if (ret != 0) {
			LOG_ERR("Failed to read MQTT input: %d", ret);
		}
	} else if ((fds->revents & (POLLHUP | POLLERR)) != 0) {
		LOG_ERR("MQTT %x connection error/closed", (uint32_t)&mqtt);
		ret = -ENOTCONN;
	}

	ret = mqtt_live(&mqtt);
	if ((ret != 0) && (ret != -EAGAIN)) {
		LOG_ERR("Failed to live MQTT: %d", ret);
		goto exit;
	}
	
exit:
	return ret;
}

int mqttc_keepalive_time_left(void)
{
	return mqtt_keepalive_time_left(&mqtt);
}