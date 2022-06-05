#include <kernel.h>

#include "ble_controller.h"

#include "net_time.h"

#include "ble/xiaomi_record.h"
#include "uart_ipc/ipc_frame.h"
#include "uart_ipc/ipc.h"
#include "ha/devices.h"
#include "userio/leds.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ble_ctrlr, LOG_LEVEL_NONE);

K_MSGQ_DEFINE(ipc_ble_msgq, sizeof(ipc_frame_t), 1U, 4U);
struct k_poll_event ipc_event =
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&ipc_ble_msgq, 0);
static struct k_work_poll ipc_ble_work;

static void ipc_work_handler(struct k_work *work)
{
	int ret;

	ipc_frame_t frame;

	/* process all available messages */
	while (k_msgq_get(&ipc_ble_msgq, &frame, K_NO_WAIT) == 0U) {
		ret = ha_register_xiaomi_from_dataframe(
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