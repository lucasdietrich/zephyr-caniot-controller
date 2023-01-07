/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto.h"

#include <zephyr/linker/sections.h>

#include <mbedtls/memory_buffer_alloc.h>

/* __ccm_noinit_section config in CONFIG_ARM context only */
#if !defined(__ccm_noinit_section)
#define __ccm_noinit_section __noinit
#endif

#ifdef CONFIG_APP_MBEDTLS_CUSTOM_HEAP_CCM
#define mbedtls_heap_section __ccm_noinit_section
#else
#define mbedtls_heap_section __noinit
#endif /* CONFIG_APP_MBEDTLS_CUSTOM_HEAP_CCM */

static unsigned char mbedtls_heap_section
	mbedtls_heap[CONFIG_APP_MBEDTLS_CUSTOM_HEAP_SIZE];

void crypto_mbedtls_heap_init(void)
{
	mbedtls_memory_buffer_alloc_init(mbedtls_heap, sizeof(mbedtls_heap));
}