/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CREDS_MANAGER_H_
#define _CREDS_MANAGER_H_

#include <zephyr/kernel.h>

#include "utils/buffers.h"
#include "credentials.h"

int creds_manager_init(void);

int cred_get(cred_id_t id, struct cred *c);

int cred_copy_to(cred_id_t id, char *buf, size_t *size);


#endif /* _CREDS_MANAGER_H_ */