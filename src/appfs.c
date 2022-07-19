/*
 * Copyright (c) 2022 Lucas Dietrich
 * Copyright (c) 2022 Lukasz Majewski, DENX Software Engineering GmbH
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>

#include <fs/fs.h>
#include <ff.h>
// #include <fs/littlefs.h>

#include <storage/flash_map.h>
#include <storage/disk_access.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app_fs, LOG_LEVEL_DBG);

/*____________________________________________________________________________*/

static FATFS fatfs;

static const char *const disk_mount_pt = "/RAM:";

static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fatfs,
	.mnt_point = disk_mount_pt,
};

/*____________________________________________________________________________*/

static int lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		LOG_ERR("Error opening dir %s [%d]", path, res);
		return res;
	}

	LOG_INF("Listing dir %s ...", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			if (res < 0) {
				LOG_ERR("Error reading dir [%d]", res);
			}
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			LOG_INF("[DIR ] %s", entry.name);
		} else {
			LOG_INF("[FILE] %s (size = %zu)",
				   entry.name, entry.size);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}

int app_fs_stats(const char *abs_path)
{
	int rc;
	struct fs_statvfs buf;

	rc = fs_statvfs(abs_path, &buf);
	if (rc < 0) {
		LOG_ERR("fs_statvfs(%s, ..) failed, err=%d", abs_path, rc);
		goto exit;
	}

	LOG_INF("%s bsize=%lu frsize=%lu  bfree=%lu/%lu (blocks)",
		abs_path, buf.f_bsize, buf.f_frsize, buf.f_bfree, buf.f_blocks);

	lsdir(abs_path);
	if (rc < 0) {
		LOG_ERR("lsdir(%s) failed, err=%d", abs_path, rc);
		goto exit;
	}

	rc = 0;

exit:
	return rc;
}

/*____________________________________________________________________________*/

int app_fs_init(void)
{
	/* Mount the FATFS filesystem */
	int rc = fs_mount(&mp);
	if (rc < 0) {
		LOG_ERR("Failed to mount FATFS filesystem, err=%d", rc);
		goto exit;
	}

	app_fs_stats(mp.mnt_point);

exit:
	return rc;
}
