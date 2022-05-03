#ifndef _HA_ROOM_H_
#define _HA_ROOM_H_

#include <stdint.h>

#include "devices.h"

struct ha_room
{
	uint8_t index;
	const char *name;

	ha_dev_type_t type;
	ha_dev_addr_t addr;
};

#endif