/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HA_CONFIG_H_
#define _HA_CONFIG_H_

#include "ha/core/ha.h"
#include "ha/core/room.h"

#define HA_ROOM_MY	HA_ROOM_LOFT_BEDROOM
#define HA_ROOM_PARENTS HA_ROOM_BEDROOM_1

extern struct ha_room ha_cfg_rooms[];
extern const size_t ha_cfg_rooms_count;

extern struct ha_room_assoc ha_cfg_rooms_assoc[];
extern const size_t ha_cfg_rooms_assoc_count;

#endif /* _HA_CONFIG_H_ */