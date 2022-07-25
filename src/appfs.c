/*
 * Copyright (c) 2022 Lucas Dietrich
 * Copyright (c) 2022 Lukasz Majewski, DENX Software Engineering GmbH
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>

#include <storage/flash_map.h>
#include <storage/disk_access.h>
#include <fs/fs.h>
#include <ff.h>
// #include <fs/littlefs.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app_fs, LOG_LEVEL_DBG);

/*____________________________________________________________________________*/

int app_fs_stats(const char *abs_path);
static int lsdir(const char *path);

/*____________________________________________________________________________*/

#if defined(CONFIG_DISK_DRIVER_RAM)

static FATFS fatfs_ram;

static const char *const ramdisk_mount_pt = "/" CONFIG_DISK_RAM_VOLUME_NAME ":";

static struct fs_mount_t mp_ram = {
	.type = FS_FATFS,
	.fs_data = &fatfs_ram,
	.mnt_point = ramdisk_mount_pt,
};

#endif /* CONFIG_DISK_DRIVER_RAM */

/*____________________________________________________________________________*/

#if defined(CONFIG_DISK_DRIVER_SDMMC) && \
	DT_HAS_COMPAT_STATUS_OKAY(zephyr_mmc_spi_slot)

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

static int disk_get_info(const char *disk_pdrv,
			 struct appfs_disk_info *dinfo)
{
	int rc;
	uint64_t memory_size_mb;
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
	LOG_DBG("Block count %u", block_count);

	if (disk_access_ioctl(disk_pdrv,
			      DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
		LOG_ERR("Unable to get sector size, rc=%d", rc);
		goto exit;
	}
	LOG_DBG("Sector size %u\n", block_size);

	memory_size_mb = (uint64_t)block_count * block_size;
	LOG_DBG("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));

	if (dinfo != NULL) {
		dinfo->memory_size_mb = memory_size_mb;
		dinfo->block_count = block_count;
		dinfo->block_size = block_size;
	}
exit:
	return rc;
}

#endif 

/*____________________________________________________________________________*/

struct fs_mount_t *appfs_mp[] = {
#if defined(CONFIG_DISK_DRIVER_RAM)
	&mp_ram,
#endif
#if defined(CONFIG_DISK_DRIVER_SDMMC) && \
	DT_HAS_COMPAT_STATUS_OKAY(zephyr_mmc_spi_slot)
	& mp_mmc,
#endif
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
	int rc = 0;
	struct fs_mount_t **mp;

	/* get MMC disk info */
	const char *mmc_disk_pdrv = CONFIG_SDMMC_VOLUME_NAME;
	disk_get_info(mmc_disk_pdrv, NULL);

	/* Mount all configured disks */
	for (mp = appfs_mp; mp < appfs_mp + ARRAY_SIZE(appfs_mp); mp++) {
		rc = fs_mount(*mp);
		if (rc < 0) {
			LOG_ERR("fs_mount( %s ) failed, err=%d", 
				log_strdup((*mp)->mnt_point), rc);
			goto exit;
		}

		app_fs_stats((*mp)->mnt_point);
	}

exit:
	return rc;
}
