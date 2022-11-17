/*
 * Copyright (c) 2022 Lucas Dietrich
 * Copyright (c) 2022 Lukasz Majewski, DENX Software Engineering GmbH
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/storage/flash_map.h>
#include <zephyr/storage/disk_access.h>
#include <ff.h>
// #include <fs/littlefs.h>

#include "appfs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_fs, LOG_LEVEL_DBG);

#define APPFS_LIST_ROOT_FILES 0

#if defined(CONFIG_DISK_DRIVER_SDMMC) && \
	!DT_HAS_COMPAT_STATUS_OKAY(zephyr_sdhc_spi_slot)
#warning "SDMMC driver enabled but no compatible slot found"
#endif

int app_fs_stats(const char *abs_path);
int app_fs_lsdir(const char *path);

#if defined(CONFIG_DISK_DRIVER_RAM)

static FATFS fatfs_ram;

static const char *const ramdisk_mount_pt = "/" CONFIG_DISK_RAM_VOLUME_NAME ":";

static struct fs_mount_t mp_ram = {
	.type = FS_FATFS,
	.fs_data = &fatfs_ram,
	.mnt_point = ramdisk_mount_pt,
};

#endif /* CONFIG_DISK_DRIVER_RAM */

#if defined(CONFIG_DISK_DRIVER_SDMMC) && \
	DT_HAS_COMPAT_STATUS_OKAY(zephyr_sdhc_spi_slot)

static FATFS fat_fs_mmc;

/*
*  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
*  in ffconf.h
*/
static const char *const mmcdisk_mount_pt = "/" CONFIG_SDMMC_VOLUME_NAME ":";

/* mounting info */
static struct fs_mount_t mp_mmc = {
	.type = FS_FATFS,
	.fs_data = &fat_fs_mmc,
	.mnt_point = mmcdisk_mount_pt,
};

struct appfs_disk_info {
	uint64_t memory_size_mb;
	uint32_t block_count;
	uint32_t block_size;
};

int disk_get_info(const char *disk_pdrv,
			 struct appfs_disk_info *dinfo)
{
	int rc;
	uint32_t memory_size_mb;
	uint32_t block_count;
	uint32_t block_size;

	rc = disk_access_init(disk_pdrv);
	if (rc != 0) {
		LOG_ERR("Storage init error, rc=%d", rc);
		goto exit;
	}

	rc = disk_access_ioctl(disk_pdrv,
			       DISK_IOCTL_GET_SECTOR_COUNT,
			       &block_count);
	if (rc != 0) {
		LOG_ERR("Unable to get sector count, rc=%d", rc);
		goto exit;
	}

	if (disk_access_ioctl(disk_pdrv,
			      DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
		LOG_ERR("Unable to get sector size, rc=%d", rc);
		goto exit;
	}

	memory_size_mb = ((uint64_t)block_count * block_size) >> 20u;

	if (dinfo != NULL) {
		dinfo->memory_size_mb = memory_size_mb;
		dinfo->block_count = block_count;
		dinfo->block_size = block_size;
	}
	LOG_DBG("SD Blocks %u of size %u, total size %uMB",
		block_count, block_size, memory_size_mb);
exit:
	return rc;
}

#endif 




struct fs_mount_t *appfs_mp[] = {
#if defined(CONFIG_DISK_DRIVER_RAM)
	&mp_ram,
#endif
#if defined(CONFIG_DISK_DRIVER_SDMMC) && \
	DT_HAS_COMPAT_STATUS_OKAY(zephyr_sdhc_spi_slot)
	&mp_mmc,
#endif
};

int app_fs_lsdir(const char *path)
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

int app_fs_file_add(const char *fpath, const char *data, size_t size)
{
	int rc, written = 0;
	struct fs_file_t file;

	fs_file_t_init(&file);
	rc = fs_open(&file, fpath, FS_O_CREATE | FS_O_WRITE);
	if (rc < 0) {
		LOG_ERR("FAIL: open %s: %d", fpath, rc);
		return rc;
	}

	rc = fs_seek(&file, 0, FS_SEEK_SET);
	if (rc < 0) {
		LOG_ERR("FAIL: seek %s: %d", fpath, rc);
		goto out;
	}

	rc = fs_write(&file, data, size);
	if (rc < 0) {
		LOG_ERR("FAIL: write %s: %d", fpath, rc);
		goto out;
	}

	written = rc;

out:
	rc = fs_close(&file);
	if (rc < 0) {
		LOG_ERR("FAIL: close %s: %d", fpath, rc);
		return rc;
	}

	return written;
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

	app_fs_lsdir(abs_path);
	if (rc < 0) {
		LOG_ERR("app_fs_lsdir(%s) failed, err=%d", abs_path, rc);
		goto exit;
	}

	rc = 0;

exit:
	return rc;
}


int app_fs_init(void)
{
	int rc = 0;
	struct fs_mount_t **mp;

#if defined(CONFIG_SDMMC_LOG_LEVEL_DBG)
	/* get MMC disk info */
	const char *mmc_disk_pdrv = CONFIG_SDMMC_VOLUME_NAME;
	disk_get_info(mmc_disk_pdrv, NULL);
#endif

	/* Mount all configured disks */
	for (mp = appfs_mp; mp < appfs_mp + ARRAY_SIZE(appfs_mp); mp++) {
		rc = fs_mount(*mp);
		if (rc < 0) {
			LOG_ERR("fs_mount( %s ) failed, err=%d",
				(*mp)->mnt_point, rc);
			goto exit;
		}

		LOG_INF("FS mounted %s", (*mp)->mnt_point);
		
#if APPFS_LIST_ROOT_FILES
		app_fs_stats((*mp)->mnt_point);
#endif 
	}

exit:
	return rc;
}

int app_fs_iterate_dir_files(const char *path,
			     app_fs_iterate_fs_cb_t callback,
			     void *user_data)
{
	int res = -EINVAL;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	if (callback == NULL) {
		goto ret;
	}

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		LOG_ERR("Error opening dir %s [%d]", path, res);
		return res;
	}

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

		/* Callback and break if it returns false */
		if (callback(path, &entry, user_data) == false) {
			break;
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

ret:
	return res;
}

int app_fs_filepath_normalize(const char *path, char *out_path, size_t out_size)
{
	if (!path || !out_path || !out_size) {
		return -EINVAL;
	}

	const int add_slash = path[0] == '/' ? 0 : 1;
	const size_t len = strlen(path) + add_slash;

	if (len >= out_size) {
		return -ENOMEM;
	}

	strncpy(out_path + add_slash, path, out_size - add_slash);
	out_path[0] = '/';

	return len;
}