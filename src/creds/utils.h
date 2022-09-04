/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CREDS_UTILS_H
#define _CREDS_UTILS_H

#include <zephyr.h>

#include "credentials.h"
#include "flash_creds.h"

const char *flash_cred_status_to_str(flash_cred_status_t status);

const char *cred_id_to_str(cred_id_t id);

const char *cred_format_to_str(cred_format_t format);

#endif /* _CREDS_UTILS_H */