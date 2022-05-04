#include "room.h"

#include <zephyr.h>

#include <bluetooth/addr.h>


struct room_dev_assoc
{	
	room_id_t room;
	ha_dev_addr_t addr;
};

#define XIAOMI_BT_LE_ADDR_0 0xA4U
#define XIAOMI_BT_LE_ADDR_1 0xC1U
#define XIAOMI_BT_LE_ADDR_2 0x38U


#define MIJIA_ROOM(rid, bt3, bt4, bt5) \
{ \
	.room = rid, \
	.addr = { \
		.type = HA_DEV_TYPE_XIAOMI_MIJIA, \
		.mac = { \
			.medium = HA_DEV_MEDIUM_BLE, \
			.addr = { \
				.ble = { \
					.type = BT_ADDR_LE_PUBLIC, \
					.a = { \
						.val = { \
							XIAOMI_BT_LE_ADDR_0, \
							XIAOMI_BT_LE_ADDR_1, \
							XIAOMI_BT_LE_ADDR_2, \
							bt3, \
							bt4, \
							bt5, \
						} \
					} \
				} \
			} \
		} \
	} \
}

static const struct room_dev_assoc assocs[] = {
	MIJIA_ROOM(HA_ROOM_ENTRANCE, 0x0A, 0x1E, 0x38),
};

static const struct room_dev_assoc *ha_get_device_assoc(ha_dev_t *const dev)
{
	const struct room_dev_assoc *assoc;

	for (assoc = assocs; assoc < assocs + ARRAY_SIZE(assocs); assoc++) {
		if (ha_dev_addr_cmp(&assoc->addr, &dev->addr)) {
			return assoc;
		}
	}

	return NULL;
}

room_id_t ha_get_device_room(ha_dev_t *const dev)
{
	room_id_t rid = HA_ROOM_NONE;
	const struct room_dev_assoc *assoc = ha_get_device_assoc(dev);

	if (assoc != NULL) {
		rid = assoc->room;
	}

	return rid;
}
