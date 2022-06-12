#include "dispatcher.h"

#if 0

#include "cantcp/cantcp_server.h"
#include "ha/caniot_controller.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(can_dispatcher, LOG_LEVEL_WRN);

typedef int (*can_handler_func_t)(struct zcan_frame *frame);

struct can_handler
{
	const char *name;
	can_handler_func_t handler;
	// struct zcan_filter filter;
	struct {
		/* enable handler */
		uint8_t enabled: 1;

		/* enables use of filter */
		// uint8_t filter_enabled: 1;

		/* passe frame to next handler if no filter matches,
		 * if filter doesn't match, frame is automatically 
		 * tested against the next module
		 */
		uint8_t pass_next: 1;
	} flags;
};

static const struct can_handler handlers[] = {
	{
		.name = "caniot",
		.handler = ha_ciot_process_frame,
		// .filter = {
		// 	.id_type = CAN_ID_STD,
		// },
		.flags = {
			.enabled = 1U,
			// .filter_enabled = 1U,
			.pass_next = 1U,
		}
	},
#if defined(CONFIG_CANTCP_SERVER)
	{
		.name = "cantcp",
		.handler = cantcp_server_broadcast,
		.flags = {
			.enabled = 1U,
			// .filter_enabled = 0U,
			.pass_next = 1U,
		}
	}
#endif
};


int can_dispatch(CAN_bus_t bus, struct zcan_frame *frame)
{
	int ret = 0;
	uint32_t count = 0U;
	const struct can_handler *hdlr = NULL;

	for (hdlr = handlers; hdlr < handlers + ARRAY_SIZE(handlers); hdlr++) {
		if ((hdlr->flags.enabled) && (hdlr->handler != NULL)) {
			count++;
			ret = hdlr->handler(frame);
			LOG_DBG("%s: ret = %d (%u pass_next)", log_strdup(hdlr->name),
				ret, hdlr->flags.pass_next == true ? 1 : 0);
			
			if (hdlr->flags.pass_next != 1U) {
				break;
			}
		}
	}

	LOG_DBG("CAN msg dispatched to %u handlers", count);

	return ret;
}

#endif