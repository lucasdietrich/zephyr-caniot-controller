#include "stm32_crc32.h"

#include <zephyr.h>
#include <device.h>

#include <stm32f429xx.h>
#include <stm32f4xx_ll_crc.h>

#include "stm32_crc32.h"

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(crc1), st_stm32_crc, okay)

BUILD_ASSERT(DT_REG_ADDR(DT_NODELABEL(crc1)) == CRC_BASE,
	     "CRC base address mismatch");
	     
#define DEV_DATA(dev) ((struct crc_stm32_data *const)(dev)->data)
#define DEV_CFG(dev) \
	((const struct crc_stm32_config *const)(dev)->config)

static int crt_stm32_init(const struct device *dev)
{
	/* Enable CRC peripheral clock */
	__HAL_RCC_CRC_CLK_ENABLE();

	return 0;
}

static uint32_t ctc_stm32_calculate(const struct device *dev,
				    uint32_t *buf,
				    size_t len)
{
	const struct crc_stm32_config *cfg = DEV_CFG(dev);
	struct crc_stm32_data *data = DEV_DATA(dev);
	CRC_TypeDef *crc = cfg->crc;

	k_mutex_lock(&data->lock, K_FOREVER);

	LL_CRC_ResetCRCCalculationUnit(crc);

	for (uint32_t *p = buf; p < buf + len; p++) {
		LL_CRC_FeedData32(crc, *p);
	}

	const uint32_t result = LL_CRC_ReadData32(crc);

	k_mutex_unlock(&data->lock);

	return result;
}

static const struct crc_drivers_api api = {
	.calculate = ctc_stm32_calculate,
};

static const struct crc_stm32_config crc1_stm32_cfg = {
	.crc = (CRC_TypeDef *)DT_REG_ADDR(DT_NODELABEL(crc1)),
};

static struct crc_stm32_data crc1_stm32_data = {
	.lock = Z_MUTEX_INITIALIZER(crc1_stm32_data.lock),
};

DEVICE_DT_DEFINE(DT_NODELABEL(crc1), &crt_stm32_init,
		 NULL, &crc1_stm32_data, &crc1_stm32_cfg, POST_KERNEL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api);

#endif 