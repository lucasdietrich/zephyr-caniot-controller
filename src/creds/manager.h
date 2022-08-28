/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CREDS_MANAGER_H_
#define _CREDS_MANAGER_H_

#include <zephyr.h>

#include "utils/buffers.h"

typedef struct cred_buf {
	const char *data;
	size_t len;
} cred_buf_t;

typedef enum {
	/* HTTPS server */
	CRED_HTTPS_SERVER_PRIVATE_KEY,
	CRED_HTTPS_SERVER_CERTIFICATE,

	/* For future features, when verifying client certificates */
	CRED_HTTPS_CLIENT_PRIVATE_CERTIFICATE,
	CRED_HTTPS_CLIENT_CA,

	/* Cloud AWS */
	CRED_AWS_PRIVATE_KEY,
	CRED_AWS_CERTIFICATE,
	CRED_AWS_ROOT_CA,
} cred_t;

int creds_manager_init(void);

int creds_manager_get(cred_t type, cred_buf_t *cred);



#endif /* _CREDS_MANAGER_H_ */