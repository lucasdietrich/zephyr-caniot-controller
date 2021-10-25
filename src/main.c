#include <zephyr.h>
#include <devicetree.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include <storage/flash_map.h>
#include <string.h>
#define STORAGE_FLASH_ID                FLASH_AREA_ID(storage)

#include <drivers/flash.h>
#include <device.h>
#define STORAGE_NODE                    DT_NODE_BY_FIXED_PARTITION_LABEL(storage)
#define FLASH_NODE                      DT_MTD_FROM_FIXED_PARTITION(STORAGE_NODE)

const struct flash_area *fa;

static char buffer[0x400];
static char mystring[] = "It's me, Lucas !!!";

void main(void)
{
        int ret;

        /* drivers API */
        const struct device *flash_dev = DEVICE_DT_GET(FLASH_NODE);

        size_t pages = flash_get_page_count(flash_dev);
        LOG_INF("flash_get_page_count(0x%x) = %d sectors (pages)",
                (uint32_t)flash_dev, pages);

        struct flash_pages_info page_info;
        for (uint_fast8_t i = 0; i < pages; i++) {
                ret = flash_get_page_info_by_idx(flash_dev, i, &page_info);
                LOG_DBG("flash_get_page_info_by_idx(%x, %d, *) = %d",
                        (uint32_t)flash_dev, i, ret);
                LOG_INF("index=%d size=0x%x start_offset=0x%x",
                        page_info.index, page_info.size, page_info.start_offset);
        }
        
        /* application API */
        ret = flash_area_open(STORAGE_FLASH_ID, &fa);
        LOG_INF("flash_area_open(0x%x, *) = %d", STORAGE_FLASH_ID, ret);

        if (ret == 0) {
                ret = flash_area_read(fa, 0, buffer, sizeof(buffer));
                LOG_DBG("flash_area_read() = %d", ret);

                LOG_HEXDUMP_INF(buffer, (uint32_t) sizeof(buffer), "content");

                if (strncmp(buffer, mystring, sizeof(mystring)) != 0) {
                        LOG_INF("String not found in flash, writing ... (%d)", 0);

                        ret = flash_area_erase(fa, 0, fa->fa_size);
                        LOG_DBG("flash_area_erase(*, *, 0x%x) = %d", fa->fa_size, ret);

                        if (ret == 0) {
                                ret = flash_area_write(fa, 0x00,
                                        mystring, sizeof(mystring));
                                LOG_DBG("flash_area_write() = %d", ret);

                                ret = flash_area_read(fa, 0, buffer, sizeof(buffer));
                                LOG_DBG("flash_area_read() = %d", ret);

                                LOG_HEXDUMP_INF(buffer, (uint32_t) sizeof(buffer), "content");
                        }
                }

                flash_area_close(fa);
        }

        k_sleep(K_FOREVER);
}
