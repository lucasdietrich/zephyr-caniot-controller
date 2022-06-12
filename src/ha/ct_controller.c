#include <stdio.h>

#include <caniot/caniot.h>
#include <caniot/controller.h>
#include <caniot/datatype.h>

#include "ct_controller.h"
#include "devices.h"
#include "net_time.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(caniot, LOG_LEVEL_DBG);

static void thread(void *_a, void *_b, void *_c);

CAN_DEFINE_MSGQ(ha_ciot_ctrl_rx_msgq, 4U);

K_THREAD_DEFINE(ha_ciot_thread, 0x1000, thread, NULL, NULL, NULL,
		K_PRIO_COOP(7), 0U, 0U);

static struct caniot_controller ctrl;

static int zcan_to_caniot(struct zcan_frame *zcan,
			  struct caniot_frame *caniot)
{
	if ((zcan == NULL) || (caniot == NULL)) {
		return -EINVAL;
	}

	caniot_clear_frame(caniot);
	caniot->id = caniot_canid_to_id((uint16_t)zcan->id);
	caniot->len = MIN(zcan->dlc, 8U);
	memcpy(caniot->buf, zcan->data, caniot->len);

	return 0U;
}

// static
int caniot_to_zcan(struct zcan_frame *zcan,
			  struct caniot_frame *caniot)
{
	if ((zcan == NULL) || (caniot == NULL)) {
		return -EINVAL;
	}

	memset(zcan, 0x00U, sizeof(struct zcan_frame));

	zcan->id = caniot_id_to_canid(caniot->id);
	zcan->dlc = MIN(caniot->len, 8U);
	memcpy(zcan->data, caniot->buf, zcan->dlc);

	return 0U;
}

bool event_cb(const caniot_controller_event_t *ev,
	      void *user_data)
{
	return true;
}

bool event_cb2(const caniot_controller_event_t *ev,
	      void *user_data)
{
	int ret;

	switch (ev->status) {
	case CANIOT_CONTROLLER_EVENT_STATUS_OK:
	{
		const struct caniot_frame *resp = ev->response;
		__ASSERT(resp != NULL, "response is NULL");

		if ((resp->id.type == CANIOT_FRAME_TYPE_TELEMETRY) &&
		    (resp->id.endpoint == CANIOT_ENDPOINT_BOARD_CONTROL)) {
			if (resp->len != 8U) {
				LOG_WRN("Expected length for board control "
					"telemetry is 8, got %d",
					resp->len);
			}

			ret = ha_dev_register_caniot_telemetry(
				net_time_get(),
				CANIOT_DID(resp->id.cls, resp->id.sid),
				AS_BOARD_CONTROL_TELEMETRY(resp->buf)
			);
		}
	}
	break;
	case CANIOT_CONTROLLER_EVENT_STATUS_ERROR:
		break;
	case CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT:
		break;
	case CANIOT_CONTROLLER_EVENT_STATUS_CANCELLED:
		break;
	default:
		break;
	}

	return true;
}

static void thread(void *_a, void *_b, void *_c)
{
	ARG_UNUSED(_a);
	ARG_UNUSED(_b);
	ARG_UNUSED(_c);

	int ret;
	static struct caniot_frame frame;
	static struct zcan_frame zframe;
	uint32_t reftime = k_uptime_get_32();

	caniot_controller_init(&ctrl, event_cb, NULL);

	while (1) {
		const uint32_t timeout = caniot_controller_next_timeout(&ctrl);

		ret = k_msgq_get(&ha_ciot_ctrl_rx_msgq, &zframe, K_MSEC(timeout));
		if ((ret < 0) && (ret != -EAGAIN)) {
			LOG_ERR("k_msgq_get failed: %d", ret);
			continue;
		}

		const uint32_t now = k_uptime_get_32();
		const uint32_t delta = now - reftime;
		reftime = now;

		if (ret == 0) {
			zcan_to_caniot(&zframe, &frame);

			char buf[64];
			ret = caniot_explain_frame_str(&frame, buf, sizeof(buf));
			if (ret > 0) {
				LOG_INF("%s", log_strdup(buf));
			} else {
				LOG_WRN("Failed to encode frame, ret = %d", ret);
			}

			caniot_controller_process_single(&ctrl, delta, &frame);
		} else if (ret == -EAGAIN) {
			caniot_controller_process_single(&ctrl, delta, NULL);
		}
	}
}