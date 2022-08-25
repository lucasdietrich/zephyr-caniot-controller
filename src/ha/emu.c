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

#include <logging/log.h>
LOG_MODULE_REGISTER(ha_emu, LOG_LEVEL_INF);

#define EMU_BLE_RDM_MIN_MS 200
#define EMU_BLE_RDM_MAX_MS 2000

#define PRODUCER_THREADS_START_DELAY_MS 2000u
#define CONSUMER_THREADS_START_DELAY_MS 3000u

static uint32_t get_rdm_delay_ms(uint32_t min, uint32_t max)
{
	return sys_rand32_get() % (max - min) + min;
}

static uint32_t get_rdm_delay_ms_1(void)
{
	return get_rdm_delay_ms(EMU_BLE_RDM_MIN_MS, EMU_BLE_RDM_MAX_MS);
}

void emu_thread_provider(void *_a, void *_b, void *_c);
void emu_thread_consumer(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(emu_thread_provider1, 1024u, emu_thread_provider, 1, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, PRODUCER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_thread_provider2, 1024u, emu_thread_provider, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, PRODUCER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_thread_provider3, 1024u, emu_thread_provider, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, PRODUCER_THREADS_START_DELAY_MS);

K_THREAD_DEFINE(emu_thread_consumer1, 1024u, emu_thread_consumer, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_thread_consumer2, 1024u, emu_thread_consumer, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_thread_consumer3, 1024u, emu_thread_consumer, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);
K_THREAD_DEFINE(emu_thread_consumer4, 1024u, emu_thread_consumer, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4u), 0u, CONSUMER_THREADS_START_DELAY_MS);

#define EMU_BLE_ADDR_INIT(_type, _last) \
	{ _type, { { 0xFF, 0xFF, 0xFF, 0x00, 0x00, _last } } }

static const bt_addr_le_t addrs[] = {
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0xECu, 0x1Cu, 0x6Du),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x68u, 0x05u, 0x63u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x0Au, 0x1Eu, 0x38u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0xA7u, 0x30u, 0xC4u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0xD5u, 0x08u, 0x40u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x28u, 0x17u, 0xE3u),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0xE0u, 0x18u, 0xEDu),
	HA_BT_ADDR_LE_PUBLIC_INIT(XIAOMI_BT_LE_ADDR_0, XIAOMI_BT_LE_ADDR_1, XIAOMI_BT_LE_ADDR_2, 0x8Du, 0xBAu, 0xB4u),
};

void emu_thread_provider(void *_a, void *_b, void *_c)
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

		// ha_dev_t *dev = ha_dev_get_by_addr(&addr);

		// if (dev == NULL) {
		// 	dev = ha_dev_register(&addr);
		// }

		// if (dev != NULL) {
			
		// }

		ret = ha_dev_register_data(&addr, &record, sizeof(xiaomi_record_t), 0);
		

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

void emu_thread_consumer(void *_a, void *_b, void *_c)
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
				event->time,
				((struct ha_xiaomi_dataset *)event->data)->temperature.value);

			if (event->dev && event->dev->room) {
				LOG_INF("(%p %p %p) Room: %s", event, event->dev, 
					event->dev->room, log_strdup(event->dev->room->name));
			}

			ha_ev_unref(event);
		}
		
		k_sleep(K_MSEC(get_rdm_delay_ms_1() >> 2));
	}
}