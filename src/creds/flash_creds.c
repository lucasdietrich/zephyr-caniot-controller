/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "flash_creds.h"

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/crc.h>
LOG_MODULE_REGISTER(flash_creds);

/* Using flash device here for reading is like directly reading the address
 * in the flash, but it made it portable in case of using an external SPI flash
 * for instance.
 */

// todo check for
// CONFIG_FLASH_BASE_ADDRESS
// CONFIG_SOC_FLASH_STM32

#define CREDS_PARTITION credentials_partition

#define CREDS_FLASH_DEVICE FIXED_PARTITION_DEVICE(CREDS_PARTITION)
#define CREDS_AREA_ID	   FIXED_PARTITION_ID(CREDS_PARTITION)
#define CREDS_AREA_OFFSET  FIXED_PARTITION_OFFSET(CREDS_PARTITION)
#define CREDS_AREA_SIZE	   FIXED_PARTITION_SIZE(CREDS_PARTITION)

#define FLASH_CREDS_SLOTS_COUNT (CREDS_AREA_SIZE / FLASH_CRED_BLOCK_SIZE)

const uint32_t flash_creds_slots_count =
	MIN(FLASH_CREDS_SLOTS_COUNT, FLASH_CREDS_SLOTS_MAX_COUNT);

#define BLANK 0xFFFFFFFFu

#if CREDS_AREA_SIZE != 64u * 1024u
#warning "credentials area size is not 128KB as expected"
#endif

static const struct device *flash_dev = CREDS_FLASH_DEVICE;

int flash_creds_init(void)
{
	while (!device_is_ready(flash_dev)) {
		LOG_WRN("Flash device (%p) not ready", flash_dev);
		k_sleep(K_MSEC(1));
	}

	return 0;
}

static struct flash_cred_buf *get_cred_addr(cred_id_t id)
{
	return (struct flash_cred_buf *)(CREDS_AREA_OFFSET +
					 (id * FLASH_CRED_BLOCK_SIZE));
}

static flash_cred_status_t check_cred(struct flash_cred_buf *p, bool assume_checksum)
{
	if (!p) {
		return FLASH_CRED_NULL;
	}

	if (p->header.descr == BLANK) {
		return FLASH_CRED_UNALLOCATED;
	}

	if (p->header.size == 0) {
		return FLASH_CRED_SIZE_BLANK;
	}

	if (p->header.size > FLASH_CRED_MAX_SIZE) {
		return FLASH_CRED_SIZE_INVALID;
	}

	if (p->header.revoked != BLANK) {
		return FLASH_CRED_REVOKED;
	}

	if (!assume_checksum) {
		uint32_t crc_calc = crc32_ieee((uint8_t *)p->data, p->header.size);
		if (crc_calc != p->header.crc32) {
			return FLASH_CRED_CRC_MISMATCH;
		}
	}

	return FLASH_CRED_VALID;
}

int flash_creds_iterate(bool (*cb)(struct flash_cred_buf *, flash_cred_status_t, void *),
			void *user_data)
{
	uint16_t slot = 0u;

	if (!cb) {
		return -EINVAL;
	}

	while (slot < flash_creds_slots_count) {
		struct flash_cred_buf *c   = get_cred_addr(slot);
		flash_cred_status_t status = check_cred(c, false);

		if (cb(c, status, user_data) == false) {
			break;
		}

		slot++;
	}

	return 0;
}

static bool
count_cb(struct flash_cred_buf *c, flash_cred_status_t status, void *user_data)
{
	uint16_t *count = (uint16_t *)user_data;

	if (status == FLASH_CRED_VALID) {
		(*count)++;
	}

	return true;
}

int flash_creds_count(void)
{
	uint16_t count = 0u;

	int ret = flash_creds_iterate(count_cb, &count);

	return (ret == 0) ? count : ret;
}

int flash_cred_get_slot_from_addr(struct flash_cred_buf *fc)
{
	if (!fc) {
		return -EINVAL;
	}

	uint32_t addr	= (uint32_t)fc;
	uint32_t offset = addr - CREDS_AREA_OFFSET;

	return offset / FLASH_CRED_BLOCK_SIZE;
}

bool cred_find_cb(struct flash_cred_buf *fc, flash_cred_status_t status, void *user_data)
{
	struct cred *const c = user_data;

	const cred_id_t search_for_id = c->len;

	if ((status == FLASH_CRED_VALID) && (search_for_id == fc->header.id)) {
		c->len	= fc->header.size;
		c->data = fc->data;
		return false;
	}

	return true;
}

int flash_cred_get(cred_id_t id, struct cred *c)
{
	int ret;

	if (!c) {
		return -EINVAL;
	}

	c->data = NULL;
	c->len	= id; /* Hehe, Don't do this at home */

	ret = flash_creds_iterate(cred_find_cb, c);
	if (ret != 0) {
		return ret;
	} else if (c->data == NULL) {
		c->len = 0;
		return -ENOENT;
	}

	return 0;
}

int flash_cred_copy_to(cred_id_t id, char *buf, size_t *size)
{
	struct cred c;

	if (!buf || !size) {
		return -EINVAL;
	}

	int ret = flash_cred_get(id, &c);
	if (ret == 0) {
		if (*size >= c.len) {
			memcpy(buf, c.data, c.len);
			*size = c.len;
			ret   = c.len;
		} else {
			*size = 0;
			ret   = -ENOMEM;
		}
	}
	return ret;
}