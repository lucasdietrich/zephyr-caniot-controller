

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

#include <sys/eventfd.h>

#include <net/socket.h>
#include <net/net_core.h>

#include "net_time.h"

#include "cloud/mqttc.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cloud, LOG_LEVEL_DBG);

#define AWS_THREAD_STACK_SIZE 2096u

static void task(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(cloud_thread, AWS_THREAD_STACK_SIZE, task,
		NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, 0);

extern int cloud_app_init(void);
extern int cloud_app_process(atomic_val_t flags);
extern int cloud_app_cleanup(void);

extern struct cloud_platform aws_platform;

struct cloud_platform *cloud_platform_get(void)
{
	return &aws_platform;
}

#define FDS_MQTT 0u
#define FDS_APP 1u

static struct pollfd fds[2u];

static atomic_t app_fd_flags = ATOMIC_INIT(0u);

enum cloud_state {
	STATE_INIT,
	STATE_PROVISION,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_DISCONNECTING,
	STATE_DISCONNECTED,
	STATE_ERROR,
};

static const char *cloud_state_to_str(enum cloud_state state)
{
	switch (state) {
	case STATE_INIT:
		return "STATE_INIT";
	case STATE_PROVISION:
		return "STATE_PROVISION";
	case STATE_CONNECTING:
		return "STATE_CONNECTING";
	case STATE_CONNECTED:
		return "STATE_CONNECTED";
	case STATE_DISCONNECTING:
		return "STATE_DISCONNECTING";
	case STATE_DISCONNECTED:
		return "STATE_DISCONNECTED";
	case STATE_ERROR:
		return "STATE_ERROR";
	default:
		return "UNKNOWN";
	}
}

enum cloud_state state = STATE_INIT;

static void set_state(enum cloud_state new_state)
{
	if (state != new_state) {
		LOG_INF("State changed: %s (%d) -> %s (%d)", 
			log_strdup(cloud_state_to_str(state)), state, 
			log_strdup(cloud_state_to_str(new_state)), new_state);
		state = new_state;
	}
}

static bool state_machine(void)
{
	int ret = 0;

	switch (state) {
	case STATE_INIT:
	{
		/* Initialize the MQTT client */
		ret = mqttc_init();
		if (ret == 0) {
			set_state(STATE_CONNECTING);
		} else {
			LOG_ERR("Failed to initialize MQTT client err=%d", ret);
			set_state(STATE_ERROR);
		}
		break;
	}
	case STATE_CONNECTING:
	{
		/* Try to connect to the MQTT broker */
		if (mqttc_try_connect(MQTTC_TRY_CONNECT_FOREVER) == 0) {
			mqttc_set_pollfd(&fds[FDS_MQTT]);
			set_state(STATE_CONNECTED);
			cloud_app_init();
		} else {
			LOG_ERR("Failed to connect to MQTT broker err=%d", ret);
			set_state(STATE_ERROR);
		}
		break;
	}
	case STATE_CONNECTED:
	{
		/* Wait for events */
		int timeout = mqttc_keepalive_time_left();
		ret = poll(&fds[FDS_MQTT], 2u, timeout);
		if (ret >= 0) {
			if (mqttc_process(&fds[FDS_MQTT]) < 0) {
				cloud_app_cleanup();
				set_state(STATE_DISCONNECTING);
			}

			if (fds[FDS_APP].revents & POLLIN) {
				/* Clear the event */
				eventfd_t _val;
				eventfd_read(fds[FDS_APP].fd, &_val);
				(void) _val;

				cloud_app_process(atomic_clear(&app_fd_flags));
			}
		} else {
			LOG_ERR("poll failed: %d", ret);
			set_state(STATE_ERROR);
		}
		break;
	}
	case STATE_DISCONNECTING:
	{
		mqttc_disconnect();

		set_state(STATE_DISCONNECTED);

		break;
	}
	case STATE_DISCONNECTED:
	{
		mqttc_cleanup();

		set_state(STATE_INIT);

		break;
	}
	case STATE_ERROR:
		cloud_app_cleanup();
		return false;
	default:
		break;
	}

	return true;
}

static void task(void *_a, void *_b, void *_c)
{
	int ret = 0;

	ret = net_time_wait_synced(K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to sync time: %d", ret);
		goto exit;
	}

	int appfd = eventfd(0, 0);
	if (appfd < 0) {
		LOG_ERR("Failed to create eventfd: %d", appfd);
		goto exit;
	}

	fds[FDS_APP].fd = appfd;
	fds[FDS_APP].events = POLLIN;

	for (;;) {
		if (!state_machine()) {
			break;
		}
	}

exit:
	LOG_ERR("Cloud task exit %d", ret);
	return;
}

void cloud_notify(atomic_val_t flags)
{
	atomic_or(&app_fd_flags, flags);

	eventfd_write(fds[FDS_APP].fd, 1);
}