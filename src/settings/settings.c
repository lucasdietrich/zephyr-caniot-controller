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

#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

#define NVS_PARTITION_ID	FIXED_PARTITION_ID(NVS_PARTITION)

#if CONFIG_QEMU_TARGET
#	define NVS_MAX_SECTORS_COUNT	64u
#else
#	define NVS_MAX_SECTORS_COUNT	4u
#endif

/* or use flash_area_get_sectors() at runtime */
static struct nvs_fs fs;
static struct flash_pages_info info;
static struct flash_sector fs_sectors[NVS_MAX_SECTORS_COUNT];

int settings_init(void)
{
	int rc;

	fs.flash_device = NVS_PARTITION_DEVICE;

	if (!device_is_ready(fs.flash_device)) {
		LOG_ERR("Flash device %s is not ready\n", fs.flash_device->name);
		rc = -EIO;
		goto exit;
	}

	rc = flash_get_page_info_by_offs(fs.flash_device, NVS_PARTITION_OFFSET, &info);
	if (rc) {
		LOG_ERR("Unable to get page info rc=%d\n", rc);
		goto exit;
	}

	uint32_t count = ARRAY_SIZE(fs_sectors);
	rc = flash_area_get_sectors(NVS_PARTITION_ID, &count, fs_sectors);
	if (rc) {
		LOG_ERR("Unable to get sector count rc=%d", rc);
		goto exit;
	}

	if (Z_LOG_CONST_LEVEL_CHECK(LOG_LEVEL_DBG)) {
		for (uint32_t i = 0; i < count; i++) {
			LOG_DBG("Sector %u: offset=0x%08x, size=0x%08x", i,
				(uint32_t) fs_sectors[i].fs_off, fs_sectors[i].fs_size);
		}
	}

	LOG_DBG("info: start_offset=%ld size=%u index=%u",
		info.start_offset, info.size, info.index);
	
	fs.offset = info.start_offset;
	fs.sector_size = info.size;
	fs.sector_count = count;

	rc = nvs_mount(&fs);
	if (rc) {
		LOG_ERR("Unable to initialize nvs rc=%d\n", rc);
		goto exit;
	}

	ssize_t free_space = nvs_calc_free_space(&fs);
	LOG_INF("Free space: %u B", free_space);

exit:
	return rc;
}