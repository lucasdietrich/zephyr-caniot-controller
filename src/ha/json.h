#ifndef _HA_JSON_H_
#define _HA_JSON_H_

#include "utils/buffers.h"

struct json_device_base {
	// const char *device_name;

	// const char *datetime;
	// int32_t rel_time;

	uint32_t timestamp;
};

struct json_xiaomi_record_measures {
	int32_t rssi;

	char *temperature;		  /* °C */
	int32_t temperature_raw;  /* 1e-2 °C */
	uint32_t humidity;		  /* 1e-2 % */
	uint32_t battery_level;	  /* % */
	uint32_t battery_voltage; /* mV */
};

struct json_xiaomi_record {
	char *bt_mac;

	struct json_device_base base;
	struct json_xiaomi_record_measures measures;
};

struct json_xiaomi_record_buf {
	char addr[BT_ADDR_LE_STR_LEN];
	char temperature[9u];
};

struct json_xiaomi_record_array {
	struct json_xiaomi_record_buf _bufs[HA_XIAOMI_MAX_DEVICES];
	struct json_xiaomi_record records[HA_XIAOMI_MAX_DEVICES];
	size_t count;
};

#endif /* _HA_JSON_H_ */