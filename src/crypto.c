#include "crypto.h"

#include <linker/sections.h>
#include <mbedtls/memory_buffer_alloc.h>

/* __ccm_noinit_section config in CONFIG_ARM context */
#if !defined(__ccm_noinit_section)
#	define __ccm_noinit_section __attribute__((section(".noinit")))
#endif

#ifdef CONFIG_MBEDTLS_CUSTOM_HEAP_CCM
#	define mbedtls_heap_section __ccm_noinit_section
#else
#	define mbedtls_heap_section
#endif /* CONFIG_MBEDTLS_CUSTOM_HEAP_CCM */

static unsigned char mbedtls_heap_section mbedtls_heap[CONFIG_MBEDTLS_CUSTOM_HEAP_SIZE];

void crypto_mbedtls_heap_init(void)
{
        mbedtls_memory_buffer_alloc_init(mbedtls_heap, sizeof(mbedtls_heap));
}