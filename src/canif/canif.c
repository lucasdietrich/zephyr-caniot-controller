#include <kernel.h>
#include <poll.h>

#include <caniot/caniot.h>
#include <ha/ct_controller.h>

#include "canif/canif.h"
#include "dispatcher.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(can, LOG_LEVEL_NONE);

#define CAN1_DEVICE DEVICE_DT_GET(DT_NODELABEL(can1))


static void can_thread(const struct device *dev,
		       struct k_msgq *rx_msgq,
		       struct k_msgq *tx_msgq);

// CAN_DEFINE_MSGQ(rx_msgqueue, 4);
CAN_DEFINE_MSGQ(tx_msgqueue, 4);

K_THREAD_DEFINE(cantid, 0x400, can_thread, CAN1_DEVICE,
		&tx_msgqueue, NULL, K_PRIO_COOP(5), 0, 0);

// static int handle_received_frame(struct zcan_frame *frame);

/* TODO remove these threads and use k_work_poll in dispatch.c file */
static void can_thread(const struct device *dev,
		       struct k_msgq *txq,
		       struct k_msgq *rxq)
{
	ARG_UNUSED(rxq);

	caniot_test();

	int ret;
	struct zcan_frame frame;

	/* attach message q */
	struct zcan_filter filter = {
		.id_type = CAN_ID_STD, /* currently we ignore extended IDs */
	};
	ret = can_attach_msgq(dev, &ha_ciot_ctrl_rx_msgq, &filter);
	if (ret) {
		LOG_ERR("can_attach_msgq failed: %d", ret);
		return;
	}

#if defined(CONFIG_CANTCP_SERVER)
	cantcp_server_attach_rx_msgq(txq);
#endif /* defined(CONFIG_CANTCP_SERVER) */


	/* wait for device ready */
        while (!device_is_ready(dev)) {
                LOG_WRN("CAN: Device %s not ready.\n", dev->name);
                k_sleep(K_SECONDS(1));
        }
	
	struct k_poll_event events[] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						txq, 0),
		/*
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						rxq, 0),
		*/
	};

	/* poll for events */
	for (;;) {
		ret = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		if (ret >= 0) {
			if (events[0].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
				ret = k_msgq_get(txq, &frame, K_NO_WAIT);

				__ASSERT(ret == 0, "Failed to get TX  CAN frame from msgq");

				ret = can_send(dev, &frame, K_FOREVER, NULL, NULL);
				if (ret < 0) {
					LOG_ERR("Failed to send CAN frame, ret = %d", ret);
				}
			}

			/*
			if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
				ret = k_msgq_get(rxq, &frame, K_NO_WAIT);

				__ASSERT(ret == 0, "Failed to get received CAN frame");

				handle_received_frame(&frame);
			}
			*/

			events[0].state = K_POLL_STATE_NOT_READY;

			/*
			events[1].state = K_POLL_STATE_NOT_READY;
			*/
		}
	}
}

/*
static int handle_received_frame(struct zcan_frame *frame)
{
	// show received frame
	LOG_DBG("RX id_type=%u rtr=%u id=%x dlc=%u", frame->id_type,
		frame->rtr, frame->id, frame->dlc);
	LOG_HEXDUMP_DBG(frame->data, frame->dlc, "can data");

	ha_ciot_process_frame(frame);
}
*/

int can_queue(CAN_bus_t bus, struct zcan_frame *frame, uint32_t delay_ms)
{
	if (bus != CAN_BUS_1) {
		LOG_ERR("CAN bus %d not supported", bus);
		return -EINVAL;
	}

	if (delay_ms != 0) {
		LOG_WRN("CAN: delay not supported, forced delay %u to 0 ms", delay_ms);
		delay_ms = 0;
	}

	return k_msgq_put(&tx_msgqueue, frame, K_NO_WAIT);
}

// const struct zcan_frame frame = {
//         .id_type = CAN_STANDARD_IDENTIFIER,
//         .rtr = CAN_DATAFRAME,
//         .id = 0x123,
//         .dlc = 8,
//         .data = {1, 2, 3, 4, 5, 6, 7, 8}
// };