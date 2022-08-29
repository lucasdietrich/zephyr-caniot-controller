/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "flash_creds.h"

#include <string.h>

#include <device.h>
#include <devicetree.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(flash_creds);

/* Using flash device here for reading is like directly reading the address
 * in the flash, but it made it portable in case of using an external SPI flash 
 * for instance.
 */

#define FLASH_DEVICE FLASH_AREA_DEVICE(credentials)

#define CREDS_AREA_ID FLASH_AREA_ID(credentials)
#define CREDS_AREA_OFFSET FLASH_AREA_OFFSET(credentials)
#define CREDS_AREA_SIZE FLASH_AREA_SIZE(credentials)

#if CREDS_AREA_SIZE != 128u*1024u
#warning "credentials area size is not 128KB as expected"
#endif

static const struct device *flash_dev = FLASH_DEVICE;

int flash_creds_init(void)
{
	while (!device_is_ready(flash_dev)) {
		LOG_WRN("Flash device (%p) not ready", flash_dev);
		k_sleep(K_MSEC(1));
	}

	return 0;
}

int flash_creds_count(void)
{
	int ret;
	size_t count = 0u;
	const struct flash_area *fa = NULL;
	
	ret = flash_area_open(CREDS_AREA_ID, &fa);
	if (ret) {
		LOG_ERR("flash_area_open(%u, . ) = %d", CREDS_AREA_ID, ret);
		goto exit;
	}


	off_t offset = 0u;
	struct flash_cred fc;

	do {
		/* Read flash credential control block */
		ret = flash_area_read(fa, offset, &fc, sizeof(fc));
		if (ret) {
			LOG_ERR("flash_area_read(%p, %lu, %p, %u) = %d",
				fa, offset, &fc, sizeof(fc), ret);
			flash_area_close(fa);
			goto exit;
		}

		/* Check credential block */
		if (fc.type == 0xFFFFFFFFu) {
			break;
		}

		count++;

	} while (offset < CREDS_AREA_SIZE);

exit:
	if (fa) {
		flash_area_close(fa);
	}

	if (ret == 0) {
		ret = count;
	}

	return ret;
}