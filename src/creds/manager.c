/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "manager.h"

extern const char x509_public_certificate_rsa1024_der[];
extern size_t x509_public_certificate_rsa1024_der_size;
extern const char rsa_private_key_rsa1024_der[];
extern size_t rsa_private_key_rsa1024_der_size;

extern const char x509_public_certificate_rsa2048_der[];
extern size_t x509_public_certificate_rsa2048_der_size;
extern const char rsa_private_key_rsa2048_der[];
extern size_t rsa_private_key_rsa2048_der_size;

int creds_manager_get(cred_t type, cred_buf_t *cred)
{
	int ret = -EINVAL;

	if (cred == NULL) {
		goto exit;
	}

	switch (type) {
	case CRED_HTTPS_SERVER_PRIVATE_KEY:
		cred->data = rsa_private_key_rsa1024_der;
		cred->len = rsa_private_key_rsa1024_der_size;
		break;
	case CRED_HTTPS_SERVER_CERTIFICATE:
		cred->data = x509_public_certificate_rsa1024_der;
		cred->len = x509_public_certificate_rsa1024_der_size;
		break;
	default:
		ret = -1;
		break;
	}

	ret = 0;

exit:
	return ret;
}