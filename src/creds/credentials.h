/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CREDS_CREDENTIALS_H_
#define _CREDS_CREDENTIALS_H_

#include <zephyr.h>

#include <stdint.h>

#include <utils/buffers.h>

#define CRED_HTTPS_SERVER_BASE 0x10u
#define CRED_AWS_BASE 0x20u

typedef enum {
	/* HTTPS server */
	CRED_HTTPS_SERVER_PRIVATE_KEY = CRED_HTTPS_SERVER_BASE,
	CRED_HTTPS_SERVER_CERTIFICATE,

	CRED_HTTPS_SERVER_PRIVATE_KEY_DER,
	CRED_HTTPS_SERVER_CERTIFICATE_DER,

	CRED_HTTPS_SERVER_CLIENT_CA,
	CRED_HTTPS_SERVER_CLIENT_CA_DER,

	/* Cloud AWS */
	CRED_AWS_PRIVATE_KEY = CRED_AWS_BASE,
	CRED_AWS_CERTIFICATE,

	CRED_AWS_PRIVATE_KEY_DER,
	CRED_AWS_CERTIFICATE_DER,

	CRED_AWS_ROOT_CA1,
	CRED_AWS_ROOT_CA3,

	CRED_AWS_ROOT_CA1_DER,
	CRED_AWS_ROOT_CA3_DER,
} cred_id_t;

typedef enum {
	CRED_FORMAT_UNKOWN,
	CRED_FORMAT_PEM,
	CRED_FORMAT_DER,
} cred_format_t;

typedef struct cred
{
	const char *data;
	size_t len;
} cred_t;

#endif /* _CREDS_CREDENTIALS_H_ */