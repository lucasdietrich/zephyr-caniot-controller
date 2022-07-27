/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _APP_FS_H_
#define _APP_FS_H_

#include <stddef.h>
#include <fs/fs.h>

int app_fs_init(void);

int app_fs_stats(const char *abs_path);

int app_fs_lsdir(const char *path);

int app_fs_file_add(const char *fpath, const char *data, size_t size);

/**
 * @brief Use this function when iterating over the files in a directory.
 * @param path is the path to the directory.
 * @param cb is the callback function to be call for each file.
 * @param user_data is a pointer to the user data to be passed to the callback.
 */
typedef bool (*app_fs_iterate_fs_cb_t)(const char *path,
				    struct fs_dirent *dirent,
				    void *user_data);

int app_fs_iterate_dir_files(const char *path,
			     app_fs_iterate_fs_cb_t callback,
			     void *user_data);

#endif /* _APP_FS_H_ */