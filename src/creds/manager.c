/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "manager.h"

#include "hardcoded_creds.h"
#include "flash_creds.h"
#include "fs_creds.h"
#include "utils.h"

#include "utils/misc.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(creds_manager, LOG_LEVEL_DBG);

int creds_manager_init(void)
{
	int ret = -ENOTSUP;

#if defined(CONFIG_CREDS_HARDCODED)
	ret = harcoded_creds_init();
	CHECK_OR_EXIT(ret == 0);
#endif

#if defined(CONFIG_CREDS_FLASH)
	ret = flash_creds_init();
	CHECK_OR_EXIT(ret == 0);

	int count = flash_creds_count();
	if (count < 0) {
		LOG_ERR("Failed to count credentials in FLASH, err=%d", count);
	} else {
		LOG_INF("Found %d credentials in FLASH", count);
	}
#endif

#if defined(CONFIG_CREDS_FS)
	ret = fs_creds_init();
	CHECK_OR_EXIT(ret == 0);
#endif


exit:
	return ret;
}

extern int harcoded_cred_get(cred_id_t id, struct cred *c);

int cred_get(cred_id_t id, struct cred *c)
{
	int ret = -ENOENT;

#if defined(CONFIG_CREDS_HARDCODED)
	ret = harcoded_cred_get(id, c);
	if (ret == 0) {
		goto exit;
	}
#endif 

#if defined(CONFIG_CREDS_FLASH)
	ret = flash_cred_get(id, c);
	if (ret == 0) {
		goto exit;
	}
#endif

#if defined(CONFIG_CREDS_FS)
	ret = fs_cred_get(id, c);
	if (ret == 0) {
		goto exit;
	}
#endif

exit:
	if (ret != 0) {
		LOG_ERR("Failed to get credential %s, err=%d", cred_id_to_str(id), ret);
	}

	return ret;
}

int cred_copy_to(cred_id_t id, char *buf, size_t *size)
{
	int ret = -ENOENT;

#if defined(CONFIG_CREDS_HARDCODED)
	ret = harcoded_cred_copy_to(id, buf, size);
	if (ret >= 0 || ret == -ENOMEM) {
		goto exit;
	}
#endif 

#if defined(CONFIG_CREDS_FLASH)
	ret = flash_cred_copy_to(id, buf, size);
	if (ret >= 0 || ret == -ENOMEM) {
		goto exit;
	}
#endif

#if defined(CONFIG_CREDS_FS)
	ret = fs_cred_copy_to(id, buf, size);
	if (ret >= 0 || ret == -ENOMEM) {
		goto exit;
	}
#endif

exit:
	return ret;
}