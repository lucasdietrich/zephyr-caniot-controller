#include "service.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(service, LOG_LEVEL_DBG);

DEFINE_SERVICE(system_service, SERVICE_SERVICE_MANAGEMENT, NULL, NULL, NULL, NULL);

void system_services_start_ready(void)
{
	k_sched_lock();

	STRUCT_SECTION_FOREACH(system_service, service) {
		if (service->enabled) {
			if (service->api.on_start != 0) {
				LOG_INF("Starting service %s", service->name);

				service->api.on_start();
			} else {
				LOG_WRN("Cannot start enabled service '%s' "
					"(missing on_start function)", service->name);
			}
		} else {
			LOG_DBG("Service %s", service->name);
		}
	}

	k_sched_unlock();
}

static struct system_service* get_service_by_id(system_service_id_t id)
{
	struct system_service *service = NULL;

	STRUCT_SECTION_FOREACH(system_service, s) {
		if (s->id == id) {
			service = s;
		}
	}

	return service;
}

int system_start_service(system_service_id_t id)
{
	struct system_service *service = get_service_by_id(id);get_service_by_id(id);

	if (service && service->api.on_start) {
		return service->api.on_start();
	}

	return -EINVAL;
}

int system_stop_service(system_service_id_t id)
{
	struct system_service *service = get_service_by_id(id);

	if (service && service->api.on_stop) {
		return service->api.on_stop();
	}

	return -EINVAL;
}

int system_restart_service(system_service_id_t id)
{
	struct system_service *service = get_service_by_id(id);

	if (service && service->api.on_restart) {
		return service->api.on_restart();
	}

	return -EINVAL;
}