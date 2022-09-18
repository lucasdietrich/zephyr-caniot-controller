#include <zephyr.h>

#include <device.h>
#include <storage/flash_map.h>

#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>

#include "dfu.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(dfu, LOG_LEVEL_DBG);

#define FLASH_DEVICE 		FLASH_AREA_DEVICE(image_0)
#define FLASH_AREA_SLOT0_ID 	FLASH_AREA_ID(image_0)
#define FLASH_AREA_SLOT1_ID 	FLASH_AREA_ID(image_1)

void dfu_image_check(void)
{
	struct mcuboot_img_header header;

	while (!device_is_ready(FLASH_DEVICE)) {
		LOG_WRN("Flash device (%p) not ready", FLASH_DEVICE);
		k_sleep(K_MSEC(1));
	}

	/* Check current image */
	int ret = boot_read_bank_header(FLASH_AREA_SLOT0_ID, &header, sizeof(header));
	if (ret) {
		LOG_ERR("Failed to read bank header: %d", ret);
		return;
	}

	const bool confirmed = boot_is_img_confirmed();

	LOG_INF("MCUBOOT version=%x IMAGE size=%x version=%u.%u.%u+%u confirmed=%u",
		header.mcuboot_version, header.h.v1.image_size,
		header.h.v1.sem_ver.major, header.h.v1.sem_ver.minor,
		header.h.v1.sem_ver.revision, header.h.v1.sem_ver.build_num,
		(int)confirmed);

	if ((header.h.v1.sem_ver.major == 0) &&
	    (header.h.v1.sem_ver.minor == 0)) {
		LOG_WRN("Development image (version=0.0.%u+%u)",
			header.h.v1.sem_ver.revision, header.h.v1.sem_ver.build_num);
		return;
	}

	/* Mark image as confirmed */
	if (!confirmed) {
		boot_write_img_confirmed();
	}
}