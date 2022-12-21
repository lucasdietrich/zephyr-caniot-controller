/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_DEVICES_F429ZI_H
#define _HA_DEVICES_F429ZI_H

#include "ha/core/ha.h"

struct ha_ds_f429zi
{
	struct ha_data_temperature die_temperature;
};

int ha_dev_register_die_temperature(uint32_t timestamp, float die_temperature);

float ha_ev_get_die_temperature(const ha_ev_t *ev);

#endif /* _HA_DEVICES_F429ZI_H */