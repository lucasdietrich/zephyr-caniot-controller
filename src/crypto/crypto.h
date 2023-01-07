/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#include <stddef.h>
#include <stdint.h>

int crypt_sha1(const unsigned char *input, size_t ilen, unsigned char output[20]);

#endif /* _CRYPTO_H_ */