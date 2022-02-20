#include "can_if.h"

#include <kernel.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(can, LOG_LEVEL_INF);

#define CAN1_DEVICE DEVICE_DT_GET(DT_NODELABEL(can1))

const struct zcan_filter filter = {
                .id = 0
};

static void can_rx_thread(const struct device *dev, struct k_msgq *msgq, struct zcan_filter *filter);
static void can_tx_thread(const struct device *can_dev, void *_b, void *_c);

CAN_DEFINE_MSGQ(can_rx_msgqueue, 2);
K_THREAD_DEFINE(canrx, 0x500, can_rx_thread, CAN1_DEVICE, &can_rx_msgqueue, &filter, K_PRIO_COOP(5), 0, 0);

K_THREAD_DEFINE(cantx, 0x500, can_tx_thread, CAN1_DEVICE, NULL, NULL, K_PRIO_COOP(5), 0, 0);

static void can_rx_thread(const struct device *dev, struct k_msgq *msgq, struct zcan_filter *filter)
{
        int ret;

        ret = can_attach_msgq(dev, msgq, filter);
        LOG_DBG("can_attach_msgq(%p, %p, %p) = %d", dev, msgq, filter, ret);

        struct zcan_frame rx;

        for (;;) {
                k_msgq_get(msgq, &rx, K_FOREVER);

                LOG_INF("RX id_type=%u rtr=%u id=%u dlc=%u", rx.id_type, rx.rtr, rx.id, rx.dlc);
                LOG_HEXDUMP_INF(rx.data, rx.dlc, "can data");
        }
}

static void can_tx_thread(const struct device *dev, void *_b, void *_c)
{
        int ret;

        const struct zcan_frame frame = {
                .id_type = CAN_STANDARD_IDENTIFIER,
                .rtr = CAN_DATAFRAME,
                .id = 0x123,
                .dlc = 8,
                .data = {1, 2, 3, 4, 5, 6, 7, 8}
        };

        while (!device_is_ready(dev)) {
                LOG_INF("CAN: Device %s not ready.\n", dev->name);
                k_sleep(K_SECONDS(1));
        }

        for (;;) {
                ret = can_send(dev, &frame, K_MSEC(100), NULL, NULL);
                if (ret != CAN_TX_OK) {
                        LOG_ERR("Sending failed [%d]", ret);
                }

                k_sleep(K_SECONDS(5));
        }
}


