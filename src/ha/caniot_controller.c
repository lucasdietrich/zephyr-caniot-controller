/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "can/can_interface.h"
#include "caniot_controller.h"
#include "emu.h"
#include "ha/core/ha.h"
#include "ha/core/utils.h"
#include "ha/devices/caniot.h"
#include "net_time.h"
#include "utils/misc.h"

#include <assert.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/dlist.h>

#include <caniot/caniot.h>
#include <caniot/controller.h>
#include <caniot/datatype.h>
LOG_MODULE_REGISTER(caniot, LOG_LEVEL_WRN);

#define HA_CIOT_QUERY_TIMEOUT_TOLERANCE_MS 200u

#if defined(CONFIG_APP_CAN_INTERFACE)
CAN_MSGQ_DEFINE(can_rxq, 4U);
#endif

/* initialized with 0 */
static ha_ciot_ctrl_did_cb_t did_callbacks[CANIOT_DID_MAX_VALUE];

// CONFIG CAN_INTERFACE
// CONFIG_APP_HA_EMULATED_DEVICES

static void thread(void *_a, void *_b, void *_c);

#define HA_CANIOT_CTRL_THREAD_STACK_SIZE 0x400

K_THREAD_DEFINE(ha_caniot_ctrl_thread,
		HA_CANIOT_CTRL_THREAD_STACK_SIZE,
		thread,
		NULL,
		NULL,
		NULL,
		K_PRIO_COOP(2),
		0U,
		0U);

static int z_can_send(const struct caniot_frame *frame, uint32_t delay_ms)
{
	int ret;

	__ASSERT(frame != NULL, "frame is NULL");

#if defined(CONFIG_APP_CAN_INTERFACE)
	struct can_frame zframe;
	caniot_to_zcan(&zframe, frame);
	ret = if_can_send(CAN_BUS_CANIOT, &zframe);
	if (ret) goto exit;
#endif

#if defined(CONFIG_APP_HA_EMULATED_DEVICES)
	ret = emu_caniot_send((struct caniot_frame *)frame);
	if (ret) goto exit;
#endif

exit:
	return ret;
}

const struct caniot_drivers_api driv = {
	.entropy  = NULL,
	.get_time = NULL,
	.set_time = NULL,
	.recv	  = NULL,
	.send	  = z_can_send,
};

static struct caniot_controller ctrl;

K_FIFO_DEFINE(fifo_queries);

typedef enum {
	SYNCQ_IMMEDIATE,	   /* Query but returned immediately (as timeout was 0) */
	SYNCQ_ANSWERED,		   /* Query answered with a valid response */
	SYNCQ_ANSWERED_WITH_ERROR, /* Query answered with a CANIOT error */
	SYNCQ_TIMEOUT,		   /* Query timed out */
	SYNCQ_CANCELLED,	   /* Query cancelled */
	SYNCQ_ERROR,		   /* Error during querying */
} syncq_status_t;

// convert syncq_status_t to string
static const char *syncq_status_to_str(syncq_status_t event)
{
	switch (event) {
	case SYNCQ_IMMEDIATE:
		return "SYNCQ_IMMEDIATE";
	case SYNCQ_ANSWERED:
		return "SYNCQ_ANSWERED";
	case SYNCQ_ANSWERED_WITH_ERROR:
		return "SYNCQ_ANSWERED_WITH_ERROR";
	case SYNCQ_TIMEOUT:
		return "SYNCQ_TIMEOUT";
	case SYNCQ_CANCELLED:
		return "SYNCQ_CANCELLED";
	case SYNCQ_ERROR:
		return "SYNCQ_ERROR";
	default:
		return "<unknown syncq_status_t>";
	}
}

struct syncq {
	union {
		void *_tie; /* for k_fifo_put */
		sys_dnode_t _node;
	};

	struct k_sem _sem;

	/* Query status */
	syncq_status_t status;

	/* error in case caniot_controller_query() returned immediately
	 */
	int query_error;

	/* query queued to be answered */
	struct caniot_frame *query;

	/* buffer which will contain the response on success */
	struct caniot_frame *response;

	/* query timeout */
	uint32_t timeout;

	/* TODO union { */

	/* handle of the query when queued */
	uint8_t handle;

	/* query pending on this device */
	caniot_did_t did;

	/* }; */

	union {
		/* contain update on creation */
		uint32_t uptime;

		/* Duration if answered (or timeout) */
		uint32_t delta;
	};
};

K_MEM_SLAB_DEFINE(sq_pool, sizeof(struct syncq), CONFIG_CANIOT_MAX_PENDING_QUERIES, 4U);

/* requires ~80B of stack */
void log_caniot_frame(const struct caniot_frame *frame)
{
	char repr[64];
	int ret = caniot_explain_frame_str(frame, repr, sizeof(repr));
	if (ret > 0) {
		LOG_INF("%s", repr);
	} else {
		LOG_WRN("Failed to encode frame, ret = %d", ret);
	}
}

bool event_cb(const caniot_controller_event_t *ev, void *user_data)
{
	int ret;

	bool response_is_set = false;

	switch (ev->status) {
	case CANIOT_CONTROLLER_EVENT_STATUS_OK: {
		const struct caniot_frame *resp = ev->response;
		__ASSERT(resp != NULL, "response is NULL");

		response_is_set = true;

		if (resp->id.type == CANIOT_FRAME_TYPE_TELEMETRY) {
			if (resp->len != 8U) {
				LOG_WRN("Expected length for board control "
					"telemetry is 8, got %d",
					resp->len);
			}

			ret = ha_dev_register_caniot_telemetry(
				net_time_get(),
				CANIOT_DID(resp->id.cls, resp->id.sid),
				(char *)resp->buf,
				(caniot_id_t *)&resp->id);
		}
	} break;
	case CANIOT_CONTROLLER_EVENT_STATUS_ERROR:
		response_is_set = true;
		break;
	case CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT:
		break;
	case CANIOT_CONTROLLER_EVENT_STATUS_CANCELLED:
		break;
	default:
		break;
	}

	/* call associated general callback */
	if ((response_is_set == true) && (did_callbacks[ev->did] != NULL)) {
		__ASSERT(ev->response != NULL, "response is NULL");

		LOG_DBG("callback set for did %d : %p", ev->did, did_callbacks[ev->did]);

		/* implement a user_data mechanism to pass the context for
		 * example*/
		did_callbacks[ev->did](ev->did, ev->response, NULL);
	}

	struct syncq *const qx = ev->user_data;
	if ((ev->context == CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY) && (qx != NULL)) {
		LOG_DBG("Query %p answered", qx);

		switch (ev->status) {
		case CANIOT_CONTROLLER_EVENT_STATUS_OK:
			qx->status = SYNCQ_ANSWERED;
			break;
		case CANIOT_CONTROLLER_EVENT_STATUS_ERROR:
			qx->status = SYNCQ_ANSWERED_WITH_ERROR;
			break;
		case CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT:
			qx->status = SYNCQ_TIMEOUT;
			break;
		case CANIOT_CONTROLLER_EVENT_STATUS_CANCELLED:
			qx->status = SYNCQ_CANCELLED;
			break;
		}

		if (response_is_set == true) {
			memcpy(qx->response, ev->response, sizeof(struct caniot_frame));
		}

		qx->delta = k_uptime_delta32(&qx->uptime);
		k_sem_give(&qx->_sem);
	}

	return true;
}

typedef struct {
	struct k_poll_event query;
#if defined(CONFIG_APP_CAN_INTERFACE)
	struct k_poll_event can;
#endif
#if defined(CONFIG_APP_HA_EMULATED_DEVICES)
	struct k_poll_event can_emu;
#endif
} kpoll_can_events_t;

#define KPOLL_CAN_EVENTS_COUNT (sizeof(kpoll_can_events_t) / sizeof(struct k_poll_event))

static void thread(void *_a, void *_b, void *_c)
{
	ARG_UNUSED(_a);
	ARG_UNUSED(_b);
	ARG_UNUSED(_c);

	int ret;
	static struct caniot_frame frame;

#if defined(CONFIG_APP_CAN_INTERFACE)
	static struct can_frame zframe;

	struct can_filter filter = {.id_type = CAN_ID_STD};

	ret = if_can_attach_rx_msgq(CAN_BUS_CANIOT, &can_rxq, &filter);
	if (ret < 0) {
		LOG_ERR("Failed to attach CAN RX queue: %d", ret);
		return;
	}
#endif

	caniot_controller_driv_init(&ctrl, &driv, event_cb, NULL);

	kpoll_can_events_t events =
	{.query = K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						  K_POLL_MODE_NOTIFY_ONLY,
						  &fifo_queries,
						  0),
#if defined(CONFIG_APP_CAN_INTERFACE)
	 .can = K_POLL_EVENT_STATIC_INITIALIZER(
		 K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &can_rxq, 0),
#endif
#if defined(CONFIG_APP_HA_EMULATED_DEVICES)
	 .can_emu = K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
						    K_POLL_MODE_NOTIFY_ONLY,
						    &emu_caniot_rxq,
						    0),
#endif
	};

	uint32_t reftime = k_uptime_get_32();

	while (1) {
		const uint32_t timeout_ms = caniot_controller_next_timeout(&ctrl);
		LOG_DBG("k_poll(., %u, %u)", KPOLL_CAN_EVENTS_COUNT, timeout_ms);
		ret = k_poll((struct k_poll_event *)&events,
			     KPOLL_CAN_EVENTS_COUNT,
			     K_MSEC(timeout_ms));
		if (ret == 0) {
			struct caniot_frame *resp = NULL;

#if defined(CONFIG_APP_CAN_INTERFACE)
			/* events */
			if (!resp &&
			    (events.can.state == K_POLL_STATE_MSGQ_DATA_AVAILABLE)) {
				ret = k_msgq_get(&can_rxq, &zframe, K_NO_WAIT);
				if (ret == 0) {
					zcan_to_caniot(&zframe, &frame);
					log_caniot_frame(&frame);
					resp = &frame;
				}
				events.can.state = K_POLL_STATE_NOT_READY;
			}
#endif

#if defined(CONFIG_APP_HA_EMULATED_DEVICES)
			/* "!resp" is to be sure to process a single frame at a
			 * time */
			if (!resp &&
			    (events.can_emu.state == K_POLL_STATE_MSGQ_DATA_AVAILABLE)) {
				ret = k_msgq_get(&emu_caniot_rxq, &frame, K_NO_WAIT);
				if (ret == 0) {
					log_caniot_frame(&frame);
					resp = &frame;
				}
				events.can_emu.state = K_POLL_STATE_NOT_READY;
			}
#endif

			/* we need to process the response before sending a
			 * query, otherwise the query could timeout immediately
			 * because the timeout queue was not shifted in time
			 */
			const uint32_t delta = k_uptime_delta32(&reftime);
			caniot_controller_process_single(&ctrl, delta, resp);

			struct syncq *qx;
			if ((events.query.state == K_POLL_STATE_FIFO_DATA_AVAILABLE) &&
			    ((qx = k_fifo_get(&fifo_queries, K_NO_WAIT)) != NULL)) {

				__ASSERT_NO_MSG(qx->timeout != CANIOT_TIMEOUT_FOREVER);

				ret = caniot_controller_query(
					&ctrl, qx->did, qx->query, qx->timeout);
				log_caniot_frame(qx->query);

				if (ret > 0) {
					/* pending query registered */
					qx->handle = (uint8_t)ret;
					caniot_controller_handle_user_data_set(
						&ctrl, qx->handle, qx);
				} else {
					/* no context allocated, return
					 * immediately */
					if (ret == 0) {
						/* Query sent but returned
						 * immediately as timeout is
						 * null */
						qx->status = SYNCQ_IMMEDIATE;
					} else {
						/* Error */
						qx->status	= SYNCQ_ERROR;
						qx->query_error = ret;
					}

					qx->delta = k_uptime_delta32(&qx->uptime);
					k_sem_give(&qx->_sem);
				}

				events.query.state = K_POLL_STATE_NOT_READY;
			}
		} else if (ret == -EAGAIN) { /* k_poll timed out */
			const uint32_t delta = k_uptime_delta32(&reftime);
			caniot_controller_process_single(&ctrl, delta, NULL);
		} else {
			LOG_ERR("k_poll failed: %d", ret);
			break;
		}
	}
}

int ha_caniot_controller_query(struct caniot_frame *__restrict req,
		       struct caniot_frame *__restrict resp,
		       caniot_did_t did,
		       uint32_t *timeout)
{
	int ret		 = -EINVAL;
	struct syncq *qx = NULL;

	if (!req || !resp || !timeout) {
		goto exit;
	}

	/* forever wait not supported */
	if (*timeout == CANIOT_TIMEOUT_FOREVER) {
		goto exit;
	} else if (*timeout == 0) {
		LOG_WRN("Timeout=%d not recommended, use ha_caniot_controller_send() "
			"instead",
			0);
	}

	/* alloc and prepare */
	ret = k_mem_slab_alloc(&sq_pool, (void **)&qx, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("k_mem_slab_alloc() failed: %d", ret);
		goto exit;
	}

	k_sem_init(&qx->_sem, 0, 1);
	qx->did		= did;
	qx->query	= req;
	qx->response	= resp;
	qx->timeout	= *timeout;
	qx->uptime	= k_uptime_get_32();
	qx->query_error = 0;

	/* queue synchronous query */
	k_fifo_put(&fifo_queries, qx);

	/* wait for response */
	ret = k_sem_take(&qx->_sem,
			 K_MSEC(qx->timeout + HA_CIOT_QUERY_TIMEOUT_TOLERANCE_MS));
	if (ret != 0) {
		LOG_ERR("k_sem_take shouldn't timeout ret=%d", ret);
		ret = -EAGAIN;
		goto exit;
	}

	switch (qx->status) {
	case SYNCQ_IMMEDIATE:
		ret	  = 0;
		qx->delta = 0; /* no delta */
		break;
	case SYNCQ_ANSWERED:
		ret = 1;
		break;
	case SYNCQ_ANSWERED_WITH_ERROR:
		ret = 2;
		break;
	case SYNCQ_TIMEOUT:
		ret = -EAGAIN;
		break;
	case SYNCQ_ERROR:
		ret = qx->query_error;
		break;
	case SYNCQ_CANCELLED:
		ret = -ECANCELED;
		break;
	default:
		LOG_ERR("Unhandled error ret=%d", ret);
		ret = -1; /* any unhandled error */
		break;
	}

	*timeout = qx->delta; /* actual time the query took */

	LOG_DBG("Sync query completed in %u ms with status %s (ret = %d)",
		qx->delta,
		syncq_status_to_str(qx->status),
		ret);

exit:
	/* cleanup */
	if (qx != NULL) {
		k_mem_slab_free(&sq_pool, (void **)&qx);
	}

	return ret;
}

int ha_caniot_controller_send(struct caniot_frame *__restrict req, caniot_did_t did)
{
	/* this is safe because no context is allocated */
	return caniot_controller_send(&ctrl, did, req);
}

int ha_controller_caniot_discover(uint32_t timeout, ha_ciot_ctrl_did_cb_t cb)
{
	return -ENOTSUP;
}