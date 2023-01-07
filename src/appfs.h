/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _APP_FS_H_
#define _APP_FS_H_

#include <stddef.h>

#include <zephyr/fs/fs.h>

/**
 * @brief Initialize the application file system(s)
 *
 * @return int
 */
int app_fs_init(void);

/**
 * @brief Create a file at the given path and copy the given buffer into it
 *
 * @param fpath Path to the file to be created
 * @param data Buffer to copy into the file
 * @param size Size of the buffer
 * @return int
 */
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

/**
 * @brief Iterate over the files in a directory.
 *
 * @param path
 * @param callback
 * @param user_data
 * @return int
 */
int app_fs_iterate_dir_files(const char *path,
			     app_fs_iterate_fs_cb_t callback,
			     void *user_data);

/**
 * @brief Append a slash ('/') to the beginning of the path if it doesn't have
 * one. and copy the result to the output buffer.
 *
 * @param path
 * @param out_path
 * @param out_size
 * @return int
 */
int app_fs_filepath_normalize(const char *path, char *out_path, size_t out_size);

/*____________________________________________________________________________*/

/**
 * @brief Debug function, Get stats of FS:
 *
 * Example:
 * 	/SD:/
 * 	/RAM:/
 *
 * @param abs_path
 * @return int
 */
int app_fs_stats(const char *abs_path);

/**
 * @brief Debug function, listing all files and directories in a given path
 *
 * @param path
 * @return int
 */
int app_fs_lsdir(const char *path);

/**
 * @brief Create a directory and all its parents if they don't exist
 *
 * @param path
 * @param is_filepath Tells whether the path contains the filename or not
 * @return int Negative value on error, 0 on success
 */
int app_fs_mkdir_intermediate(const char *path, bool is_filepath);

#endif /* _APP_FS_H_ */