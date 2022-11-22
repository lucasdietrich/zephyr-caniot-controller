/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "net_time.h"

#include <zephyr/net/sntp.h>
#include <zephyr/posix/time.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_time, LOG_LEVEL_INF);

static void sntp_handler(struct k_work *work);

struct sntp_context {
	const char *server; /* SNTP server */

	uint32_t failures; /* Number of failures */
	uint32_t count; /* Number of times we synced time */
	time_t last_sync; /* Last time we synced time */

	struct k_work work; /* Work item to sync time */

	struct k_mutex mutex; /* Mutex to protect context access from other threads */
	struct k_condvar condvar; /* Condvar to signal when the time is synced */
};

struct sntp_context sntp_ctx = {
	.server = "0.fr.pool.ntp.org",
	.failures = 0U,
	.count = 0U,
	.last_sync = 0U,
	.work = Z_WORK_INITIALIZER(sntp_handler),
	.mutex = Z_MUTEX_INITIALIZER(sntp_ctx.mutex),
	.condvar = Z_CONDVAR_INITIALIZER(sntp_ctx.condvar),
};

static void sntp_handler(struct k_work *work)
{
	int ret;
	struct sntp_time time;
	struct timespec tspec;
	struct sntp_context *ctx = CONTAINER_OF(work, struct sntp_context, work);

	/* https://www.pool.ntp.org/zone/fr */
	ret = sntp_simple(ctx->server, 10U * MSEC_PER_SEC, &time);

	k_mutex_lock(&ctx->mutex, K_FOREVER);

	if (ret == 0) {
		tspec.tv_sec = time.seconds;
		tspec.tv_nsec = ((uint64_t)time.fraction * (1000LU * 1000LU * 1000LU)) >> 32;

		clock_settime(CLOCK_REALTIME, &tspec);

		ctx->count++;
		ctx->last_sync = tspec.tv_sec;

		ret = k_condvar_broadcast(&ctx->condvar);

		LOG_INF("SNTP time from %s:123 = %u, %d thread(s) signaled",
			ctx->server, (uint32_t)time.seconds, ret);
	} else {
		ctx->failures++;

		/* todo retry later */
		LOG_ERR("sntp_simple() failed with error = %d", ret);

		// k_sleep(K_SECONDS(100));

		/* Reschedule the work */
		net_time_sync();
	}

	k_mutex_unlock(&ctx->mutex);
}

int net_time_sync(void)
{
	return k_work_submit(&sntp_ctx.work);
}

uint32_t net_time_get(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
		LOG_ERR("Invalid net time ! %d", -1);
		return 0;
	}

	return ts.tv_sec;
}

static inline bool time_synced(void)
{
	return sntp_ctx.count > 0;
}

bool net_time_is_synced(void)
{
	bool synced;

	k_mutex_lock(&sntp_ctx.mutex, K_FOREVER);

	synced = time_synced();

	k_mutex_unlock(&sntp_ctx.mutex);

	return false;
}

int net_time_wait_synced(k_timeout_t timeout)
{
	int ret = 0;

	k_mutex_lock(&sntp_ctx.mutex, K_FOREVER);

	bool synced = time_synced();
	if (synced == false) {
		ret = k_condvar_wait(&sntp_ctx.condvar, &sntp_ctx.mutex, timeout);
	}

	k_mutex_unlock(&sntp_ctx.mutex);

	return ret;
}

void net_time_show(void)
{
	time_t timestamp = net_time_get();

	struct tm time_infos;

	gmtime_r(&timestamp, &time_infos);

	printk("Local (Europe/Paris) Date and time : %04d/%02d/%02d %02d:%02d:%02d\n",
	       time_infos.tm_year + 1900, time_infos.tm_mon + 1, time_infos.tm_mday,
	       time_infos.tm_hour, time_infos.tm_min, time_infos.tm_sec);
}