#include <zephyr.h>

#include "net_interface.h"
#include "user_io.h"
#include "net_time.h"
#include "crypto.h"

#include <mbedtls/memory_buffer_alloc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static void debug_mbedtls_memory(void)
{
        size_t cur_used, cur_blocks, max_used, max_blocks;
        mbedtls_memory_buffer_alloc_cur_get(&cur_used, &cur_blocks);
        mbedtls_memory_buffer_alloc_max_get(&max_used, &max_blocks);

        LOG_DBG("MAX %u (%u) CUR %u (%u)", max_used,
                max_blocks, cur_used, cur_blocks);
}

void main(void)
{
        crypto_mbedtls_heap_init();
        net_interface_init();
        user_io_init();

        static int counter = 0;
        
        for (;;) {
                if (counter++ % 60 == 0) {
                        net_time_show();

                        debug_mbedtls_memory();
                }

                k_msleep(1000);
        }
}
