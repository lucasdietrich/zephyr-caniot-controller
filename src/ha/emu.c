/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "emu.h"

#include <zephyr/kernel.h>
#include <zephyr/random/rand32.h>

#include "net_time.h"
#include "system.h"
#include "ble/xiaomi_record.h"

#include "caniot_controller.h"

#include "ha/devices.h"
#include "ha/room.h"
#include "ha/devices/xiaomi.h"

#include <caniot/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ha_emu, LOG_LEVEL_INF);

#define EMU_BLE_RDM_MIN_MS 		100
#define EMU_BLE_RDM_MAX_MS 		250
#define EMU_CAN_BROADCAST_RDM_MS 	1000
#define EMU_CAN_CMD_RDM_MS 		1000

#define PRODUCER_THREADS_START_DELAY_MS 2000u
#define CONSUMER_THREADS_START_DELAY_MS 3000u
#define COMMAND_THREADS_START_DELAY_MS 3000u
#define CANIOT_THREADS_START_DELAY_MS 1000u

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
void emu_caniot_broadcast_thread(void *_a, void *_b, void *_c);
void emu_caniot_cmd_thread(void *_a, void *_b, void *_c);
void emu_caniot_devices_thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(emu_ble_device1, 1024u, emu_ble_device, 1, NULL, NULL,
		K_PRIO_COOP(4u), 0u, PRODUCER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_ble_device2, 1024u, emu_ble_device, NULL, NULL, NULL,
		K_PRIO_COOP(4u), 0u, PRODUCER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_ble_device3, 1024u, emu_ble_device, NULL, NULL, NULL,
		K_PRIO_COOP(4u), 0u, PRODUCER_THREADS_START_DELAY_MS);

K_THREAD_DEFINE(emu_consumer1, 1024u, emu_consumer, NULL, NULL, NULL,
		K_PRIO_COOP(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_consumer2, 1024u, emu_consumer, NULL, NULL, NULL,
		K_PRIO_COOP(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_consumer3, 1024u, emu_consumer, NULL, NULL, NULL,
		K_PRIO_COOP(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_consumer4, 1024u, emu_consumer, NULL, NULL, NULL,
		K_PRIO_COOP(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);

/* Enable this to force devices to respond */
// K_THREAD_DEFINE(emu_caniot_broadcast_thread1, 1024u, emu_caniot_broadcast_thread, NULL, NULL, NULL,
// 		K_PRIO_COOP(4u), 0u, COMMAND_THREADS_START_DELAY_MS);

// K_THREAD_DEFINE(emu_caniot_cmd_thread1, 1024u, emu_caniot_cmd_thread, NULL, NULL, NULL,
// 		K_PRIO_COOP(4u), 0u, COMMAND_THREADS_START_DELAY_MS);

K_THREAD_DEFINE(emu_caniot_devices_thread1, 1024u, emu_caniot_devices_thread, NULL, NULL, NULL,
		K_PRIO_COOP(4u), 0u, CANIOT_THREADS_START_DELAY_MS);

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
					event->dev->room, event->dev->room->name);
			}

			ha_ev_unref(event);
		}
		
		k_sleep(K_MSEC(get_rdm_delay_ms_1() >> 2));
	}
}

void emu_caniot_broadcast_thread(void *_a, void *_b, void *_c)
{
	uint8_t caniot_ep = 0u;

	for (;;) {
		// ha_ev_t *resp = ha_dev_command(&target, &cmd, K_SECONDS(1));

		struct caniot_frame req;
		struct caniot_frame resp;
		uint32_t timeout = 200u;

		caniot_build_query_telemetry(&req, (caniot_ep++) % 4u);

		// caniot_did_t did = CANIOT_DID(0, 0);
		const caniot_did_t did = CANIOT_DID_BROADCAST;

		int ret = ha_ciot_ctrl_query(&req, &resp, did, &timeout);

		LOG_DBG("ret = %d", ret);

		k_sleep(K_MSEC(EMU_CAN_BROADCAST_RDM_MS));
	}
}

void emu_caniot_cmd_thread(void *_a, void *_b, void *_c)
{
	int ret;

	struct caniot_frame req;
	struct caniot_frame resp;

	uint8_t buf[8u];

	caniot_did_t did = CANIOT_DID(1, 4);

	for (;;) {

		uint32_t timeout = 200u;

		for (uint8_t i = 0u; i < 8u; i++) {
			buf[i] = 1u;
		}

		caniot_build_query_command(&req, CANIOT_ENDPOINT_BOARD_CONTROL,
					   buf, sizeof(buf));

		ret = ha_ciot_ctrl_query(&req, &resp, did, &timeout);

		LOG_DBG("CMD ret=%d timeout=%u", ret, timeout);

		timeout = 200u;

		caniot_build_query_telemetry(&req, CANIOT_ENDPOINT_BOARD_CONTROL);

		ret = ha_ciot_ctrl_query(&req, &resp, did, &timeout);

		LOG_DBG("TLM ret=%d timeout=%u", ret, timeout);

		k_sleep(K_MSEC(EMU_CAN_CMD_RDM_MS));
	}
}

struct emu_caniot_device {
	const struct caniot_identification id;
	struct caniot_device dev;
	struct caniot_config config;

	uint64_t state;
};

static uint64_t caniot_emu_state = 0ull;

int caniot_emu_telem(struct caniot_device *dev,
		     caniot_endpoint_t ep,
		     char *buf,
		     uint8_t *len)
{
	memcpy(buf, &caniot_emu_state, 8u);
	*len = 8u;

	k_sleep(K_MSEC(10u));

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

K_MSGQ_DEFINE(emu_caniot_txq, sizeof(struct caniot_frame), 2u, 4u);
K_MSGQ_DEFINE(emu_caniot_rxq, sizeof(struct caniot_frame), 2u, 4u);

struct caniot_config default_config = CANIOT_CONFIG_DEFAULT_INIT();

const struct caniot_api emu_caniot_api = {
	.config.on_read = NULL,
	.config.on_write = NULL,
	.custom_attr.read = NULL,
	.custom_attr.write = NULL,
	.telemetry_handler = caniot_emu_telem,
	.command_handler = caniot_emu_cmd,
};

void emu_caniot_device_init(struct emu_caniot_device *emu_dev)
{
	memcpy(&emu_dev->config, &default_config, sizeof(struct caniot_config));

	emu_dev->dev.api = &emu_caniot_api;
	emu_dev->dev.config = &emu_dev->config;
	emu_dev->dev.flags.request_telemetry = 0u;
	emu_dev->dev.identification = &emu_dev->id;
	caniot_device_system_reset(&emu_dev->dev);
	emu_dev->state = 0ull;
}

#define EMU_CANIOT_ID(_cls, _did, _magic, _name) \
	{ \
		.did = CANIOT_DID(_cls, _did), \
		.magic_number = _magic, \
		.name = _name, \
		.version = CANIOT_VERSION, \
	}

#define EMU_CANIOT(_cls, _did, _magic, _name) \
	{ \
		.id = EMU_CANIOT_ID(_cls, _did, _magic, _name), \
	}

struct emu_caniot_device caniot_devices[] = {
	EMU_CANIOT(0, 0, 0x01234567, "Test device 0.0"),
	EMU_CANIOT(0, 1, 0x12345678, "Test device 0.1"),
	EMU_CANIOT(0, 2, 0x23456789, "Test device 0.2"),
	EMU_CANIOT(0, 3, 0x34567890, "Test device 0.3"),
	EMU_CANIOT(0, 4, 0x45678901, "Test device 0.4"),
	EMU_CANIOT(0, 5, 0x56789012, "Test device 0.5"),
	EMU_CANIOT(0, 6, 0x67890123, "Test device 0.6"),
	EMU_CANIOT(0, 7, 0x78901234, "Test device 0.7"),
	EMU_CANIOT(1, 0, 0x56789012, "Test device 1.0"),
	EMU_CANIOT(1, 1, 0x67890123, "Test device 1.1"),
	EMU_CANIOT(1, 2, 0x78901234, "Test device 1.2"),
	EMU_CANIOT(1, 3, 0x89012345, "Test device 1.3"),
	EMU_CANIOT(1, 4, 0x90123456, "Test device 1.4"),
	EMU_CANIOT(1, 5, 0x01234567, "Test device 1.5"),
	EMU_CANIOT(1, 6, 0x12345678, "Test device 1.6"),
	EMU_CANIOT(1, 7, 0x23456789, "Test device 1.7"),
	// EMU_CANIOT(2, 0, 0x01234567, "Test device 2.0"),
	// EMU_CANIOT(2, 1, 0x12345678, "Test device 2.1"),
	// EMU_CANIOT(2, 2, 0x23456789, "Test device 2.2"),
	// EMU_CANIOT(2, 3, 0x34567890, "Test device 2.3"),
	// EMU_CANIOT(2, 4, 0x45678901, "Test device 2.4"),
	// EMU_CANIOT(2, 5, 0x56789012, "Test device 2.5"),
	// EMU_CANIOT(2, 6, 0x67890123, "Test device 2.6"),
	// EMU_CANIOT(2, 7, 0x78901234, "Test device 2.7"),
	// EMU_CANIOT(3, 0, 0x56789012, "Test device 3.0"),
	// EMU_CANIOT(3, 1, 0x67890123, "Test device 3.1"),
	// EMU_CANIOT(3, 2, 0x78901234, "Test device 3.2"),
	// EMU_CANIOT(3, 3, 0x89012345, "Test device 3.3"),
	// EMU_CANIOT(3, 4, 0x90123456, "Test device 3.4"),
	// EMU_CANIOT(3, 5, 0x01234567, "Test device 3.5"),
	// EMU_CANIOT(3, 6, 0x12345678, "Test device 3.6"),
	// EMU_CANIOT(3, 7, 0x23456789, "Test device 3.7"),
	// EMU_CANIOT(4, 0, 0x01234567, "Test device 4.0"),
	// EMU_CANIOT(4, 1, 0x12345678, "Test device 4.1"),
	// EMU_CANIOT(4, 2, 0x23456789, "Test device 4.2"),
	// EMU_CANIOT(4, 3, 0x34567890, "Test device 4.3"),
	// EMU_CANIOT(4, 4, 0x45678901, "Test device 4.4"),
	// EMU_CANIOT(4, 5, 0x56789012, "Test device 4.5"),
	// EMU_CANIOT(4, 6, 0x67890123, "Test device 4.6"),
	// EMU_CANIOT(4, 7, 0x78901234, "Test device 4.7"),
	// EMU_CANIOT(5, 0, 0x56789012, "Test device 5.0"),
	// EMU_CANIOT(5, 1, 0x67890123, "Test device 5.1"),
	// EMU_CANIOT(5, 2, 0x78901234, "Test device 5.2"),
	// EMU_CANIOT(5, 3, 0x89012345, "Test device 5.3"),
	// EMU_CANIOT(5, 4, 0x90123456, "Test device 5.4"),
	// EMU_CANIOT(5, 5, 0x01234567, "Test device 5.5"),
	// EMU_CANIOT(5, 6, 0x12345678, "Test device 5.6"),
	// EMU_CANIOT(5, 7, 0x23456789, "Test device 5.7"),
	// EMU_CANIOT(6, 0, 0x01234567, "Test device 6.0"),
	// EMU_CANIOT(6, 1, 0x12345678, "Test device 6.1"),
	// EMU_CANIOT(6, 2, 0x23456789, "Test device 6.2"),
	// EMU_CANIOT(6, 3, 0x34567890, "Test device 6.3"),
	// EMU_CANIOT(6, 4, 0x45678901, "Test device 6.4"),
	// EMU_CANIOT(6, 5, 0x56789012, "Test device 6.5"),
	// EMU_CANIOT(6, 6, 0x67890123, "Test device 6.6"),
	// EMU_CANIOT(6, 7, 0x78901234, "Test device 6.7"),
	// EMU_CANIOT(7, 0, 0x56789012, "Test device 7.0"),
	// EMU_CANIOT(7, 1, 0x67890123, "Test device 7.1"),
	// EMU_CANIOT(7, 2, 0x78901234, "Test device 7.2"),
	// EMU_CANIOT(7, 3, 0x89012345, "Test device 7.3"),
	// EMU_CANIOT(7, 4, 0x90123456, "Test device 7.4"),
	// EMU_CANIOT(7, 5, 0x01234567, "Test device 7.5"),
	// EMU_CANIOT(7, 6, 0x12345678, "Test device 7.6"),
};

void emu_caniot_devices_thread(void *_a, void *_b, void *_c)
{
	int ret;

	struct emu_caniot_device *emu_dev;

	for (emu_dev = caniot_devices;
	     emu_dev < caniot_devices + ARRAY_SIZE(caniot_devices);
	     emu_dev++) {
		emu_caniot_device_init(emu_dev);
	}

	struct caniot_frame req;
	struct caniot_frame resp;

	for (;;) {
		ret = k_msgq_get(&emu_caniot_txq, &req, K_FOREVER);
		if (ret) goto error;

		for (emu_dev = caniot_devices;
		     emu_dev < caniot_devices + ARRAY_SIZE(caniot_devices);
		     emu_dev++) {
			if (caniot_deviceid_match(emu_dev->id.did,
						  CANIOT_DID(req.id.cls, req.id.sid))) {
				ret = caniot_device_handle_rx_frame(&emu_dev->dev, &req, &resp);

				ret = k_msgq_put(&emu_caniot_rxq, &resp, K_FOREVER);
				if (ret) goto error;
			}
		}
	}

error:
	LOG_ERR("emu_caniot_devices_thread thread terminated with err=%d", ret);
}

int emu_caniot_send(struct caniot_frame *f)
{
	return k_msgq_put(&emu_caniot_txq, f, K_NO_WAIT);
}