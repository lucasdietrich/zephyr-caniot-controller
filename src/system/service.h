/**
 * @file configuration.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-03-10
 * 
 * @copyright Copyright (c) 2022
 * 
 * System services and persistent configuration
 * 
 * TODO:
 * 	- Persistent configuration
 * 	- Create struct for holding runtime information (like started or stopped ...)
 * 
 * # Use:
 * https://docs.zephyrproject.org/latest/reference/iterable_sections/index.html
 */

#ifndef _SYSTEM_CONFIGURATION_H_
#define _SYSTEM_CONFIGURATION_H_


#include <zephyr.h>

typedef enum {
	SERVICE_UNKNOWN = 0x00U,

	SERVICE_SNTP = 0x10U,
	SERVICE_DHCP,
	SERVICE_DNS,

	SERVICE_AWS_MQTT = 0x20U,
	SERVICE_HTTP_SERVER,
	SERVICE_CANTCP_SERVER,
	SERVICE_WEB_SERVER,

	SERVICE_HISTORICAL_DATA = 0x30U,

	SERVICE_USER_MANAGEMENT = 0x40U,
	SERVICE_SERVICE_MANAGEMENT
} system_service_id_t;

struct system_service_api
{
	int (*on_start)(void);
	int (*on_stop)(void);

	int (*on_restart)(void);
	int (*on_reconfigure)(void);
};

struct system_service
{
	const char *name;
	struct {
		uint32_t id: 8;
		uint32_t enabled: 1;
	};

	struct system_service_api api;
};

#define DEFINE_SERVICE(service_name, service_id, start, stop, restart, reconfigure) \
	STRUCT_SECTION_ITERABLE(system_service, service_name) = { \
		.name = STRINGIFY(service_name), \
		.id = service_id, \
		.enabled = 1, \
		.api = { \
			.on_start = start, \
			.on_stop = stop, \
			.on_restart = restart, \
			.on_reconfigure = reconfigure, \
		} \
	}

/**
 * @brief Start enabled services
 */
void system_services_start_ready(void);

int system_start_service(system_service_id_t id);

int system_stop_service(system_service_id_t id);

int system_restart_service(system_service_id_t id);

// void system_register_service(struct system_service *service);

#endif /* _SYSTEM_CONFIGURATION_H_ */