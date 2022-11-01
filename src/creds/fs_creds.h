/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CREDS_FS_CREDS_H_
#define _CREDS_FS_CREDS_H_

#include <zephyr/kernel.h>

#include "credentials.h"

int fs_creds_init(void);

#endif /* _CREDS_FS_CREDS_H_ */