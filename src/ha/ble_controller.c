/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

#include "ble_controller.h"

#include "net_time.h"

#include "ble/xiaomi_record.h"
#include "ha/devices.h"
#include "userio/leds.h"

#include <uart_ipc/ipc_frame.h>
#include <uart_ipc/ipc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ble_ctrlr, LOG_LEVEL_NONE);

K_MSGQ_DEFINE(ipc_ble_msgq, sizeof(ipc_frame_t), 1U, 4U);
struct k_poll_event ipc_event =
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&ipc_ble_msgq, 0);
static struct k_work_poll ipc_ble_work;


int process_xiaomi_dataframe(xiaomi_dataframe_t *frame)
{
	int ret = 0;
	char addr_str[BT_ADDR_STR_LEN];

	// show dataframe records
	LOG_DBG("Received BLE Xiaomi records count: %u, frame_time: %u",
		frame->count, frame->time);

	uint32_t frame_ref_time = frame->time;
	uint32_t now = net_time_get();

	// Process each record
	for (uint8_t i = 0; i < frame->count; i++) {
		xiaomi_record_t *const rec = &frame->records[i];

		bt_addr_to_str(&rec->addr.a, addr_str,
			       sizeof(addr_str));

		int32_t record_rel_time = rec->time - frame_ref_time;
		uint32_t record_timestamp = now + record_rel_time;
		rec->time = record_timestamp;

		ret = ha_dev_register_xiaomi_record(rec);
		if (ret != 0) {
			LOG_ERR("ha_dev_register_xiaomi_record() failed err=%d",
				ret);
			goto exit;
		}

		/* Show BLE address, temperature, humidity, battery
		 *   Only raw values are showed in debug because,
		 *   there is no formatting (e.g. float)
		 */
		LOG_INF("BLE Xiaomi rec %u [%d s]: %s [rssi %d] " \
			"temp: %d°C hum: %u %% bat: %u mV (%u %%)",
			i, record_rel_time,
			log_strdup(addr_str), 
			(int32_t)rec->measurements.rssi,
			(int32_t)rec->measurements.temperature / 100,
			(uint32_t)rec->measurements.humidity / 100,
			(uint32_t)rec->measurements.battery_mv,
			(uint32_t)rec->measurements.battery_level);
	}

exit:
	return ret;
}

static void ipc_work_handler(struct k_work *work)
{
	int ret;

	ipc_frame_t frame;

	/* process all available messages */
	while (k_msgq_get(&ipc_ble_msgq, &frame, K_NO_WAIT) == 0U) {
		ret = process_xiaomi_dataframe(
			(xiaomi_dataframe_t *)frame.data.buf);
		if (ret != 0) {
			LOG_ERR("Failed to handle BLE Xiaomi record, err: %d",
				ret);
		}
	}

	// optional ?
	ipc_event.state = K_POLL_STATE_NOT_READY;

	/* re-register for next event */
	ret = k_work_poll_submit(&ipc_ble_work, &ipc_event, 1U, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to resubmit work %p to poll queue: %d", work, ret);
	}
}

void ipc_ev_cb(ipc_event_t event,
	       void *data)
{
	static led_state_t state = LED_OFF;

	switch (event) {
	case IPC_DATA_FRAME_RECEIVED:
		state = (led_state_t)(1 - state);
		leds_set(LED_BLE, state);
		break;
	default:
		break;
	}
}

int ha_ble_controller_init(void)
{
	int ret = net_time_wait_synced(K_FOREVER);
	if (ret == 0) {
		ipc_attach_rx_msgq(&ipc_ble_msgq);
		ipc_register_event_callback(ipc_ev_cb, NULL);

		k_work_poll_init(&ipc_ble_work, ipc_work_handler);
		k_work_poll_submit(&ipc_ble_work, &ipc_event, 1U, K_FOREVER);

	} else {
		LOG_ERR("Time not synced ! ret = %d", ret);
	}

	return ret;
}