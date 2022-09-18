/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <storage/flash_map.h>
#include <dfu/flash_img.h>
#include <dfu/mcuboot.h>

#include "dfu_server.h"
#include "http_utils.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_dfu, LOG_LEVEL_DBG);

#define FLASH_DEVICE 		FLASH_AREA_DEVICE(image_0)
#define FLASH_AREA_SLOT0_ID 	FLASH_AREA_ID(image_0)
#define FLASH_AREA_SLOT1_ID 	FLASH_AREA_ID(image_1)

int http_dfu_status(struct http_request *req,
		    struct http_response *resp)
{
	LOG_DBG("DFU http_dfu_status");

	return 0;
}

static struct flash_img_context img;

int http_dfu_image_upload(struct http_request *req,
			  struct http_response *resp)
{
	int ret;

	if (http_stream_begins(req)) {
		ret = flash_img_init(&img);

		if (ret) {
			LOG_ERR("flash_img_init error %d", ret);
			goto exit;
		}
	}

	if (http_request_has_chunk_data(req)) {
		ret = flash_img_buffered_write(&img, req->chunk.loc,
					       req->chunk.len, false);
		if (ret) {
			LOG_ERR("flash_img_buffered_write error %d", ret);
			goto exit;
		}
	}

exit:
	return ret;
}

int http_dfu_image_upload_response(struct http_request *req,
				   struct http_response *resp)
{
	int ret;
	size_t size;

	ret = flash_img_buffered_write(&img, NULL, 0, true);
	if (ret) {
		LOG_ERR("flash_img_buffered_write error %d", ret);
		goto exit;
	}

	size = flash_img_bytes_written(&img);

	boot_request_upgrade(BOOT_UPGRADE_TEST);

	// boot_erase_img_bank(FLASH_AREA_SLOT1_ID);

	LOG_INF("DFU image upload complete, ret=%d size=%u", ret, size);
exit:
	return ret;
}