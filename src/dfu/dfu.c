/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dfu.h"

#include <zephyr/device.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
LOG_MODULE_REGISTER(dfu, LOG_LEVEL_DBG);

#define FLASH_SLOT0_PARTITION slot0_partition
#define FLASH_SLOT1_PARTITION slot1_partition

#define FLASH_DEVICE		FIXED_PARTITION_DEVICE(FLASH_SLOT0_PARTITION)
#define FLASH_AREA_SLOT0_ID FIXED_PARTITION_ID(FLASH_SLOT0_PARTITION)
#define FLASH_AREA_SLOT1_ID FIXED_PARTITION_ID(FLASH_SLOT1_PARTITION)

int dfu_image_read_header(struct mcuboot_img_header *header)
{
	return boot_read_bank_header(FLASH_AREA_SLOT0_ID, header,
								 sizeof(struct mcuboot_img_header));
}

void dfu_image_check(void)
{
	struct mcuboot_img_header header;

	while (!device_is_ready(FLASH_DEVICE)) {
		LOG_WRN("Flash device (%p) not ready", FLASH_DEVICE);
		k_sleep(K_MSEC(1));
	}

	/* Check current image */
	int ret = dfu_image_read_header(&header);
	if (ret) {
		LOG_ERR("Failed to read bank header: %d", ret);
		return;
	}

	const bool confirmed = boot_is_img_confirmed();

	LOG_INF("MCUBOOT version=%x IMAGE size=0x%x version=%u.%u.%u+%u "
			"confirmed=%u",
			header.mcuboot_version, header.h.v1.image_size, header.h.v1.sem_ver.major,
			header.h.v1.sem_ver.minor, header.h.v1.sem_ver.revision,
			header.h.v1.sem_ver.build_num, (int)confirmed);

	if ((header.h.v1.sem_ver.major == 0) && (header.h.v1.sem_ver.minor == 0)) {
		LOG_WRN("Development image (version=0.0.%u+%u)", header.h.v1.sem_ver.revision,
				header.h.v1.sem_ver.build_num);
	}

	/* Mark image as confirmed */
	if (!confirmed) {
		ret = boot_write_img_confirmed();
		LOG_DBG("Image confirmed: ret=%d", ret);
	}
}