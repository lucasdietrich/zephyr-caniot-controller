/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_DEVICES_F429ZI_H
#define _HA_DEVICES_F429ZI_H

#include "../ha.h"
#include "../devices.h"

struct ha_f429zi_dataset
{
	float die_temperature; /* °C */
};

int ha_dev_register_die_temperature(uint32_t timestamp, float die_temperature);

float ha_ev_get_die_temperature(const ha_ev_t *ev);

#endif /* _HA_DEVICES_F429ZI_H */