/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CREDS_FLASH_CREDS_H_
#define _CREDS_FLASH_CREDS_H_

#include <zephyr/kernel.h>

#include "credentials.h"

#define FLASH_CRED_BLOCK_SIZE 0x1000u
#define FLASH_CRED_CONTROL_BLOCK_SIZE 16u
#define FLASH_CRED_MAX_SIZE (FLASH_CRED_BLOCK_SIZE - FLASH_CRED_CONTROL_BLOCK_SIZE)

#define FLASH_CREDS_SLOTS_MAX_COUNT 32u

struct flash_cred_ctrl
{
	union {
		struct
		{
			cred_id_t id : 8u;
			cred_format_t format : 8u;
			uint32_t strength : 8u;
			uint32_t version : 8u;
		};
		uint32_t descr;
	};
	uint32_t size;
	uint32_t crc32;
	uint32_t revoked;
};

struct flash_cred_buf {
	struct flash_cred_ctrl ctrl;
	char data[];
};

typedef enum {
	FLASH_CRED_VALID, /* Block contains a credential */
	FLASH_CRED_UNALLOCATED, /* Block is unallocated */
	FLASH_CRED_SIZE_BLANK, /* Block allocated, but not written */
	FLASH_CRED_SIZE_INVALID, /* Block allocated, but size is invalid */
	FLASH_CRED_CRC_MISMATCH, /* Block has a CRC mismatch */
	FLASH_CRED_REVOKED, /* Block has been revoked */

	FLASH_CRED_NULL, /* Invalid credential block given */

} flash_cred_status_t;

extern const uint32_t flash_creds_slots_count;

int flash_creds_init(void);

int flash_creds_count(void);

int flash_creds_iterate(bool (*cb)(struct flash_cred_buf *,
				   flash_cred_status_t,
				   void *),
			void *user_data);

int flash_cred_get_slot_from_addr(struct flash_cred_buf *fc);

int flash_cred_get(cred_id_t id, struct cred *c);

int flash_cred_copy_to(cred_id_t id, char *buf, size_t *size);

int flash_cred_write(cred_id_t id, const struct cred *c);

int flash_cred_ctrl_read(uint16_t slot, struct flash_cred_ctrl *ctrl);

#endif /* _CREDS_FLASH_CREDS_H_ */