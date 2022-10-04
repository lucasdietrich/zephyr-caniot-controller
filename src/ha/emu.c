/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "emu.h"

#include <zephyr.h>
#include <random/rand32.h>

#include "net_time.h"
#include "system.h"
#include "ble/xiaomi_record.h"

#include "ha/devices.h"
#include "ha/room.h"
#include "ha/devices/xiaomi.h"

#include <caniot/device.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ha_emu, LOG_LEVEL_INF);

#define EMU_BLE_RDM_MIN_MS 200
#define EMU_BLE_RDM_MAX_MS 2000

#define PRODUCER_THREADS_START_DELAY_MS 2000u
#define CONSUMER_THREADS_START_DELAY_MS 3000u
#define COMMAND_THREADS_START_DELAY_MS 3000u
#define CANIOT_THREADS_START_DELAY_MS 3000u

static uint32_t get_rdm_delay_ms(uint32_t min, uint32_t max)
{
	return sys_rand32_get() % (max - min) + min;
}

static uint32_t get_rdm_delay_ms_1(void)
{
	return get_rdm_delay_ms(EMU_BLE_RDM_MIN_MS, EMU_BLE_RDM_MAX_MS);
}

void emu_ble_device(void *_a, void *_b, void *_c);
void emu_consumer(void *_a, void *_b, void *_c);
void emu_cmd_thread(void *_a, void *_b, void *_c);
void emu_caniot_device(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(emu_ble_device1, 1024u, emu_ble_device, 1, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, PRODUCER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_ble_device2, 1024u, emu_ble_device, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, PRODUCER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_ble_device3, 1024u, emu_ble_device, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, PRODUCER_THREADS_START_DELAY_MS);

K_THREAD_DEFINE(emu_consumer1, 1024u, emu_consumer, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_consumer2, 1024u, emu_consumer, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_consumer3, 1024u, emu_consumer, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_consumer4, 1024u, emu_consumer, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);

K_THREAD_DEFINE(emu_cmd_thread1, 1024u, emu_cmd_thread, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, COMMAND_THREADS_START_DELAY_MS);

K_THREAD_DEFINE(emu_caniot_device1, 1024u, emu_caniot_device, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, CANIOT_THREADS_START_DELAY_MS);

#define EMU_BLE_ADDR_INIT(_type, _last) \
	{ _type, { { 0xFF, 0xFF, 0xFF, 0x00, 0x00, _last } } }

static const bt_addr_le_t addrs[] = {
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 1u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 2u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 3u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 4u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 5u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 6u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 7u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 8u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 9u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 10u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 11u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 12u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 13u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 14u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 15u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 16u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 17u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 18u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 19u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 20u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 21u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 22u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 23u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 24u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 25u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 26u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 27u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 28u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 29u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 30u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 31u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 32u),
};

void emu_ble_device(void *_a, void *_b, void *_c)
{
	int ret;
	xiaomi_record_t record;

	for (uint32_t i = 0u;;i++) {
		/* Choose a fake address */
		bt_addr_le_copy(&record.addr, &addrs[i % ARRAY_SIZE(addrs)]);

		/* Populate with fake data */
		record.time = sys_time_get();
		record.measurements.battery_level = i % 100;
		record.measurements.battery_mv = (i % 100) * 3.3;
		record.measurements.humidity = i % 1000;
		record.measurements.temperature = i % 1000;
		record.measurements.rssi = i % 100;

		/* Get the device */

		const ha_dev_addr_t addr = {
			.type = HA_DEV_TYPE_XIAOMI_MIJIA,
			.mac = {
				.medium = HA_DEV_MEDIUM_BLE,
				.addr.ble = record.addr,
			}
		};

		ret = ha_dev_register_data(&addr, &record, sizeof(xiaomi_record_t), 0, NULL);
		

		/* Register the data to the device */

		/* Next emu measurement in */
		const uint32_t next = get_rdm_delay_ms(EMU_BLE_RDM_MIN_MS,
						 EMU_BLE_RDM_MAX_MS);
		k_sleep(K_MSEC(next));

		if (_a != NULL) {
			LOG_DBG("k_mem_slab_num_free_get(): %u",
				ha_ev_free_count());
		}
	}
}

void emu_consumer(void *_a, void *_b, void *_c)
{

	ha_ev_subs_t *trig;
	ha_ev_t *event;
	const struct ha_ev_subs_conf sub = {
		.flags = HA_EV_SUBS_DEVICE_TYPE,
		.device_type = HA_DEV_TYPE_XIAOMI_MIJIA
	};
	int ret = ha_ev_subscribe(&sub, &trig);
	if (ret != 0) {
		LOG_ERR("(thread %p) Failed to subscribe to events, ret=%d", 
			_current, ret);
		return;
	}

	for (uint32_t i = 0u;;i++) {
		event = ha_ev_wait(trig, K_MSEC(get_rdm_delay_ms_1()));

		if (event != NULL) {
			LOG_DBG("(thread %p) got event %p (refc = %u) - time=%u temp=%d",
				_current, event, (uint32_t)atomic_get(&event->ref_count),
				event->timestamp,
				((struct ha_ds_xiaomi *)event->data)->temperature.value);

			if (event->dev && event->dev->room) {
				LOG_DBG("(%p %p %p) Room: %s", event, event->dev, 
					event->dev->room, log_strdup(event->dev->room->name));
			}

			ha_ev_unref(event);
		}
		
		k_sleep(K_MSEC(get_rdm_delay_ms_1() >> 2));
	}
}

#include "caniot_controller.h"

void emu_cmd_thread(void *_a, void *_b, void *_c)
{
	// const ha_dev_addr_t target = {
	// 	.type = HA_DEV_TYPE_CANIOT,
	// 	.mac = {
	// 		.medium = HA_DEV_MEDIUM_CAN,
	// 		.addr.can = {
	// 			.bus = 0,
	// 			.ext = 0,
	// 			.id = 24
	// 		}
	// 	}
	// };
	
	// ha_dev_cmd_t cmd =
	// {
	// 	.type = 0u,
	// 	.data = 1u
	// };

	for (;;) {
		// ha_ev_t *resp = ha_dev_command(&target, &cmd, K_SECONDS(1));

		struct caniot_frame req;
		struct caniot_frame resp;
		uint32_t timeout = 1000u;

		caniot_build_query_telemetry(&req, CANIOT_ENDPOINT_BOARD_CONTROL);

		int ret = ha_ciot_ctrl_query(&req, &resp, CANIOT_DID(7, 0), &timeout);

		LOG_DBG("ret = %d", ret);

		k_sleep(K_SECONDS(1));
	}
}

static uint64_t caniot_emu_state = 0ull;

int caniot_emu_telem(struct caniot_device *dev,
		     caniot_endpoint_t ep,
		     char *buf,
		     uint8_t *len)
{
	memcpy(buf, &caniot_emu_state, 8u);
	*len = 8u;

	return 0;
}

int caniot_emu_cmd(struct caniot_device *dev,
		   caniot_endpoint_t ep,
		   const char *buf,
		   uint8_t len)
{
	caniot_emu_state += *(uint64_t *)buf;

	return 0;
}

K_MSGQ_DEFINE(emu_caniot_txq, sizeof(struct caniot_frame), 1u, 4u);
K_MSGQ_DEFINE(emu_caniot_rxq, sizeof(struct caniot_frame), 1u, 4u);

void emu_caniot_device(void *_a, void *_b, void *_c)
{
	int ret;

	const struct caniot_identification identification = {
		.did = CANIOT_DID(0u, 7u),
		.magic_number = CNAIOT_MAGIC_NUMBER_EMU,
		.name = "Caniot EMU 1",
		.version = CANIOT_VERSION
	};
	
	struct caniot_config config = CANIOT_CONFIG_DEFAULT_INIT();

	const struct caniot_api api = {
		.config.on_read = NULL,
		.config.on_write = NULL,
		.custom_attr.read = NULL,
		.custom_attr.write = NULL,
		.telemetry_handler = caniot_emu_telem,
		.command_handler = caniot_emu_cmd,
	};

	struct caniot_device dev = {
		.identification = &identification,
		.api = &api,
		.config = &config,
		.flags.request_telemetry = 0u,
	};

	caniot_device_system_reset(&dev);

	struct caniot_frame req;
	struct caniot_frame resp;

	for (;;) {
		ret = k_msgq_get(&emu_caniot_txq, &req, K_FOREVER);
		if (ret) goto error;

		ret = caniot_device_handle_rx_frame(&dev, &req, &resp);
		if (ret) goto error;

		ret = k_msgq_put(&emu_caniot_rxq, &resp, K_FOREVER);
		if (ret) goto error;
	}

error:
	LOG_ERR("emu_caniot_device thread terminated with err=%d", ret);
}

int emu_caniot_send(struct caniot_frame *f)
{
	return k_msgq_put(&emu_caniot_txq, f, K_NO_WAIT);
}