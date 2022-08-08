/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "event.h"

#define HA_EV_TRIG_TABLE_SIZE 8u

static ha_ev_trig_t trig_table[HA_EV_TRIG_TABLE_SIZE];

void trig_table_init(void)
{
	/* Create free list inside of the table */

}

ha_ev_trig_t *trig_table_alloc(void)
{

}