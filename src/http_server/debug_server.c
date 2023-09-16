#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(debug_server, LOG_LEVEL_DBG);

#include "core/http_headers.h"
#include "debug_server.h"

#include <ha/core/ha.h>

#if defined(CONFIG_APP_HA)

bool ha_data_iter_cb(const ha_data_storage_t *data, void *user_data)
{
	char buf[64u];
	ha_data_encode_value(buf, sizeof(buf), data->type, &data->value);

	LOG_DBG("[%s/%s] occ: %u (size=%u): %s", ha_data_subsystem_to_str(data->subsys),
			ha_data_type_to_str(data->type), data->occurence,
			ha_data_type_get_value_size(data->type), buf);

	return true;
}

bool ha_it_cb(ha_dev_t *dev, void *user_data)
{
	struct ha_device_endpoint *ep;
	struct ha_event *ev;

	for (uint32_t ep_index = 0u; (ep = ha_dev_ep_get(dev, ep_index)); ep_index++) {

		if (ep->last_data_event != NULL) {
			ha_ev_data_iterate(ep->last_data_event, ha_data_iter_cb, NULL);
		}
	}

	return true;
}

int debug_server_ha_telemetry(http_request_t *request, http_response_t *response)
{
	int ret;

	snprintf(response->buffer.data, response->buffer.size, "<b>Hello World!</b>");
	response->buffer.filling = strlen(response->buffer.data);

	const ha_dev_filter_t filter = {.flags = HA_DEV_FILTER_DATA_EXIST};

	ret = ha_dev_iterate(ha_it_cb, &filter, &HA_DEV_ITER_OPT_LOCK_ALL(), NULL);

	return 0;
}

#endif