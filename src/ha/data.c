#include <zephyr.h>

#include "data.h"

#include "devices.h"

static size_t get_data_size(ha_data_type_t type)
{
	switch (type) {
	case HA_DATA_TEMPERATURE:
		return sizeof(struct ha_data_temperature);
	case HA_DATA_HUMIDITY:
		return sizeof(struct ha_data_humidity);
	case HA_DATA_BATTERY_LEVEL:
		return sizeof(struct ha_data_battery_level);
	case HA_DATA_RSSI:
		return sizeof(struct ha_data_rssi);
	case HA_DATA_DIGITAL:
		return sizeof(struct ha_data_digital);
	case HA_DATA_ANALOG:
		return sizeof(struct ha_data_analog);
	default:
		return 0;
	}
}

ha_data_t *ha_data_alloc(ha_data_type_t type)
{
	const size_t data_size = get_data_size(type);
	if (data_size == 0) {
		return NULL;
	}

	ha_data_t *container = NULL;

	container = k_malloc(sizeof(ha_data_t));
	if (!container) {
		goto error;
	}

	container->data = k_malloc(data_size);
	if (!container->data) {
		goto error;
	}

	return container;

error:
	if (container) {
		k_free(container);

		if (container->data) {
			k_free(container->data);
		}
	}

	return NULL;
}

void ha_data_free(ha_data_t *data)
{
	k_free(data);
}

void ha_data_append(sys_slist_t *list, ha_data_t *data)
{
	sys_slist_append(list, &data->_node);
}

void ha_data_free_list(sys_slist_t *list)
{
	sys_snode_t *node;

	while ((node = sys_slist_get(list)) != NULL) {
		ha_data_free(CONTAINER_OF(node, ha_data_t, _node));
	}
}

// ha_data_t *ha_ev_data_alloc_queue(struct ha_event *ev,
// 				  ha_data_type_t type)
// {
// 	if (!ev) {
// 		return -EINVAL;
// 	}

// 	ha_data_t *mem = ha_data_alloc(type);
// 	if (!mem) {
// 		return -ENOMEM;
// 	}

// 	sys_slist_append(&ev->slist, &mem->_node);

// 	return 0;
// }