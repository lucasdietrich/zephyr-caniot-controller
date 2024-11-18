/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* https://tls.mbed.org/kb/how-to/reduce-mbedtls-memory-and-storage-footprint */

#define MBEDTLS_AES_ROM_TABLES

// TODO Necessary ?
// #define MBEDTLS_HAVE_TIME
// #define MBEDTLS_HAVE_TIME_DATE
// #define MBEDTLS_PLATFORM_TIME_ALT

// tls 1.3 ?
#undef MBEDTLS_SSL_KEEP_PEER_CERTIFICATE