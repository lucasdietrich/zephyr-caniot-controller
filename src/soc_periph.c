#include <device.h>
#include <devicetree.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_crc)

#include "drivers/crc/stm32_crc32.h"

#define CRC_NODE DT_NODELABEL(crc1)

static const struct device *crc_dev = DEVICE_DT_GET(CRC_NODE);

uint32_t crc_calculate32(uint32_t *buf, size_t len)
{
	return crc_calculate(crc_dev, buf, len);
}

#endif