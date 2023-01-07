#include "crypto.h"

#include <mbedtls/sha1.h>

#if defined(CONFIG_MBEDTLS)

int crypt_sha1(const unsigned char *input, size_t ilen, unsigned char output[20])
{
	return mbedtls_sha1(input, ilen, output);
}

/* TODO use hardware on stm32f439 to calculate hash */

#endif /* CONFIG_MBEDTLS */