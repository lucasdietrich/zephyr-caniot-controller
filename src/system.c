/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system.h"

#include "userio/userio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(system, LOG_LEVEL_DBG);

static uint8_t iface_state[_SYSEV_IF_COUNT] =
{
	SYSEV_IF_DOWN,
	SYSEV_IF_DOWN,
	SYSEV_IF_DOWN
};

void sysev_notify(sysev_if_t iface, sysev_if_ev_t ev, void *data)
{
	/* If state is the same, do nothing */
	if (iface_state[iface] == ev) {
		return;
	}

	LOG_DBG("iface %s received event %s",
		sysev_iface_to_str(iface),
		sysev_iface_ev_to_str(ev));

	switch (ev) {
	case SYSEV_IF_UP:
	case SYSEV_IF_DOWN:
	case SYSEV_IF_FATAL_ERROR:
	case SYSEV_IF_PENDING:
		iface_state[iface] = ev;
		break;

	/* RX/TX event is not persistent */
	case SYSEV_IF_RX_TX:
		break;
	default:
		break;
	}

#if defined(CONFIG_APP_USERIO)
	userio_iface_show_event(iface, ev);
#endif
}

sysev_if_ev_t sysev_iface_get_state(sysev_if_t iface)
{
	return iface_state[iface];
}

const char *sysev_iface_to_str(sysev_if_t iface)
{
	switch (iface) {
	case SYSEV_IF_NET:
		return "NET";
	case SYSEV_IF_CAN:
		return "CAN";
	case SYSEV_IF_BLE:
		return "BLE";
	default:
		return "<unknown sysev_if_t>";
	}
}

const char *sysev_iface_ev_to_str(sysev_if_ev_t ev)
{
	switch (ev) {
	case SYSEV_IF_UP:
		return "UP";
	case SYSEV_IF_DOWN:
		return "DOWN";
	case SYSEV_IF_FATAL_ERROR:
		return "FATAL_ERROR";
	case SYSEV_IF_PENDING:
		return "PENDING";
	case SYSEV_IF_RX_TX:
		return "RX/TX";
	default:
		return "<unknown sysev_if_ev_t>";
	}
}