/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cloud.h"
#include "cloud_internal.h"

#include "net_time.h"

#include "AWS/aws.h"

#include "app_sections.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cloud, LOG_LEVEL_DBG);

#define CLOUD_THREAD_STACK_SIZE 4096u
#define CLOUD_MQTT_BUFFER_SIZE 8024u

__buf_noinit_section char mqtt_buf[CLOUD_MQTT_BUFFER_SIZE];

static void task(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(cloud_thread, CLOUD_THREAD_STACK_SIZE, task, 
		NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, SYS_FOREVER_MS);

static struct cloud_platform *platform = NULL;
#define p platform

static bool check_platform(struct cloud_platform *platform)
{
	bool success = true;

	success &= platform->name != NULL;
	success &= platform->init != NULL;
	success &= platform->connect != NULL;
	success &= platform->publish != NULL;

	return success;
}

int cloud_init(void)
{
	int ret = 0;

#if defined(CONFIG_AWS)
	platform = &aws_platform;
#endif

	if (platform == NULL) {
		LOG_WRN("No cloud platform selected platform=%p", platform);
		ret = -ENOTSUP;
		goto exit;
	}

	/* Check platform */
	if (!check_platform(platform)) {
		LOG_ERR("Invalid cloud platform %p", platform);
		ret = -EINVAL;
		goto exit;
	}

	/* Initialize platform */
	ret = p->init();
	if (ret) {
		LOG_ERR("Failed to initialize %s platform ret=%d", p->name, ret);
		goto exit;
	}

	LOG_INF("Initialized %s platform", p->name);

	k_thread_start(cloud_thread);
exit:
	return ret;
}

static void task(void *_a, void *_b, void *_c)
{
	int ret;

	net_time_wait_synced(K_FOREVER);

	for (;;) {
		k_sleep(K_SECONDS(1));
	}
}