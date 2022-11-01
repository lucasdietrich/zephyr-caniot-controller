/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "creds/manager.h"
#include "net/tls_credentials.h"
#include "net/mqtt.h"
#include "net/socket.h"

#include "utils/misc.h"
#include "utils/buffers.h"
#include "app_sections.h"

#include "utils.h"
#include "cloud_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aws, LOG_LEVEL_DBG);

#define AWS_CERT_DER 1

#define AWS_ENDPOINT_PORT 8883u

#define TLS_TAG_DEVICE_CERT 2u
#define TLS_TAG_DEVICE_KEY 2u
#define TLS_TAG_CA_CERT 2u

static sec_tag_t sec_tls_tags[] = {
	TLS_TAG_DEVICE_CERT,
};

static int setup_credentials(void)
{
	int ret;
	struct cred cert, key, ca;

	/* Setup TLS credentials */
#if AWS_CERT_DER
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_CERTIFICATE_DER, &cert)) == 0);
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_PRIVATE_KEY_DER, &key)) == 0);
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_ROOT_CA1_DER, &ca)) == 0);
#else
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_CERTIFICATE, &cert)) == 0);
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_PRIVATE_KEY, &key)) == 0);
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_ROOT_CA1, &ca)) == 0);
#endif 

	ret = tls_credential_add(TLS_TAG_DEVICE_CERT,
				 TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 cert.data, cert.len);
	CHECK_OR_EXIT(ret == 0);

	ret = tls_credential_add(TLS_TAG_DEVICE_KEY,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 key.data, key.len);
	CHECK_OR_EXIT(ret == 0);

	ret = tls_credential_add(TLS_TAG_CA_CERT,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca.data, ca.len);
	CHECK_OR_EXIT(ret == 0);

exit:
	if (ret != 0) {
		LOG_ERR("Failed to setup credentials ret=%d", ret);
	}
	return ret;
}

static int clear_credentials(void)
{
	int ret;

	ret = tls_credential_delete(TLS_TAG_DEVICE_CERT,
				    TLS_CREDENTIAL_SERVER_CERTIFICATE);
	CHECK_OR_EXIT(ret == 0);

	ret = tls_credential_delete(TLS_TAG_DEVICE_KEY,
				    TLS_CREDENTIAL_PRIVATE_KEY);
	CHECK_OR_EXIT(ret == 0);

	ret = tls_credential_delete(TLS_TAG_CA_CERT,
				    TLS_CREDENTIAL_CA_CERTIFICATE);
	CHECK_OR_EXIT(ret == 0);

exit:
	if (ret != 0) {
		LOG_ERR("Failed to clear credentials ret=%d", ret);
	}
	return ret;
}

static int aws_init(struct cloud_platform_config *c)
{
	/* Setup TLS credentials */
	return setup_credentials();
}

static int aws_deinit(struct cloud_platform_config *p)
{
	return clear_credentials();
}

struct cloud_platform aws_platform =
{
	.name = "AWS",

	.config = {
		.clientid = CONFIG_AWS_THING_NAME,
		.endpoint = CONFIG_AWS_ENDPOINT,
		.port = AWS_ENDPOINT_PORT,
		.sec_tag_list = sec_tls_tags,
		.sec_tag_count = ARRAY_SIZE(sec_tls_tags),
	},

	.init = aws_init,
	.provision = NULL,
	.deinit = aws_deinit,
};