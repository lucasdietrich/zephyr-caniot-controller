#include "canif/canif.h"

#include "cantcp/cantcp_server.h"

#include <kernel.h>
#include <poll.h>

#include <logging/log.h>

#include <canif/caniot_controller.h>

LOG_MODULE_REGISTER(can, LOG_LEVEL_WRN);

#define CAN1_DEVICE DEVICE_DT_GET(DT_NODELABEL(can1))


static void can_thread(const struct device *dev,
		       struct k_msgq *rx_msgq,
		       struct k_msgq *tx_msgq);

CAN_DEFINE_MSGQ(rx_msgqueue, 4);
CAN_DEFINE_MSGQ(tx_msgqueue, 4);

K_THREAD_DEFINE(cantid, 0x500, can_thread, CAN1_DEVICE,
		&rx_msgqueue, &tx_msgqueue, K_PRIO_COOP(5), 0, 0);

static int handle_received_frame(struct zcan_frame *frame);

static void can_thread(const struct device *dev,
		       struct k_msgq *rxq,
		       struct k_msgq *txq)
{
	int ret;
	struct zcan_frame frame;

	/* attach message q */
	struct zcan_filter filter = {
		.id_type = CAN_ID_STD, /* currently we ignore extended IDs */
	};
	ret = can_attach_msgq(dev, rxq, &filter);
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
						rxq, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						txq, 0),
	};

	/* poll for events */
	for (;;) {
		ret = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		if (ret >= 0) {
			if (events[0].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
				ret = k_msgq_get(rxq, &frame, K_NO_WAIT);

				__ASSERT(ret == 0, "Failed to get received CAN frame");

				handle_received_frame(&frame);
			}

			if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
				ret = k_msgq_get(txq, &frame, K_NO_WAIT);

				__ASSERT(ret == 0, "Failed to get TX  CAN frame from msgq");

				ret = can_send(dev, &frame, K_FOREVER, NULL, NULL);
				if (ret < 0) {
					LOG_ERR("Failed to send CAN frame, ret = %d", ret);
				}
			}

			events[0].state = K_POLL_STATE_NOT_READY;
			events[1].state = K_POLL_STATE_NOT_READY;
		}
	}
}

static int handle_received_frame(struct zcan_frame *frame)
{
	int ret = 0;

	/* show received frame */
	LOG_DBG("RX id_type=%u rtr=%u id=%x dlc=%u", frame->id_type,
		frame->rtr, frame->id, frame->dlc);
	LOG_HEXDUMP_DBG(frame->data, frame->dlc, "can data");

	/* broadcast to cantcp connections if any */
#if defined(CONFIG_CANTCP_SERVER)
	ret = cantcp_server_broadcast(frame);
	if (ret < 0) {
		LOG_ERR("cantcp_server_broadcast() failed = %d", ret);
	}
#endif /* CONFIG_CANTCP_SERVER */

#if defined(CONFIG_CANIOT_CONTROLLER)
	caniot_process_can_frame(frame);
#endif 

	return ret;
}

int can_queue(CAN_bus_t bus, struct zcan_frame *frame)
{
	if (bus != CAN_BUS_1) {
		LOG_ERR("CAN bus %d not supported", bus);
		return -EINVAL;
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