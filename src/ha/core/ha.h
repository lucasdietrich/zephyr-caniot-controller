/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_H_
#define _HA_H_

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/addr.h>

#if defined(CONFIG_CANIOT_LIB)
#include <caniot/caniot.h>
#include <caniot/datatype.h>
#endif

#include "ha/core/devices.h"
#include "ha/core/data.h"

#if defined(CONFIG_APP_HA_EMULATED_DEVICES)
#	define HA_CANIOT_MAX_DEVICES 63u
#	define HA_XIAOMI_MAX_DEVICES 40u
#	define HA_OTHER_MAX_DEVICES 0u
#	define HA_EV_SUBS_MAX_COUNT 8u
#	define HA_EV_MAX_COUNT 128u
#else
#	define HA_CANIOT_MAX_DEVICES 5U
#	define HA_XIAOMI_MAX_DEVICES 15U
#	define HA_OTHER_MAX_DEVICES 5U
#	define HA_EV_SUBS_MAX_COUNT 8u
#	define HA_EV_MAX_COUNT 32u
#endif

#if !defined(CONFIG_CANIOT_LIB)
#	undef HA_CANIOT_MAX_DEVICES
#	define HA_CANIOT_MAX_DEVICES 0U
#endif

#define HA_MAX_DEVICES (HA_CANIOT_MAX_DEVICES + HA_XIAOMI_MAX_DEVICES + HA_OTHER_MAX_DEVICES)

#define HA_DEV_ADDR_STR_MAX_LEN MAX(BT_ADDR_LE_STR_LEN, sizeof("0x1FFFFFFF"))
#define HA_DEV_ADDR_TYPE_STR_MAX_LEN 16u
#define HA_DEV_ADDR_MEDIUM_STR_MAX_LEN 10u

// TODO move to CANIOT library
#define HA_CANIOT_CLS0_MAX_TEMPERATURES 		4u
#define HA_CANIOT_CLS1_MAX_TEMPERATURES 		4u
#define HA_CANIOT_MAX_TEMPERATURES 4U

#endif /* _HA_H_ */