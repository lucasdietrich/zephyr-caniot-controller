#include "ha/config.h"
#include "ha.h"

#include "ha/devices/all.h"

struct ha_room ha_cfg_rooms[] = {
	HA_ROOM(HA_ROOM_NONE, "<no room>"),
	HA_ROOM(HA_ROOM_MY, "Ma chambre"),
	HA_ROOM(HA_ROOM_PARENTS, "Chambre des parents"),
};

const size_t ha_cfg_rooms_count = ARRAY_SIZE(ha_cfg_rooms);

struct ha_room_assoc ha_cfg_rooms_assoc[] = {
	HA_ROOM_ASSOC(HA_ROOM_MY, HA_DEV_XIAOMI_ADDR_INIT(0xECu, 0x1Cu, 0x6Du)),
	HA_ROOM_ASSOC(HA_ROOM_MY, HA_DEV_CANIOT_ADDR_INIT(32u)),

	HA_ROOM_ASSOC(HA_ROOM_NONE, HA_DEV_XIAOMI_ADDR_INIT(0x68u, 0x05u, 0x63u)),
	HA_ROOM_ASSOC(HA_ROOM_NONE, HA_DEV_XIAOMI_ADDR_INIT(0x0Au, 0x1Eu, 0x38u)),
	HA_ROOM_ASSOC(HA_ROOM_NONE, HA_DEV_XIAOMI_ADDR_INIT(0xA7u, 0x30u, 0xC4u)),
	HA_ROOM_ASSOC(HA_ROOM_NONE, HA_DEV_XIAOMI_ADDR_INIT(0xD5u, 0x08u, 0x40u)),
	HA_ROOM_ASSOC(HA_ROOM_NONE, HA_DEV_XIAOMI_ADDR_INIT(0x28u, 0x17u, 0xE3u)),
	HA_ROOM_ASSOC(HA_ROOM_NONE, HA_DEV_XIAOMI_ADDR_INIT(0xE0u, 0x18u, 0xEDu)),
	HA_ROOM_ASSOC(HA_ROOM_NONE, HA_DEV_XIAOMI_ADDR_INIT(0x8Du, 0xBAu, 0xB4u)),	
};
const size_t ha_cfg_rooms_assoc_count = ARRAY_SIZE(ha_cfg_rooms_assoc);