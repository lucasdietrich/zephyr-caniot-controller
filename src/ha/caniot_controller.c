#include "caniot_controller.h"

#include <caniot/caniot.h>
#include <caniot/controller.h>
#include <caniot/datatype.h>

#include "devices.h"

#include "net_time.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(caniot, LOG_LEVEL_WRN);

static int handle_caniot_frame(struct caniot_frame *ctf)
{
	int ret = -EINVAL;

	if ((ctf->id.type == CANIOT_FRAME_TYPE_TELEMETRY) &&
	    (ctf->id.endpoint == CANIOT_ENDPOINT_BOARD_CONTROL)) {
		if (ctf->len != 8U) {
			LOG_WRN("Expected length for board control telemetry is 8, got %d",
				ctf->len);
		}

		ret = ha_dev_register_caniot_telemetry(
			net_time_get(),
			CANIOT_DEVICE(ctf->id.cls, ctf->id.sid),
			AS_BOARD_CONTROL_TELEMETRY(ctf->buf)
		);
	}

	return ret;
}

int caniot_process_can_frame(struct zcan_frame *frame)
{
	int ret = -EINVAL;

	if ((frame->id_type != CAN_ID_STD) ||
	    (frame->rtr == 1U)) {
		goto exit;
	}

	struct caniot_frame ctf;
	caniot_clear_frame(&ctf);
	ctf.id = caniot_canid_to_id(frame->id);
	ctf.len = MIN(frame->dlc, 8U);
	memcpy(ctf.buf, frame->data, ctf.len);

	if (caniot_controller_is_target(&ctf) == true) {
		char buf[64];
		ret = caniot_explain_frame_str(&ctf, buf, sizeof(buf));
		if (ret > 0) {
			LOG_INF("%s", buf);
		} else {
			LOG_WRN("Failed to encode frame, ret = %d", ret);
		}

		handle_caniot_frame(&ctf);
	}
exit:
	return ret;
}