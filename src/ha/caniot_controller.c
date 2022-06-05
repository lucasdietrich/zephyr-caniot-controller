#include "caniot_controller.h"

#include <caniot/caniot.h>
#include <caniot/controller.h>
#include <caniot/datatype.h>

#include "devices.h"

#include "net_time.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(caniot, LOG_LEVEL_DBG);

struct caniot_controller ctrl;

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

int ha_ciot_ctrl_init(void)
{
	return caniot_controller_init(&ctrl);
}


int ha_ciot_process_frame(struct zcan_frame *frame)
{
	int ret = -EINVAL;

	if ((frame == NULL) || 
	    (frame->id_type != CAN_ID_STD) ||
	    (frame->rtr == 1U)) {
		goto exit;
	}

	struct caniot_frame ctf;
	caniot_clear_frame(&ctf);
	ctf.id = caniot_canid_to_id((uint16_t) frame->id);
	ctf.len = MIN(frame->dlc, 8U);
	memcpy(ctf.buf, frame->data, ctf.len);

	caniot_controller_process(&ctrl);

	if (caniot_controller_is_target(&ctf) == true) {
		char buf[64];
		ret = caniot_explain_frame_str(&ctf, buf, sizeof(buf));
		if (ret > 0) {
			LOG_INF("%s", log_strdup(buf));
		} else {
			LOG_WRN("Failed to encode frame, ret = %d", ret);
		}
		
		ret = handle_caniot_frame(&ctf);
	}
exit:
	return ret;
}