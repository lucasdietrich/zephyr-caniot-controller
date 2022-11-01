/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

#include "settings.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(settings, LOG_LEVEL_INF);

#define STORAGE_PARTITION	storage_partition

#define STORAGE_NODE 		DT_NODE_BY_FIXED_PARTITION_LABEL(STORAGE_PARTITION)
#define STORAGE_AREA_OFFSET 	FIXED_PARTITION_OFFSET(STORAGE_PARTITION)
#define FLASH_NODE 		DT_MTD_FROM_FIXED_PARTITION(STORAGE_NODE)
#define FLASH_DEVICE 		FIXED_PARTITION_DEVICE(storage_partition)

/* or use flash_area_get_sectors() at runtime */
#define SECTOR_COUNT 4U

static const struct device *const flash_dev = FLASH_DEVICE;
static struct nvs_fs fs;
static struct flash_pages_info info;

int settings_init(void)
{
	int rc;

	if (!device_is_ready(flash_dev)) {
		LOG_ERR("Flash device %s is not ready\n", flash_dev->name);
		rc = -EIO;
		goto exit;
	}

	rc = flash_get_page_info_by_offs(flash_dev, STORAGE_AREA_OFFSET, &info);
	if (rc) {
		LOG_ERR("Unable to get page info rc=%d\n", rc);
		goto exit;
	}

	LOG_DBG("info: start_offset=%ld size=%u index=%u",
		info.start_offset, info.size, info.index);

	fs.offset = info.start_offset;
	fs.sector_size = info.size;
	fs.sector_count = SECTOR_COUNT;

	rc = nvs_mount(&fs);
	if (rc) {
		LOG_ERR("Unable to initialize nvs rc=%d\n", rc);
		goto exit;
	}

	ssize_t free_space = nvs_calc_free_space(&fs);
	LOG_INF("Free space: %x bytes\n", free_space);

exit:
	return rc;
}