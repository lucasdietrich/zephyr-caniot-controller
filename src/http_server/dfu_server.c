/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>

#include <zephyr/data/json.h>


#include "dfu/dfu.h"
#include "dfu_server.h"
#include "rest_server.h"
#include "http_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(http_dfu, LOG_LEVEL_DBG);

#define FLASH_SLOT0_PARTITION 	slot0_partition
#define FLASH_SLOT1_PARTITION 	slot1_partition

#define FLASH_DEVICE 		FIXED_PARTITION_DEVICE(FLASH_SLOT0_PARTITION)
#define FLASH_AREA_SLOT0_ID 	FIXED_PARTITION_ID(FLASH_SLOT0_PARTITION)
#define FLASH_AREA_SLOT1_ID 	FIXED_PARTITION_ID(FLASH_SLOT1_PARTITION)

struct json_mcuboot_img_header {
	uint32_t mcuboot_version;
	uint32_t image_size;
	uint32_t version_major;
	uint32_t version_minor;
	uint32_t version_revision;
	uint32_t version_build;
};

static const struct json_obj_descr json_mcuboot_img_header_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_mcuboot_img_header, mcuboot_version, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_mcuboot_img_header, image_size, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_mcuboot_img_header, version_major, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_mcuboot_img_header, version_minor, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_mcuboot_img_header, version_revision, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_mcuboot_img_header, version_build, JSON_TOK_NUMBER),
};

int http_dfu_status(struct http_request *req,
		    struct http_response *resp)
{
	struct mcuboot_img_header header;

	int ret = dfu_image_read_header(&header);

	if (ret == 0) {
		struct json_mcuboot_img_header json = {
			.mcuboot_version = header.mcuboot_version,
			.image_size = header.h.v1.image_size,
			.version_major = header.h.v1.sem_ver.major,
			.version_minor = header.h.v1.sem_ver.minor,
			.version_revision = header.h.v1.sem_ver.revision,
			.version_build = header.h.v1.sem_ver.build_num,
		};

		ret = rest_encode_response_json(resp,
						&json,
						json_mcuboot_img_header_descr,
						ARRAY_SIZE(json_mcuboot_img_header_descr));
	}

	return ret;
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