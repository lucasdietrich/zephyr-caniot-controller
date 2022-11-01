/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CREDS_HARDCODED_CREDS_H_
#define _CREDS_HARDCODED_CREDS_H_

#include <zephyr/kernel.h>

#include "credentials.h"

#define CREDS_HARDCODED_MAX_COUNT 12u

struct hardcoded_cred {
	cred_id_t id;
	const char *data;
	size_t len;
};

extern const struct hardcoded_cred creds_harcoded_array[CREDS_HARDCODED_MAX_COUNT];

int harcoded_creds_init(void);

int harcoded_cred_get(cred_id_t id, struct cred *c);

int harcoded_cred_copy_to(cred_id_t id, char *buf, size_t *size);

#endif /* _CREDS_HARDCODED_CREDS_H_ */