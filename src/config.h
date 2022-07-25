#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#include <zephyr.h>

/* Application specific checks */

/* Because IPC debug pins used on STM32 are GPIOC 8, 9, 10, 11, 
 * 12 which are the one used by the MMC
 */
#if defined(CONFIG_DISK_DRIVER_SDMMC) && \
	DT_HAS_COMPAT_STATUS_OKAY(zephyr_mmc_spi_slot) && \
	defined(CONFIG_UART_IPC_DEBUG_GPIO_STM32)
#	error "CONFIG_DISK_DRIVER_SDMMC and CONFIG_UART_IPC_DEBUG_GPIO_STM32 are mutually exclusive"
#endif 

#endif /* _APP_CONFIG_H_ */