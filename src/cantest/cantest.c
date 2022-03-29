#include <kernel.h>

#include <drivers/can.h>

#include "cantcp/cantcp.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cantcp_test, LOG_LEVEL_DBG);

void thread(void *_a, void *_b, void *_c);

// K_THREAD_DEFINE(cantest_thread, 0x3000, thread, NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, 0);

void thread(void *_a, void *_b, void *_c)
{
	int ret;
	struct cantcp_tunnel tunnel;
	struct zcan_frame frame;

	cantcp_client_tunnel_init(&tunnel);

	tunnel.server.hostname = "192.168.10.236"; // "192.168.10.240"  "laptop-dev.local" "192.168.10.225"
	tunnel.server.port = CANTCP_DEFAULT_PORT;

	k_sleep(K_SECONDS(1));

	for (;;) {
		k_sleep(K_SECONDS(2));

		ret = cantcp_connect(&tunnel);
		if (ret != 0) {
			continue;
		}

		frame.id_type = CAN_ID_STD;
		frame.rtr = CAN_RTR_DATA;
		frame.id = 1;
		frame.dlc = 8;
		memset(frame.data, 0x55, 8);

		ret = cantcp_send(&tunnel, &frame);
		if (ret != 0) {
			LOG_ERR("Failed to send = %d", ret);
			continue;
		}

		ret = cantcp_recv(&tunnel, &frame);
		if (ret != 0) {
			LOG_ERR("Failed to recv = %d", ret);
			continue;
		}

		cantcp_disconnect(&tunnel);
	}
}