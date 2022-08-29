/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CREDS_FLASH_CREDS_H_
#define _CREDS_FLASH_CREDS_H_

#include <zephyr.h>

struct flash_cred {
	uint32_t type;
	uint32_t size;
	uint32_t crc32;
	uint32_t unused;
	uint8_t data[];
};

int flash_creds_init(void);

int flash_creds_count(void);

#endif /* _CREDS_FLASH_CREDS_H_ */