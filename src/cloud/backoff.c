/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "backoff.h"

#include <zephyr.h>

#include <random/rand32.h>

/* Exponential Backoff And Jitter
 * https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter/
 */

#define BASE_MS (1u*MSEC_PER_SEC)
#define RETRY_DELAY_CAP_MS (1800u*MSEC_PER_SEC)
#define RETRY_MULTIPLIER 3u

#define CONST_BACKOFF_MS (1u*MSEC_PER_SEC)

#define MAX_EXPONENT 21u

static uint32_t get_random_in_range(uint32_t a, uint32_t b)
{
	if (b <= a) {
		return a;
	}

	uint32_t range = b - a;
	uint32_t random = sys_rand32_get();

	return a + (random % range);
}

int backoff_init(struct backoff *bo,
		 backoff_method_t method)
{
	if (bo == NULL) {
		return -EINVAL;
	}

	bo->delay = 0u;
	bo->method = method;
	bo->attempts = 0u;

	return 0;
}

int backoff_next(struct backoff *bo)
{
	if (bo == NULL) {
		return -EINVAL;
	}

	bo->attempts++;

	switch (bo->method) {
	case BACKOFF_METHOD_CONST:
		bo->delay = CONST_BACKOFF_MS;
		break;

	case BACKOFF_METHOD_EXPONENTIAL:
	{
		uint32_t exp = (bo->attempts > MAX_EXPONENT) ? MAX_EXPONENT : bo->attempts;
		uint32_t tmp = MIN(RETRY_DELAY_CAP_MS, (BASE_MS << exp));
		bo->delay = (tmp >> 1u) + get_random_in_range(0u, tmp >> 1u);
		break;
	}

	case BACKOFF_METHOD_DECORR_JITTER:
		bo->delay = MIN(
			RETRY_DELAY_CAP_MS,
			get_random_in_range(
				BASE_MS,
				RETRY_MULTIPLIER * bo->delay
			)
		);
		break;
	default:
		return -ENOTSUP;
	}

	return bo->delay;
}

int backoff_reset(struct backoff *bo)
{
	if (bo == NULL) {
		return -EINVAL;
	}

	bo->delay = 0u;
	bo->attempts = 0u;

	return 0;
}

int backoff_get_delay(struct backoff *bo)
{
	if (bo == NULL) {
		return -EINVAL;
	}

	return bo->delay;
}