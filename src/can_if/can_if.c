#include "can_if.h"

#include "cantcp/cantcp_server.h"

#include <kernel.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(can, LOG_LEVEL_INF);

#define CAN1_DEVICE DEVICE_DT_GET(DT_NODELABEL(can1))

const struct zcan_filter filter = {
                .id = 0
};

static void can_rx_thread(const struct device *dev,
			  struct k_msgq *msgq,
			  struct zcan_filter *filter);
static void can_tx_thread(const struct device *can_dev,
			  void *_b,
			  void *_c);

CAN_DEFINE_MSGQ(can_rx_msgqueue, 2);
K_THREAD_DEFINE(canrx, 0x500, can_rx_thread, CAN1_DEVICE, 
		&can_rx_msgqueue, &filter, K_PRIO_COOP(5), 0, 0);

K_THREAD_DEFINE(cantx, 0x500, can_tx_thread, CAN1_DEVICE, 
		NULL, NULL, K_PRIO_COOP(5), 0, 0);

static void can_rx_thread(const struct device *dev,
			  struct k_msgq *msgq,
			  struct zcan_filter *filter)
{
        int ret;
	struct zcan_frame frame;

        ret = can_attach_msgq(dev, msgq, filter);
        LOG_DBG("can_attach_msgq(%p, %p, %p) = %d", dev, msgq, filter, ret);

        for (;;) {
                k_msgq_get(msgq, &frame, K_FOREVER);

                LOG_INF("RX id_type=%u rtr=%u id=%u dlc=%u", frame.id_type, 
			frame.rtr, frame.id, frame.dlc);
                LOG_HEXDUMP_INF(frame.data, frame.dlc, "can data");

		ret = cantcp_server_broadcast(&frame);
		if (ret < 0) {
			LOG_ERR("cantcp_server_broadcast() failed = %d", ret);
		}
        }
}

K_MSGQ_DEFINE(can_tx_msgqueue, sizeof(struct zcan_frame), 1U, 4U);

int can_queue(struct zcan_frame *frame)
{
	return k_msgq_put(&can_tx_msgqueue, frame, K_NO_WAIT);
}

// const struct zcan_frame frame = {
//         .id_type = CAN_STANDARD_IDENTIFIER,
//         .rtr = CAN_DATAFRAME,
//         .id = 0x123,
//         .dlc = 8,
//         .data = {1, 2, 3, 4, 5, 6, 7, 8}
// };


static void can_tx_thread(const struct device *dev, void *_b, void *_c)
{
	struct zcan_frame frame;

        while (!device_is_ready(dev)) {
                LOG_INF("CAN: Device %s not ready.\n", dev->name);
                k_sleep(K_SECONDS(1));
        }

	cantcp_server_attach_rx_msgq(&can_tx_msgqueue);
	
        for (;;) {
		if (k_msgq_get(&can_tx_msgqueue, &frame, K_FOREVER) == 0) {
			can_send(dev, &frame, K_FOREVER, NULL, NULL);
			LOG_INF("TX id_type=%u rtr=%u id=%u dlc=%u",
				frame.id_type, frame.rtr, frame.id, frame.dlc);
			LOG_HEXDUMP_INF(frame.data, frame.dlc, "data");
		}
        }
}


