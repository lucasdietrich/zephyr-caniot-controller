/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto.h"
#include "lua/orchestrator.h"
#include "lua/utils.h"
#include "net_interface.h"
#include "net_time.h"
#include "usb/usb.h"
#include "utils/freelist.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>

#if defined(CONFIG_APP_DFU)
#include "dfu/dfu.h"
#endif

#include "creds/manager.h"
#include "fs/app_utils.h"
#include "ha/devices/f429zi.h"
#include "userio/button.h"
#include "userio/leds.h"

#if defined(CONFIG_APP_CAN_INTERFACE)
#include "can/can_interface.h"
#endif /* CONFIG_APP_CAN_INTERFACE */

#if defined(CONFIG_APP_BLE_INTERFACE)
#include "ble/ble.h"
#endif /* CONFIG_APP_BLE_INTERFACE */

#ifndef CONFIG_QEMU_TARGET
#include "ha/core/ha.h"
#endif /* CONFIG_QEMU_TARGET */

#ifdef CONFIG_LUA
#include "lua/utils.h"
#endif

#include <stdio.h>

#include <zephyr/logging/log.h>

#include <mbedtls/memory_buffer_alloc.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_temp)
#define TEMP_NODE DT_INST(0, st_stm32_temp)
#else
// #error "Could not find a compatible temperature sensor"
#endif

#ifdef TEMP_NODE
static const struct device *die_temp_dev = DEVICE_DT_GET(TEMP_NODE);

static int die_temp_dev_init(void)
{
	if (!device_is_ready(die_temp_dev)) {
		LOG_ERR("(%p) Temperature sensor is not ready", die_temp_dev);
		return -EIO;
	}

	return 0U;
}

static int die_temp_fetch(void)
{
	int rc;
	struct sensor_value val;

	rc = sensor_sample_fetch(die_temp_dev);
	if (rc) {
		LOG_ERR("Failed to fetch sample (%d)", rc);
		goto exit;
	}

	rc = sensor_channel_get(die_temp_dev, SENSOR_CHAN_DIE_TEMP, &val);
	if (rc) {
		LOG_ERR("Failed to get data (%d)", rc);
		goto exit;
	}

	const float temperature = (float)sensor_value_to_double(&val);
	if (temperature > (float)-276.0) {
#if defined(CONFIG_APP_HA)
		ha_dev_register_die_temperature(net_time_get(), temperature);
#endif

		LOG_DBG("Current DIE temperature: %.1f °C ", (double)temperature);
	} else {
		LOG_WRN("Invalid DIE temperature: %.1f °C", (double)temperature);
	}

exit:
	return rc;
}

#endif /* TEMP_NODE */

static void debug_mbedtls_memory(void)
{
	size_t cur_used, cur_blocks, max_used, max_blocks;
	mbedtls_memory_buffer_alloc_cur_get(&cur_used, &cur_blocks);
	mbedtls_memory_buffer_alloc_max_get(&max_used, &max_blocks);

	LOG_DBG("MAX %u (%u) CUR %u (%u)", max_used, max_blocks, cur_used, cur_blocks);
}

extern int lua_fs_populate(void);

int main(void)
{
	printk("Starting Zephyr application...\n");

#if defined(CONFIG_APP_DFU)
	dfu_image_check();
#endif

#ifndef CONFIG_QEMU_TARGET
	leds_init();
	button_init();
#endif
	app_fs_init();

#if defined(CONFIG_APP_CREDENTIALS_MANAGER)
	creds_manager_init();
#endif

	crypto_mbedtls_heap_init();
	net_interface_init();

#if defined(CONFIG_USB_DEVICE_STACK)
	usb_init();
#endif

#if defined(CONFIG_APP_CAN_INTERFACE)
	if_can_init();
#endif /* CONFIG_APP_CAN_INTERFACE */

#if defined(CONFIG_APP_LUA_FS_DEFAULT_SCRIPTS)
	lua_fs_populate();
#endif

#if defined(CONFIUG_LUA)
	lua_orch_init();
#endif

#ifdef TEMP_NODE
	die_temp_dev_init();
#endif /* TEMP_NODE */

#if defined(CONFIG_APP_BLE_INTERFACE)
	ble_init();
#endif

#if defined(CONFIG_APP_LUA_AUTORUN_SCRIPTS)
	lua_utils_execute_fs_script2("/RAM:/lua/entry.lua");
#endif

	uint32_t counter = 0;

	for (;;) {

		/* 1 second tasks */

#if defined(CONFIG_APP_PRINTF_1SEC_COUNTER)
		/* If you are running your code in QEMU enable this printf
		 * so that you can adjust CONFIG_QEMU_ICOUNT_SHIFT value
		 */
		printf("Counter: %u\n", counter);
#endif /* CONFIG_APP_PRINTF_1SEC_COUNTER */

		/* 10 second tasks */
		if (counter % 10 == 0) {
#ifdef TEMP_NODE
			die_temp_fetch();
#endif /* TEMP_NODE */
		}

		/* 1min tasks */
		if (counter % 60 == 0) {
		}

		/* 10min tasks but 5 seconds after startup*/
		if (counter % 600 == 5) {
			net_time_show();
			debug_mbedtls_memory();

#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS)
			extern struct k_heap _system_heap;
			struct sys_memory_stats stats;
			sys_heap_runtime_stats_get(&_system_heap.heap, &stats);
			LOG_DBG("sys heap stats: alloc=%u free=%u max=%u", stats.allocated_bytes,
					stats.free_bytes, stats.max_allocated_bytes);
#endif
		}

		counter++;
		k_msleep(1000);
	}

	return 0;
}
