

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

#define FDS_MQTT 0u
#define FDS_APP 1u

static struct pollfd fds[2u];

static void aws_loop(void)
{
	int ret;

	mqttc_init();

	mqttc_try_connect(MQTTC_TRY_CONNECT_FOREVER);

	mqttc_set_pollfd(&fds[FDS_MQTT]);

	int timeout = mqttc_keepalive_time_left();
	for (;;) {
		ret = poll(&fds[FDS_MQTT], 1u, timeout);
		if (ret >= 0) {
			mqttc_process(&fds[FDS_MQTT]);
		} else {
			LOG_ERR("poll failed: %d", ret);
			break;
		}
	}

	mqttc_cleanup();
	return;

}

static void task(void *_a, void *_b, void *_c)
{
	int ret = 0;

	ret = net_time_wait_synced(K_FOREVER);
	if (ret != 0) {
		goto exit;
	}

	for (;;) {
		aws_loop();

		k_sleep(K_SECONDS(5));
	}

exit:
	LOG_ERR("AWS task exit %d", ret);
	return;
}
