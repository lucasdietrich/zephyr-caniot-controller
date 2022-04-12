#include "caniot_controller.h"

static void caniot_thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(caniotid, 0x500, caniot_thread, NULL, NULL, NULL,
		K_PRIO_PREEMPT(8), 0, 0);

static void caniot_thread(void *_a, void *_b, void *_c)
{
	
}