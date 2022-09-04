/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aws.h"

#include "utils/misc.h"
#include "creds/manager.h"

#include <net/tls_credentials.h>

#define TLS_TAG_DEVICE_CERT 2
#define TLS_TAG_DEVICE_KEY 2
#define TLS_TAG_CA_CERT 2

struct aws_data
{
	sec_tag_t sec_tls_tags[1u];

	struct mqtt_client client_ctx;
};

struct aws_config
{
	const char *endpoint;
	uint16_t port;

	const char *thing_name;
};

static struct aws_data data = {
	.sec_tls_tags = { TLS_TAG_DEVICE_CERT },
};

static struct aws_config config;

static int aws_init(void)
{
	int ret;
	struct cred *cert, *key, *ca;

	/* Set configuration */
	config.endpoint = CONFIG_AWS_ENDPOINT;
	config.port = CLOUD_MQTT_PORT;
	config.thing_name = CONFIG_AWS_THING_NAME;

	/* Setup TLS credentials */
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_CERTIFICATE_DER, &cert)) == 0);
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_PRIVATE_KEY_DER, &key)) == 0);
	CHECK_OR_EXIT((ret = cred_get(CRED_AWS_ROOT_CA1_DER, &ca)) == 0);

	tls_credential_add(TLS_TAG_DEVICE_CERT,
			   TLS_CREDENTIAL_SERVER_CERTIFICATE,
			   cert->data, cert->len);

	tls_credential_add(TLS_TAG_DEVICE_KEY,
			   TLS_CREDENTIAL_PRIVATE_KEY,
			   key->data, key->len);

	tls_credential_add(TLS_TAG_CA_CERT,
			   TLS_CREDENTIAL_CA_CERTIFICATE,
			   ca->data, ca->len);

	memset(&data, 0, sizeof(data));

	ret = 0;

exit:
	return ret;
}

static int aws_connect(void)
{
	return -ENOTSUP;
}

static int aws_publish(const char *topic, const char *data, size_t len, int qos)
{
	return -ENOTSUP;
}

static int aws_subscribe(const char *topic, int qos)
{
	return -ENOTSUP;
}

static int aws_on_published(const char *topic, const char *data, size_t len)
{
	return -ENOTSUP;
}


struct cloud_platform aws_platform =
{
	.name = "AWS",

	.config = &config,
	.data = &data,

	.init = aws_init,
	.connect = aws_connect,
	.publish = aws_publish,
	.subscribe = aws_subscribe,
	.on_published = aws_on_published,
};