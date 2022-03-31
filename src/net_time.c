#include "net_time.h"

#include <net/sntp.h>
#include <posix/time.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(net_time, LOG_LEVEL_INF);

static void sntp_handler(struct k_work *work);

struct sntp_context {
	const char *server; /*!< SNTP server */
	struct k_work work;
	uint32_t failures; /* Number of failures */
	uint32_t count; /* Number of times we synced time */
	time_t last_sync; /* Last time we synced time */
};

struct sntp_context sntp_ctx = {
	.server = "0.fr.pool.ntp.org",
	.work = Z_WORK_INITIALIZER(sntp_handler),
	.failures = 0U,
	.count = 0U,
	.last_sync = 0U,
};

static void sntp_handler(struct k_work *work)
{
        int ret;
        struct sntp_time time;
        struct timespec tspec;
	struct sntp_context *ctx = CONTAINER_OF(work, struct sntp_context, work);

        /* https://www.pool.ntp.org/zone/fr */
	ret = sntp_simple(ctx->server, 10U * MSEC_PER_SEC, &time);
        if (ret != 0) {
                /* todo retry later */
                LOG_ERR("sntp_simple() failed with error = %d", ret);

		ctx->failures++;

		/* Reschedule the work */
		net_time_sync();

                return;
        }

        tspec.tv_sec = time.seconds;
	tspec.tv_nsec = ((uint64_t)time.fraction * (1000LU * 1000LU * 1000LU)) >> 32;

	clock_settime(CLOCK_REALTIME, &tspec);

	ctx->count++;
	ctx->last_sync = tspec.tv_sec;

	LOG_INF("SNTP time from %s:123 = %llu",
		log_strdup(ctx->server), time.seconds);
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

bool net_time_is_synced(void)
{
	return sntp_ctx.count > 0;
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