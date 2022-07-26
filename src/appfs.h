/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _APP_FS_H_
#define _APP_FS_H_

#include <stddef.h>

int app_fs_init(void);

int app_fs_stats(const char *abs_path);

int app_fs_lsdir(const char *path);

int app_fs_file_add(const char *fpath, const char *data, size_t size);

#endif /* _APP_FS_H_ */