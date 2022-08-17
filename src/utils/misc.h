/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UTILS_MISC_H_
#define _UTILS_MISC_H_

#include <kernel.h>
#include <stdint.h>

static inline uint32_t k_uptime_delta32(uint32_t *reftime)
{
	__ASSERT_NO_MSG(reftime);

	uint32_t uptime, delta;

	uptime = k_uptime_get_32();
	delta = uptime - *reftime;
	*reftime = uptime;

	return delta;
}

#endif /* _UTILS_H_ */