#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(debug_server, LOG_LEVEL_DBG);

#include "debug_server.h"

#include "core/http_headers.h"

#if defined(CONFIG_APP_HA)

int debug_server_ha_telemetry(http_request_t *request, http_response_t *response)
{
	snprintf(response->buffer.data, response->buffer.size, "<b>Hello World!</b>");
	response->buffer.filling = strlen(response->buffer.data);

	return 1;
}

#endif