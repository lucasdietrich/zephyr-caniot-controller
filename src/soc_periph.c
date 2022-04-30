#include <device.h>
#include <devicetree.h>

#include "drivers/crc/stm32_crc32.h"

#define CRC_NODE DT_NODELABEL(crc1)

static const struct device *crc_dev = DEVICE_DT_GET(CRC_NODE);

uint32_t crc_calculate32(uint32_t *buf, size_t len)
{
	return crc_calculate(crc_dev, buf, len);
}