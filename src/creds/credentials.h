#ifndef _CREDS_CREDENTIALS_H_
#define _CREDS_CREDENTIALS_H_

#include <zephyr.h>

#include <stdint.h>

#include <utils/buffers.h>

typedef enum {
	/* HTTPS server */
	CRED_HTTPS_SERVER_PRIVATE_KEY = 0,
	CRED_HTTPS_SERVER_CERTIFICATE = 1,

	/* For future features, when verifying client certificates */
	// CRED_HTTPS_CLIENT_PRIVATE_CERTIFICATE = 2,
	CRED_HTTPS_CLIENT_CA = 3,

	/* Cloud AWS */
	CRED_AWS_PRIVATE_KEY = 4,
	CRED_AWS_CERTIFICATE = 5,
	
	CRED_AWS_PRIVATE_KEY_DER = 6,
	CRED_AWS_CERTIFICATE_DER = 7,

	CRED_AWS_ROOT_CA1 = 8,
	CRED_AWS_ROOT_CA3 = 9,

	CRED_AWS_ROOT_CA1_DER = 10,
	CRED_AWS_ROOT_CA3_DER = 11,
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