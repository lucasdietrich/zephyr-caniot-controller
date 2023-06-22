#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(debug_server, LOG_LEVEL_DBG);

#include "core/http_headers.h"
#include "debug_server.h"

#include <ha/core/ha.h>

#if defined(CONFIG_APP_HA)

bool ha_it_cb(ha_dev_t *dev, void *user_data)
{
	struct ha_event *ev;
	struct ha_device_endpoint *ep;

	for (uint32_t ep_index = 0u; (ep = ha_dev_ep_get(dev, ep_index)); ep_index++) {
		LOG_DBG("dev: %d ep[%u]", dev->sdevuid, ep_index);
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