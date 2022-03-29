/**
 * @file stm32_crc32.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Drivers for CRC32 calculation in Zephyr RTOS
 * @version 0.1
 * @date 2022-03-20
 * 
 * @copyright Copyright (c) 2022
 * 
 * This is an attempt to implement CRC32 calculation drivers in Zephyr RTOS.
 * 
 * Use function crc_calculate() to calculate CRC32 using the SoC driver.
 * 
 * Tested using stm32f429zi :
 * 
 * nucleo_f429zi.dts :
 *	soc {
 *		crc1: crc@40023000 {
 *			compatible = "st,stm32-crc";
 *			reg = <0x40023000 0xC>;
 *			status = "okay";
 *		};
 *	};
 *
 * Get device using 
 */

#ifndef _UTILS_STM32_CRC32_H_
#define _UTILS_STM32_CRC32_H_

#include <zephyr.h>
#include <device.h>

typedef uint32_t(*crc_calculate_t)(const struct device *dev,
				   uint32_t *buf,
				   size_t len);

struct crc_drivers_api {
	crc_calculate_t calculate;
};

struct crc_stm32_config {
	CRC_TypeDef *crc;   /*!< CRC Registers*/
};

struct crc_stm32_data {
	struct k_mutex lock;
};

static inline uint32_t crc_calculate(const struct device *dev,
				     uint32_t *buf,
				     size_t len)
{
	const struct crc_drivers_api *api;

	api = (const struct crc_drivers_api *)dev->api;

	return api->calculate(dev, buf, len);
}

#endif /* _UTILS_STM32_CRC32_H_ */