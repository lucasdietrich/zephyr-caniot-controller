#include <zephyr.h>

#include "ipc_uart/ipc_frame.h"
#include "ipc_uart/ipc.h"

#include "ble/xiaomi_record.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(devices_controller, LOG_LEVEL_INF);

void devices_controller_thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(devices_controller_thread_id, 0x400, devices_controller_thread,
		NULL, NULL, NULL, K_PRIO_COOP(8), 0, 0);

K_MSGQ_DEFINE(msgq, IPC_FRAME_SIZE, 1, 4);

void devices_controller_thread(void *_a, void *_b, void *_c)
{
	ipc_attach_rx_msgq(&msgq);

	static ipc_frame_t frame;

	char addr_str[BT_ADDR_LE_STR_LEN];

	for (;;) {

		if (k_msgq_get(&msgq, (void *)&frame, K_FOREVER) == 0) {
			LOG_HEXDUMP_DBG(frame.data.buf, frame.data.size, "IPC frame");

			xiaomi_dataframe_t *const dataframe =
				(xiaomi_dataframe_t *)frame.data.buf;

			// show dataframe records
			LOG_INF("Received BLE Xiaomi records count: %u, frame_time: %u",
				dataframe->count, dataframe->frame_time);

			uint32_t frame_ref_time = dataframe->frame_time;

			// Show all records
			for (uint8_t i = 0; i < dataframe->count; i++) {
				xiaomi_record_t *const rec = &dataframe->records[i];

				bt_addr_le_to_str(&rec->addr, 
						  addr_str,
						  sizeof(addr_str));

				int32_t record_rel_time = rec->uptime - frame_ref_time;

				 // Show BLE address, temperature, humidity, battery
				LOG_INF("\tBLE Xiaomi record %u [%d s]: addr: %s, " \
					"temp: %d.%d °C, hum: %u %%, bat: %u mV",
					i,
					record_rel_time,
					log_strdup(addr_str),
					rec->measurements.temperature / 100,
					rec->measurements.temperature % 100,
					rec->measurements.humidity,
					rec->measurements.battery);
			}
		}
	}
}