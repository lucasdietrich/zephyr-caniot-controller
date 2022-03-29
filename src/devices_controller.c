#include <zephyr.h>

#include "ipc_uart/ipc_frame.h"
#include "ipc_uart/ipc.h"

#include "ble/xiaomi_record.h"
#include "net_time.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(devices_controller, LOG_LEVEL_INF);

void devices_controller_thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(devices_controller_thread_id, 0x400, devices_controller_thread,
		NULL, NULL, NULL, K_PRIO_COOP(8), 0, 0);

K_MSGQ_DEFINE(msgq, IPC_FRAME_SIZE, 1, 4);

xiaomi_record_t records[15];
uint32_t records_count = 0U;

K_MUTEX_DEFINE(records_mutex);

static xiaomi_record_t *get_record_from_addr(bt_addr_le_t *addr)
{
	static xiaomi_record_t *rec = NULL;

	for (uint32_t i = 0U; i < records_count; i++) {
		if (bt_addr_le_cmp(&records[i].addr, addr) == 0) {
			rec = &records[i];
		}
	}

	return rec;
}

static xiaomi_record_t *get_first_record(void)
{
	xiaomi_record_t *rec = NULL;

	if (records_count >= 1U) {
		rec = &records[0];
	}

	return rec;
}

static xiaomi_record_t *get_record(uint32_t index)
{
	xiaomi_record_t *rec = NULL;

	if (index < records_count) {
		rec = &records[index];
	}

	return rec;
}

static xiaomi_record_t *get_next_record(xiaomi_record_t *cur)
{
	xiaomi_record_t *rec = NULL;

	if (cur != NULL) {
		return get_record(cur - records + 1);
	}

	return rec;
}

static void update_local_record(xiaomi_record_t *rec)
{
	xiaomi_record_t *local = get_record_from_addr(&rec->addr);

	if (local == NULL) {
		if (records_count < ARRAY_SIZE(records)) {
			memcpy(&records[records_count], rec,
			       sizeof(xiaomi_record_t));
			records_count++;
		}
	} else {
		memcpy(local, rec, sizeof(xiaomi_record_t));
	}
}

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
				dataframe->count, dataframe->time);

			uint32_t frame_ref_time = dataframe->time;
			uint32_t now = net_time_get();

			k_mutex_lock(&records_mutex, K_FOREVER);

			// Show all records
			for (uint8_t i = 0; i < dataframe->count; i++) {
				xiaomi_record_t *const rec = &dataframe->records[i];

				bt_addr_le_to_str(&rec->addr, 
						  addr_str,
						  sizeof(addr_str));

				int32_t record_rel_time = rec->time - frame_ref_time;
				uint32_t record_timestamp = now + record_rel_time;
				rec->time = record_timestamp;

				update_local_record(rec);

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
			
			k_mutex_unlock(&records_mutex);
		}
	}
}

/*___________________________________________________________________________*/

// xiaomi_record_t *dev_ble_xiaomi_get_first_record(void)
// {
// 	k_mutex_lock(&records_mutex, K_FOREVER);

// 	xiaomi_record_t *rec = get_first_record();

// 	k_mutex_unlock(&records_mutex);

// 	return rec;
// }

// xiaomi_record_t *dev_ble_xiaomi_get_next_record(xiaomi_record_t *cur)
// {
// 	k_mutex_lock(&records_mutex, K_FOREVER);

// 	xiaomi_record_t *rec = get_next_record(cur);

// 	k_mutex_unlock(&records_mutex);

// 	return rec;
// }

// xiaomi_record_t *dev_ble_xiaomi_get_record(uint32_t index)
// {
// 	k_mutex_lock(&records_mutex, K_FOREVER);

// 	xiaomi_record_t *rec = get_record(index);

// 	k_mutex_unlock(&records_mutex);

// 	return rec;
// }

size_t dev_ble_xiaomi_iterate(void (*callback)(xiaomi_record_t *rec,
					       void *user_data),
			      void *user_data)
{
	size_t count = 0U;

	k_mutex_lock(&records_mutex, K_FOREVER);

	xiaomi_record_t *rec = get_first_record();

	while (rec != NULL) {
		callback(rec, user_data);
		rec = get_next_record(rec);
		count++;
	}

	k_mutex_unlock(&records_mutex);

	return count;
}