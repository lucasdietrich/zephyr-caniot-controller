/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CLOUD_BACKOFF_H_
#define _CLOUD_BACKOFF_H_

#include <stdint.h>

typedef enum backoff_method {
	BACKOFF_METHOD_CONST,
	BACKOFF_METHOD_EXPONENTIAL,
	BACKOFF_METHOD_DECORR_JITTER,
} backoff_method_t;

struct backoff
{
	backoff_method_t method: 8;
	uint32_t attempts;
	uint32_t delay; /* Last delay in ms */
};

int backoff_init(struct backoff *backoff, 
		 backoff_method_t method);

int backoff_next(struct backoff *bo);

int backoff_reset(struct backoff *bo);

int backoff_get_delay(struct backoff *bo);

#endif /* _CLOUD_BACKOFF_H_ */