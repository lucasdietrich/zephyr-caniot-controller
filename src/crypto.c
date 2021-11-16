#include "crypto.h"

#include <linker/sections.h>
#include <mbedtls/memory_buffer_alloc.h>

static unsigned char __ccm_noinit_section mbedtls_heap[0x10000];

void crypto_mbedtls_heap_init(void)
{
        mbedtls_memory_buffer_alloc_init(mbedtls_heap, sizeof(mbedtls_heap));
}