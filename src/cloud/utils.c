/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"

const char *mqtt_evt_get_str(enum mqtt_evt_type evt_type)
{
	static const char *mqtt_evt_str[] = {
		"MQTT_EVT_CONNACK",
		"MQTT_EVT_DISCONNECT",
		"MQTT_EVT_PUBLISH",
		"MQTT_EVT_PUBACK",
		"MQTT_EVT_PUBREC",
		"MQTT_EVT_PUBREL",
		"MQTT_EVT_PUBCOMP",
		"MQTT_EVT_SUBACK",
		"MQTT_EVT_UNSUBACK",
		"MQTT_EVT_PINGRESP",
		"<UNKNOWN mqtt_evt_type>"
	};

	return mqtt_evt_str[MIN(evt_type, ARRAY_SIZE(mqtt_evt_str) - 1)];
}