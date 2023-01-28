/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "appfs.h"
#include "can/can_interface.h"
#include "config.h"
#include "creds/flash_creds.h"
#include "creds/manager.h"
#include "creds/utils.h"
#include "ha/caniot_controller.h"
#include "ha/core/config.h"
#include "ha/core/ha.h"
#include "ha/core/utils.h"
#include "ha/devices/all.h"
#include "ha/json.h"
#include "lua/orchestrator.h"
#include "net_interface.h"
#include "net_time.h"
#include "rest_server.h"
#include "system.h"
#include "utils/buffers.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/http_parser.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/posix/time.h>

#include <caniot/caniot.h>
#include <caniot/classes/class0.h>
#include <caniot/classes/class1.h>
#include <caniot/datatype.h>
#include <embedc-url/parser.h>
#include <mbedtls/memory_buffer_alloc.h>
LOG_MODULE_REGISTER(rest_server, LOG_LEVEL_INF);

#define REST_CANIOT_QUERY_MAX_TIMEOUT_MS (1000u)

#define REST_HA_DEVICES_MAX_COUNT_PER_PAGE 10u
#define JSON_HA_MAX_DEVICES		   MIN(HA_DEVICES_MAX_COUNT, REST_HA_DEVICES_MAX_COUNT_PER_PAGE)

#define FIELD_SET(ret, n) (((ret) & (1 << (n))) != 0)

#define route_arg_get_by_index http_req_route_arg_get_number_by_index
#define route_arg_get	       http_req_route_arg_get

int rest_encode_response_json(http_response_t *resp,
			      const void *val,
			      const struct json_obj_descr *descr,
			      size_t descr_len)
{
	int ret = -EINVAL;
	ssize_t json_len;

	if (!resp || !resp->buffer.data || !http_code_has_payload(resp->status_code)) {
		LOG_WRN("unexpected payload for code %d or buffer not set !",
			resp->status_code);
		goto exit;
	}

	/* calculate needed size to encode the response */
	json_len = json_calc_encoded_len(descr, descr_len, val);

	if (json_len <= resp->buffer.size) {
		ret = json_obj_encode_buf(
			descr, descr_len, val, resp->buffer.data, resp->buffer.size);
		resp->buffer.filling = json_len;

		http_response_set_content_length(resp, json_len);
	}

exit:
	return ret;
}

int rest_encode_response_json_array(http_response_t *resp,
				    const void *val,
				    const struct json_obj_descr *descr)
{
	int ret = -EINVAL;

	if (!resp || !resp->buffer.data || !http_code_has_payload(resp->status_code)) {
		LOG_WRN("unexpected payload for code %d or buffer not set !",
			resp->status_code);
		goto exit;
	}

	ret = json_arr_encode_buf(descr, val, resp->buffer.data, resp->buffer.size);

	if (ret == 0) {
		resp->buffer.filling = strlen(resp->buffer.data);

		http_response_set_content_length(resp, resp->buffer.filling);
	}

exit:
	return ret;
}

int rest_index(http_request_t *req, http_response_t *resp)
{
	resp->status_code    = 200;
	resp->buffer.filling = 0;

	return 0;
}

struct json_info_controller_status {
	bool has_ipv4_addr;
	bool valid_system_time;
};

static const struct json_obj_descr info_controller_status_descr[] = {
	JSON_OBJ_DESCR_PRIM(
		struct json_info_controller_status, has_ipv4_addr, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(
		struct json_info_controller_status, valid_system_time, JSON_TOK_TRUE),
};

static const struct json_obj_descr net_stats_bytes_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct net_stats_bytes, sent, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_bytes, received, JSON_TOK_NUMBER),
};

static const struct json_obj_descr net_stats_ip_errors_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct net_stats_ip_errors, vhlerr, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_ip_errors, hblenerr, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_ip_errors, lblenerr, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_ip_errors, fragerr, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_ip_errors, chkerr, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_ip_errors, protoerr, JSON_TOK_NUMBER),
};

static const struct json_obj_descr net_stats_ip_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct net_stats_ip, recv, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_ip, sent, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_ip, forwarded, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_ip, drop, JSON_TOK_NUMBER),
};

static const struct json_obj_descr net_stats_icmp_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct net_stats_icmp, recv, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_icmp, sent, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_icmp, drop, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_icmp, typeerr, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_icmp, chkerr, JSON_TOK_NUMBER),
};

static const struct json_obj_descr net_stats_tcp_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct net_stats_tcp, bytes, net_stats_bytes_descr),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, resent, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, drop, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, recv, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, sent, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, seg_drop, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, chkerr, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, ackerr, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, rsterr, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, rst, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, rexmit, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, conndrop, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_tcp, connrst, JSON_TOK_NUMBER),
};

static const struct json_obj_descr net_stats_udp_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct net_stats_udp, drop, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_udp, recv, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_udp, sent, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct net_stats_udp, chkerr, JSON_TOK_NUMBER),
};

static const struct json_obj_descr net_stats_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct net_stats, processing_error, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, bytes, net_stats_bytes_descr),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, ip_errors, net_stats_ip_errors_descr),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, ipv4, net_stats_ip_descr),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, icmp, net_stats_icmp_descr),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, tcp, net_stats_tcp_descr),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, udp, net_stats_udp_descr),
};

#if defined(CONFIG_APP_SYSTEM_MONITORING)
struct json_info_mbedtls_stats {
	uint32_t cur_used;
	uint32_t cur_blocks;
	uint32_t max_used;
	uint32_t max_blocks;
};

static const struct json_obj_descr info_mbedtls_stats_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_info_mbedtls_stats, cur_used, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_info_mbedtls_stats, cur_blocks, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_info_mbedtls_stats, max_used, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_info_mbedtls_stats, max_blocks, JSON_TOK_NUMBER),
};
#endif /* CONFIG_APP_SYSTEM_MONITORING */

struct json_info {
	uint32_t uptime;
	uint32_t timestamp;
	struct json_info_controller_status status;

#if defined(CONFIG_APP_SYSTEM_MONITORING)
	struct json_info_mbedtls_stats mbedtls_stats;
#endif
};

static const struct json_obj_descr info_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_info, uptime, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_info, timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT(struct json_info, status, info_controller_status_descr),
#if defined(CONFIG_APP_SYSTEM_MONITORING)
	JSON_OBJ_DESCR_OBJECT(struct json_info, mbedtls_stats, info_mbedtls_stats_descr),
#endif
};

int rest_info(http_request_t *req, http_response_t *resp)
{
	struct json_info data;

	/* get time */
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	/* get iterface info */
	// struct net_if_config *const ifcfg = &net_if_get_default()->config;
	// char unicast_str[NET_IPV4_ADDR_LEN] = "";
	// char mcast_str[NET_IPV4_ADDR_LEN] = "";
	// char gateway_str[NET_IPV4_ADDR_LEN] = "";
	// char netmask_str[NET_IPV4_ADDR_LEN] = "";
	// char ethernet_mac_str[ETH_STR_LEN] = "";

	// ///

	/* system status */
	const controller_status_t status = {
		.atomic_val = atomic_get(&controller_status.atomic)};

	data.uptime    = k_uptime_get() / MSEC_PER_SEC;
	data.timestamp = (uint32_t)ts.tv_sec;

	data.status.has_ipv4_addr     = status.has_ipv4_addr;
	data.status.valid_system_time = status.valid_system_time;

	/* mbedtls stats */
#if defined(CONFIG_APP_SYSTEM_MONITORING)
	mbedtls_memory_buffer_alloc_cur_get(&data.mbedtls_stats.cur_used,
					    &data.mbedtls_stats.cur_blocks);
	mbedtls_memory_buffer_alloc_max_get(&data.mbedtls_stats.max_used,
					    &data.mbedtls_stats.max_blocks);
#endif

	/* rencode response */
	return rest_encode_response_json(resp, &data, info_descr, ARRAY_SIZE(info_descr));
}

struct json_net_interface_config {
	char *ethernet_mac;
	char *unicast;
	char *mcast;
	char *gateway;
	char *netmask;
};

struct json_net_interface_config_storage {
	char unicast_str[NET_IPV4_ADDR_LEN];
	char mcast_str[NET_IPV4_ADDR_LEN];
	char gateway_str[NET_IPV4_ADDR_LEN];
	char netmask_str[NET_IPV4_ADDR_LEN];
	char ethernet_mac_str[ETH_STR_LEN];
};

static const struct json_obj_descr json_net_interface_config_descr[] = {
	JSON_OBJ_DESCR_PRIM(
		struct json_net_interface_config, ethernet_mac, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_net_interface_config, unicast, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_net_interface_config, mcast, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_net_interface_config, gateway, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_net_interface_config, netmask, JSON_TOK_STRING),
};

/* base on : net_if / struct net_if_ipv4 */
struct json_net_interface {
	const char *status;
	struct net_stats net_stats;
	struct json_net_interface_config config;
	struct json_net_interface_config_storage config_storage;
};

static const struct json_obj_descr json_net_interface_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_net_interface, status, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(
		struct json_net_interface, config, json_net_interface_config_descr),
	JSON_OBJ_DESCR_OBJECT(struct json_net_interface, net_stats, net_stats_descr),
};

struct json_info_interfaces {
	struct json_net_interface interfaces[2u];
	uint32_t if_count;
};

static const struct json_obj_descr json_info_interfaces_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_info_interfaces,
				 interfaces,
				 2u,
				 if_count,
				 json_net_interface_descr,
				 ARRAY_SIZE(json_net_interface_descr)),
};

static void json_net_interface_info_fill(struct json_net_interface *ifdata,
					 struct net_if *iface)
{
	ifdata->status = net_interface_status_get(iface);

	ifdata->config.unicast	    = ifdata->config_storage.unicast_str;
	ifdata->config.mcast	    = ifdata->config_storage.mcast_str;
	ifdata->config.gateway	    = ifdata->config_storage.gateway_str;
	ifdata->config.netmask	    = ifdata->config_storage.netmask_str;
	ifdata->config.ethernet_mac = ifdata->config_storage.ethernet_mac_str;

	struct net_if_config *const ifcfg = &iface->config;

	net_addr_ntop(AF_INET,
		      (const void *)&ifcfg->ip.ipv4->unicast[0].address.in_addr,
		      ifdata->config.unicast,
		      sizeof(ifdata->config_storage.unicast_str));
	net_addr_ntop(AF_INET,
		      (const void *)&ifcfg->ip.ipv4->mcast[0].address.in_addr,
		      ifdata->config.mcast,
		      sizeof(ifdata->config_storage.mcast_str));
	net_addr_ntop(AF_INET,
		      (const void *)&ifcfg->ip.ipv4->gw,
		      ifdata->config.gateway,
		      sizeof(ifdata->config_storage.gateway_str));
	net_addr_ntop(AF_INET,
		      (const void *)&ifcfg->ip.ipv4->netmask,
		      ifdata->config.netmask,
		      sizeof(ifdata->config_storage.netmask_str));

	struct net_linkaddr *l2_addr = net_if_get_link_addr(iface);
	if (l2_addr->type == NET_LINK_ETHERNET) {
		sprintf(ifdata->config.ethernet_mac,
			"%02X:%02X:%02X:%02X:%02X:%02X",
			l2_addr->addr[0],
			l2_addr->addr[1],
			l2_addr->addr[2],
			l2_addr->addr[3],
			l2_addr->addr[4],
			l2_addr->addr[5]);
	}

	/* get network stats */
	net_mgmt(NET_REQUEST_STATS_GET_ALL,
		 iface,
		 &ifdata->net_stats,
		 sizeof(struct net_stats));
}

void rest_info_net_iface_cb(struct net_if *iface, void *user_data)
{
	struct json_info_interfaces *const data = user_data;

	if (data->if_count < ARRAY_SIZE(data->interfaces)) {
		json_net_interface_info_fill(&data->interfaces[data->if_count], iface);
		data->if_count++;
	}
}

int rest_interfaces_list(http_request_t *req, http_response_t *resp)
{
	struct json_info_interfaces data;

	data.if_count = 0u;

	net_if_foreach(rest_info_net_iface_cb, &data);

	return rest_encode_response_json_array(resp, &data, json_info_interfaces_descr);
}

int rest_interface(http_request_t *req, http_response_t *resp)
{
	int ret		      = 0;
	uint32_t iface_number = 0u;
	route_arg_get(req, "idx", &iface_number);
	struct net_if *const iface = net_if_get_by_index(iface_number + 1u);

	if (iface != NULL) {
		struct json_net_interface data;
		json_net_interface_info_fill(&data, iface);

		ret = rest_encode_response_json(resp,
						&data,
						json_net_interface_descr,
						ARRAY_SIZE(json_net_interface_descr));
	} else {
		http_response_set_status_code(resp, HTTP_STATUS_NOT_FOUND);
	}

	return ret;
}

int rest_interface_set(http_request_t *req, http_response_t *resp)
{
	return -ENOTSUP;
}

#if defined(CONFIG_APP_HA)

const struct json_obj_descr json_xiaomi_record_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, bt_mac, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, base.timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, measures.rssi, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(
		struct json_xiaomi_record, measures.temperature, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(
		struct json_xiaomi_record, measures.temperature_raw, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(
		struct json_xiaomi_record, measures.humidity, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(
		struct json_xiaomi_record, measures.battery_level, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(
		struct json_xiaomi_record, measures.battery_voltage, JSON_TOK_NUMBER),
};

const struct json_obj_descr json_xiaomi_record_array_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_xiaomi_record_array,
				 records,
				 HA_XIAOMI_MAX_DEVICES,
				 count,
				 json_xiaomi_record_descr,
				 ARRAY_SIZE(json_xiaomi_record_descr))};

static int ha_json_xiaomi_record_feed_latest(struct json_xiaomi_record *json_data,
					     struct json_xiaomi_record_buf *buf,
					     ha_dev_t *dev)
{
	ha_ev_t *ev			      = ha_dev_get_last_event(dev, 0u);
	const struct ha_ds_xiaomi *const data = ev->data;

	json_data->bt_mac		    = buf->addr;
	json_data->measures.rssi	    = data->rssi.value;
	json_data->measures.temperature	    = buf->temperature;
	json_data->measures.temperature_raw = data->temperature.value;
	json_data->measures.humidity	    = data->humidity.value;
	json_data->measures.battery_level   = data->battery_level.level;
	json_data->measures.battery_voltage = data->battery_level.voltage;

	json_data->base.timestamp = ev->timestamp;

	bt_addr_le_to_str(&dev->addr.mac.addr.ble, json_data->bt_mac, BT_ADDR_LE_STR_LEN);

	sprintf(json_data->measures.temperature, "%.2f", data->temperature.value / 100.0);

	return 0;
}

static bool xiaomi_device_cb(ha_dev_t *dev, void *user_data)
{
	struct json_xiaomi_record_array *const array = user_data;

	ha_json_xiaomi_record_feed_latest(
		&array->records[array->count], &array->_bufs[array->count], dev);

	array->count++;

	return true;
}

int rest_xiaomi_records(http_request_t *req, http_response_t *resp)
{
	struct json_xiaomi_record_array array;

	array.count = 0;

	ha_dev_xiaomi_iterate_data(xiaomi_device_cb, &array);

	return rest_encode_response_json_array(
		resp, &array, json_xiaomi_record_array_descr);
}

struct json_caniot_temperature_record {
	const char *repr;
	int32_t value;
	uint32_t sens_type;
};

static const struct json_obj_descr json_caniot_temperature_record_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_caniot_temperature_record, repr, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(
		struct json_caniot_temperature_record, value, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(
		struct json_caniot_temperature_record, sens_type, JSON_TOK_NUMBER),
};

/* todo, rename to json_caniot_telemetry */
struct json_caniot_telemetry {
	uint32_t did;

	/* device base data */
	struct json_device_base base;

	/* request duration (in case of a request) */
	uint32_t duration;

	uint32_t dio;
	uint32_t pdio;

	struct json_caniot_temperature_record temperatures[HA_CANIOT_MAX_TEMPERATURES];
	uint32_t temperatures_count;
};

static const struct json_obj_descr json_caniot_telemetry_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, did, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_caniot_telemetry,
				  "timestamp",
				  base.timestamp,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, dio, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, pdio, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_caniot_telemetry,
				 temperatures,
				 HA_CANIOT_MAX_TEMPERATURES,
				 temperatures_count,
				 json_caniot_temperature_record_descr,
				 ARRAY_SIZE(json_caniot_temperature_record_descr)),
};

struct json_caniot_telemetry_array {
	struct json_caniot_telemetry records[REST_HA_DEVICES_MAX_COUNT_PER_PAGE];
	size_t count;
	char temp_repr[REST_HA_DEVICES_MAX_COUNT_PER_PAGE][HA_CANIOT_MAX_TEMPERATURES]
		      [9U];
};

const struct json_obj_descr json_caniot_telemetry_array_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_caniot_telemetry_array,
				 records,
				 REST_HA_DEVICES_MAX_COUNT_PER_PAGE,
				 count,
				 json_caniot_telemetry_descr,
				 ARRAY_SIZE(json_caniot_telemetry_descr))};

static bool caniot_device_cb(ha_dev_t *dev, void *user_data)
{
	if (CANIOT_DID_CLS(dev->addr.mac.addr.caniot) != CANIOT_DEVICE_CLASS0) {
		LOG_WRN("(TODO) Skipping CANIOT non-class0 device (did=%x)",
			dev->addr.mac.addr.caniot);
		return true;
	}

	struct json_caniot_telemetry_array *const arr =
		(struct json_caniot_telemetry_array *)user_data;

	struct json_caniot_telemetry *const rec = &arr->records[arr->count];
	const struct ha_ds_caniot_blc0 *const dt =
		HA_DEV_EP_0_GET_CAST_LAST_DATA(dev, struct ha_ds_caniot_blc0);

	ha_ev_t *ev		= ha_dev_get_last_event(dev, 0u);
	rec->base.timestamp	= ev->timestamp;
	rec->did		= (uint32_t)dev->addr.mac.addr.caniot;
	rec->temperatures_count = 0U;
	rec->dio		= dt->dio.value;
	rec->pdio		= dt->pdio.value;

	/* TODO USE ha_data_get() to get the temperatures */

	/* encode temperatures */
	for (size_t i = 0; i < HA_CANIOT_MAX_TEMPERATURES; i++) {
		/* if temperature is valid */
		if (dt->temperatures[i].type != HA_DEV_SENSOR_TYPE_NONE) {
			const size_t j = rec->temperatures_count;

			sprintf(arr->temp_repr[arr->count][j],
				"%.2f",
				dt->temperatures[i].value / 100.0);

			rec->temperatures[j].repr      = arr->temp_repr[arr->count][j];
			rec->temperatures[j].sens_type = dt->temperatures[i].type;
			rec->temperatures[j].value     = dt->temperatures[i].value;
			rec->temperatures_count++;
		}
	}

	arr->count++;

	return true;
}

__deprecated int rest_caniot_records(http_request_t *req, http_response_t *resp)
{
	/* Parge page */
	int page_n	     = 0;
	char *const page_str = query_args_parse_find(req->query_string, "page");
	if (page_str) {
		page_n = atoi(page_str);
	}
	if (page_n < 0) {
		page_n = 0;
	}

	const ha_dev_filter_t filter = {
		.flags = HA_DEV_FILTER_DATA_EXIST | HA_DEV_FILTER_DEVICE_TYPE |
			 HA_DEV_FILTER_FROM_INDEX | HA_DEV_FILTER_TO_INDEX,
		.device_type = HA_DEV_TYPE_CANIOT,
		.endpoint_id = HA_DEV_EP_NONE,
		.from_index  = REST_HA_DEVICES_MAX_COUNT_PER_PAGE * page_n,
		.to_index    = REST_HA_DEVICES_MAX_COUNT_PER_PAGE * page_n +
			    REST_HA_DEVICES_MAX_COUNT_PER_PAGE};

	struct json_caniot_telemetry_array arr;
	arr.count = 0;

	ha_dev_iterate(caniot_device_cb, &filter, NULL, &arr);

	return rest_encode_response_json_array(
		resp, &arr, json_caniot_telemetry_array_descr);
}

struct json_dev_ep_last_ev {
	uint32_t addr;
	uint32_t timestamp;
	uint32_t refcount;
	uint32_t type;
};

struct json_device_endpoint {
	uint32_t eid;
	uint32_t data_size;
	uint32_t in_data_size;
	struct json_dev_ep_last_ev last_event;
	uint32_t telemetry; /* ingest */
	uint32_t command;
};

struct json_device {
	uint32_t sdevuid;
	const char *addr_type;
	const char *addr_medium;
	char *addr_repr;

	uint32_t registered_timestamp;

	uint32_t rid;
	const char *room_name;

	struct ha_device_stats stats;

	uint32_t endpoints_count;
	struct json_device_endpoint endpoints[HA_DEV_EP_MAX_COUNT];
};

struct json_device_buf {
	char addr_repr[HA_DEV_ADDR_STR_MAX_LEN];
};

static const struct json_obj_descr json_dev_ep_last_ev_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_dev_ep_last_ev, addr, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_dev_ep_last_ev, timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_dev_ep_last_ev, refcount, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_dev_ep_last_ev, type, JSON_TOK_NUMBER),
};

static const struct json_obj_descr json_device_endpoint_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_device_endpoint, eid, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_device_endpoint, data_size, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_device_endpoint, in_data_size, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT(
		struct json_device_endpoint, last_event, json_dev_ep_last_ev_descr),
	JSON_OBJ_DESCR_PRIM(struct json_device_endpoint, telemetry, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_device_endpoint, command, JSON_TOK_NUMBER),
};

static const struct json_obj_descr json_device_stats_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct ha_device_stats, rx, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_device_stats, rx_bytes, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_device_stats, tx, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_device_stats, tx_bytes, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_device_stats, err_ev, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_device_stats, err_flags, JSON_TOK_NUMBER),
};

static const struct json_obj_descr json_device_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_device, sdevuid, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_device, addr_type, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_device, addr_medium, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_device, addr_repr, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_device, registered_timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_device,
				 endpoints,
				 HA_DEV_EP_MAX_COUNT,
				 endpoints_count,
				 json_device_endpoint_descr,
				 ARRAY_SIZE(json_device_endpoint_descr)),
	JSON_OBJ_DESCR_PRIM(struct json_device, rid, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_device, room_name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct json_device, stats, json_device_stats_descr),
};

/* ~154 B per device here */
struct json_device_array {
	struct json_device_buf _bufs[JSON_HA_MAX_DEVICES];
	struct json_device devices[JSON_HA_MAX_DEVICES];
	size_t count;
};

static const struct json_obj_descr json_device_array_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_device_array,
				 devices,
				 JSON_HA_MAX_DEVICES,
				 count,
				 json_device_descr,
				 ARRAY_SIZE(json_device_descr))};

static bool devices_cb(ha_dev_t *dev, void *user_data)
{
	struct json_device_array *const arr = user_data;
	struct json_device *jd		    = &arr->devices[arr->count];

	jd->endpoints_count = 0u;

	for (uint32_t i = 0u; i < dev->endpoints_count; i++) {
		struct ha_device_endpoint *ep = ha_dev_ep_get(dev, i);
		if (ep) {
			struct json_device_endpoint *jep =
				&jd->endpoints[jd->endpoints_count];
			jep->eid	  = ep->api->eid;
			jep->data_size	  = ep->api->data_size;
			jep->in_data_size = ep->api->expected_payload_size;
			jep->telemetry	  = (uint32_t)ep->api->ingest;
			jep->command	  = (uint32_t)ep->api->command;

			jd->endpoints_count++;

			ha_ev_t *const last_ev = ep->last_data_event;
			if (last_ev) {
				jep->last_event.addr	  = (uint32_t)last_ev;
				jep->last_event.refcount  = last_ev->ref_count;
				jep->last_event.timestamp = last_ev->timestamp;
				jep->last_event.type	  = last_ev->type;
			} else {
				memset(&jep->last_event, 0, sizeof(jep->last_event));
			}
		}
	}

	jd->sdevuid   = dev->sdevuid;
	jd->addr_repr = arr->_bufs[arr->count].addr_repr;
	ha_dev_addr_to_str(&dev->addr, jd->addr_repr, HA_DEV_ADDR_STR_MAX_LEN);
	jd->addr_medium		 = ha_dev_medium_to_str(dev->addr.mac.medium);
	jd->addr_type		 = ha_dev_type_to_str(dev->addr.type);
	jd->registered_timestamp = dev->registered_timestamp;
	struct ha_room *room	 = ha_dev_get_room(dev);
	if (room) {
		jd->rid	      = room->rid;
		jd->room_name = room->name;
	} else {
		jd->rid	      = 0;
		jd->room_name = "";
	}

	memcpy(&jd->stats, &dev->stats, sizeof(jd->stats));

	arr->count++;

	return arr->count < JSON_HA_MAX_DEVICES;
}

int rest_devices_list(http_request_t *req, http_response_t *resp)
{
	/* Parge page */
	int page_n	     = 0;
	char *const page_str = query_args_parse_find(req->query_string, "page");
	if (page_str) {
		page_n = atoi(page_str);
	}
	if (page_n < 0) {
		page_n = 0;
	}

	struct json_device_array arr;

	arr.count = 0u;

	const ha_dev_filter_t filter = {
		.flags	    = HA_DEV_FILTER_FROM_INDEX | HA_DEV_FILTER_TO_INDEX,
		.from_index = REST_HA_DEVICES_MAX_COUNT_PER_PAGE * page_n,
		.to_index   = REST_HA_DEVICES_MAX_COUNT_PER_PAGE * page_n +
			    REST_HA_DEVICES_MAX_COUNT_PER_PAGE};

	ha_dev_iterate(devices_cb, &filter, &HA_DEV_ITER_OPT_LOCK_ALL(), &arr);

	/* TODO use "json_obj_encode()" to encode incrementally
	 * all the devices with CHUNKED encoding
	 */

	return rest_encode_response_json_array(resp, &arr, json_device_array_descr);
}

int rest_device_get(http_request_t *req, http_response_t *resp)
{
	/* TODO */
	return 0u;
}

static const struct json_obj_descr json_ha_stats_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct ha_stats, ev, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, dev_dropped, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, dev_no_mem, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, dev_no_api, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, dev_ep_init, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, dev_no_ep, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, dev_toomuch_ep, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, ev_dropped, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, ev_data_dropped, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, ev_cmd_dropped, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, ev_no_mem, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, ev_no_ep, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, ev_ep, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, ev_payload_size, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, ev_no_data_mem, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, ev_ingest, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, ev_never_ref, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, mem_ev_count, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, mem_ev_remaining, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, mem_device_count, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, mem_device_remaining, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, mem_sub_count, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, mem_sub_remaining, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, mem_heap_alloc, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ha_stats, mem_heap_total, JSON_TOK_NUMBER),
};

int rest_ha_stats(http_request_t *req, http_response_t *resp)
{
	struct ha_stats stats;
	ha_stats_copy(&stats);

	return rest_encode_response_json(
		resp, &stats, json_ha_stats_descr, ARRAY_SIZE(json_ha_stats_descr));
}

static bool room_devices_cb(ha_dev_t *dev, void *user_data)
{
	buffer_t *const buf = (buffer_t *)user_data;

	buffer_snprintf(buf, "%p<br/>", dev);

	return true;
}

int rest_room_devices_list(http_request_t *req, http_response_t *resp)
{
	const ha_dev_filter_t filter = {
		.flags = HA_DEV_FILTER_ROOM_ID,
		.rid   = HA_ROOM_MY,
	};

	ha_dev_iterate(
		room_devices_cb, &filter, &HA_DEV_ITER_OPT_LOCK_ALL(), &resp->buffer);

	return 0;
}

#endif

int rest_caniot_info(http_request_t *req, http_response_t *resp)
{
	return -ENOTSUP;
}

int rest_caniot_command(http_request_t *req, http_response_t *resp)
{
	// const uint8_t did = 24;
	// const uint8_t enpoint = CANIOT_ENDPOINT_BOARD_CONTROL;

	return -ENOTSUP;
}

int rest_test_caniot_query_telemetry(http_request_t *req, http_response_t *resp)
{
	struct caniot_frame query, response;

	uint64_t buf = 1U;

	caniot_build_query_command(
		&query, CANIOT_ENDPOINT_APP, (uint8_t *)&buf, sizeof(buf));
	const caniot_did_t did = CANIOT_DID(CANIOT_DEVICE_CLASS0, CANIOT_DEVICE_SID4);

	uint32_t timeout = MIN(req->timeout_ms, REST_CANIOT_QUERY_MAX_TIMEOUT_MS);
	ha_caniot_controller_query(&query, &response, did, &timeout);

	return 0;
}

struct json_can_payload {
	size_t count;
	uint32_t vals[CAN_MAX_DLEN];
};

static const struct json_obj_descr json_can_payload_descr[] = {JSON_OBJ_DESCR_ARRAY(
	struct json_can_payload, vals, CAN_MAX_DLEN, count, JSON_TOK_NUMBER)};

static int json_parse_can_payload(char *buf, size_t len, uint8_t can_buf[CAN_MAX_DLEN])
{
	struct json_can_payload data;

	/* Parse payload */
	int ret = json_arr_parse(buf, len, json_can_payload_descr, &data);
	if (ret < 0 || data.count > CAN_MAX_DLEN) {
		goto exit;
	}

	/* validate command and build it */
	for (size_t i = 0; i < data.count; i++) {
		if (data.vals[i] > 0xFF) {
			ret = -EINVAL;
			goto exit;
		}
		can_buf[i] = data.vals[i];
	}

	ret = data.count;
exit:
	return ret;
}

#if defined(CONFIG_APP_CANIOT_CONTROLLER)

int rest_devices_garage_get(http_request_t *req, http_response_t *resp)
{
	return 0;
}

/* REST_HANDLE_DECL(devices_garage_get) { } */

struct json_garage_post {
	const char *left_door;
	const char *right_door;
};

const struct json_obj_descr json_garage_post_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_garage_post, left_door, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_garage_post, right_door, JSON_TOK_STRING),
};

int rest_devices_garage_post(http_request_t *req, http_response_t *resp)
{
	int ret;
	struct json_garage_post post;

	int map = json_obj_parse(req->payload.loc,
				 req->payload.len,
				 json_garage_post_descr,
				 ARRAY_SIZE(json_garage_post_descr),
				 &post);

	if (map > 0) {
		struct ha_dev_garage_cmd cmd;
		ha_dev_garage_cmd_init(&cmd);

		if (FIELD_SET(map, 0U) &&
		    ((ret = ha_parse_ss_command(post.left_door)) > 0)) {
			cmd.actuate_left = 1U;
		}

		if (FIELD_SET(map, 1U) &&
		    ((ret = ha_parse_ss_command(post.right_door)) > 0)) {
			cmd.actuate_right = 1U;
		}

		ha_dev_garage_cmd_send(&cmd);

		resp->status_code = 200U;
	} else {
		resp->status_code = 400U;
	}

	return 0;
}

static const struct json_obj_descr json_caniot_query_telemetry_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, did, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_caniot_telemetry,
				  "timestamp",
				  base.timestamp,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, duration, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, dio, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, pdio, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_caniot_telemetry,
				 temperatures,
				 HA_CANIOT_MAX_TEMPERATURES,
				 temperatures_count,
				 json_caniot_temperature_record_descr,
				 ARRAY_SIZE(json_caniot_temperature_record_descr)),
};

/*
[lucas@fedora zephyr-caniot-controller]$ python3 scripts/api.py
<Response [200]>
200
{'did': 32, 'dio': 240, 'duration': 2006, 'pdio': 0, 'temperatures': []}
*/

static int json_format_caniot_telemetry_resp(struct caniot_frame *r,
					     http_response_t *resp,
					     uint32_t timeout)
{
	if (r->id.cls == CANIOT_DEVICE_CLASS0) {
		struct ha_ds_caniot_blc0 blt;
		ha_dev_caniot_blc_cls0_to_blt(&blt, AS_BLC0_TELEMETRY(r->buf));

		struct json_caniot_telemetry json = {
			.did = CANIOT_DID(r->id.cls, r->id.sid),
			.base =
				{
					.timestamp = net_time_get(),
				},
			.duration	    = timeout,
			.dio		    = blt.dio.value,
			.pdio		    = blt.dio.value,
			.temperatures_count = 0U, /* TODO temperatures */
		};

		char temp_repr[HA_CANIOT_MAX_TEMPERATURES][9U];
		for (size_t i = 0; i < HA_CANIOT_MAX_TEMPERATURES; i++) {
			if (blt.temperatures[i].type == HA_DEV_SENSOR_TYPE_NONE) {
				continue;
			}

			const size_t j = json.temperatures_count++;

			sprintf(temp_repr[j], "%.2f", blt.temperatures[i].value / 100.0);

			json.temperatures[j].repr      = temp_repr[j];
			json.temperatures[j].sens_type = blt.temperatures[i].type;
			json.temperatures[j].value     = blt.temperatures[i].value;
		}

		resp->status_code = 200U;

		return rest_encode_response_json(
			resp,
			&json,
			json_caniot_query_telemetry_descr,
			ARRAY_SIZE(json_caniot_query_telemetry_descr));
	} else {
		LOG_WRN("Unsupported CANIOT device class %u", r->id.cls);
		return 0u;
	}
}

/* QUERY CANIOT COMMAND/TELEMETRY and BUILD JSON RESPONSE */
int caniot_q_ct_to_json_resp(struct caniot_frame *q,
			     caniot_did_t did,
			     uint32_t *timeout,
			     http_response_t *resp)
{
	struct caniot_frame r;

	int ret = ha_caniot_controller_query(q, &r, did, timeout);

	switch (ret) {
	case 1:
		/* Ok */
		resp->status_code = 200U;
		ret		  = json_format_caniot_telemetry_resp(&r, resp, *timeout);
		break;
	case 0:
		/* No response expected */
		resp->status_code = 204U;
		break;
	case 2:
		/* returned but with error */
		resp->status_code = 204U;
		break;
	case -EAGAIN:
		/* timeout */
		resp->status_code = 404U;
		break;
	case -EINVAL:
		/* Invalid arguments */
		resp->status_code = 400U;
		break;
	default:
		/* Other unhandled error */
		resp->status_code = 500U;
		break;
	}

	return 0;
}

int rest_devices_caniot_telemetry(http_request_t *req, http_response_t *resp)
{
	/* get ids */
	uint32_t did = 0, ep = 0;
	route_arg_get(req, "did", &did);
	route_arg_get(req, "ep", &ep);

	/* build CANIOT query */
	struct caniot_frame q;
	caniot_build_query_telemetry(&q, ep);

	/* execute and build appropriate response */
	uint32_t timeout = MIN(req->timeout_ms, REST_CANIOT_QUERY_MAX_TIMEOUT_MS);
	int ret		 = caniot_q_ct_to_json_resp(&q, did, &timeout, resp);
	LOG_INF("GET /devices/caniot/%u/endpoints/%u/telemetry -> %d [in %u "
		"ms]",
		did,
		ep,
		ret,
		timeout);

	return 0;
}

struct json_caniot_blc0_cmd_post_descr {
	const char *coc1;
	const char *coc2;
	const char *crl1;
	const char *crl2;
};

const struct json_obj_descr json_caniot_blc0_cmd_post_descr[] = {
	JSON_OBJ_DESCR_PRIM(
		struct json_caniot_blc0_cmd_post_descr, coc1, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(
		struct json_caniot_blc0_cmd_post_descr, coc2, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(
		struct json_caniot_blc0_cmd_post_descr, crl1, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(
		struct json_caniot_blc0_cmd_post_descr, crl2, JSON_TOK_STRING),
};

int rest_devices_caniot_blc0_command(http_request_t *req, http_response_t *resp)
{
	int ret = 0;
	struct json_caniot_blc0_cmd_post_descr post;

	int map = json_obj_parse(req->payload.loc,
				 req->payload.len,
				 json_caniot_blc0_cmd_post_descr,
				 ARRAY_SIZE(json_caniot_blc0_cmd_post_descr),
				 &post);

	/* if no commands are given, we do nothing */
	if (map <= 0) {
		resp->status_code = 400U;
		goto exit;
	}

	/* build command */
	struct caniot_blc0_command cmd;
	caniot_blc0_command_init(&cmd);

	if (FIELD_SET(map, 0U)) {
		cmd.coc1 = ha_parse_xps_command(post.coc1);
	}

	if (FIELD_SET(map, 1U)) {
		cmd.coc2 = ha_parse_xps_command(post.coc2);
	}

	if (FIELD_SET(map, 2U)) {
		cmd.crl1 = ha_parse_xps_command(post.crl1);
	}

	if (FIELD_SET(map, 3U)) {
		cmd.crl2 = ha_parse_xps_command(post.crl2);
	}

	/* TODO add support for reset commands + config reset */

	/* parse did */
	uint32_t did = 0;
	route_arg_get(req, "did", &did);

	/* build CANIOT query */
	struct caniot_frame q;
	caniot_build_query_command(
		&q, CANIOT_ENDPOINT_BOARD_CONTROL, (uint8_t *)&cmd, sizeof(cmd));

	/* execute and build appropriate response */
	uint32_t timeout = MIN(req->timeout_ms, REST_CANIOT_QUERY_MAX_TIMEOUT_MS);
	ret		 = caniot_q_ct_to_json_resp(&q, did, &timeout, resp);

	LOG_INF("GET /devices/caniot/%u/endpoints/blc/command -> %d [in %u ms]",
		did,
		ret,
		timeout);

exit:
	return ret;
}

struct json_caniot_blc1_cmd_post {
	const char *xps[19u];
	uint32_t count;
};

const struct json_obj_descr json_caniot_blc1_cmd_post_descr[] = {JSON_OBJ_DESCR_ARRAY(
	struct json_caniot_blc1_cmd_post, xps, 19U, count, JSON_TOK_STRING)};

int rest_devices_caniot_blc1_command(http_request_t *req, http_response_t *resp)
{
	/* parse did */
	int ret	     = 0;
	uint32_t did = 0;
	route_arg_get(req, "did", &did);

	resp->status_code = HTTP_STATUS_BAD_REQUEST;

	if (caniot_deviceid_valid(did) == false) {
		goto exit;
	}

	/* parse payload */
	struct json_caniot_blc1_cmd_post post;
	ret = json_arr_parse(req->payload.loc,
			     req->payload.len,
			     json_caniot_blc1_cmd_post_descr,
			     &post);
	if (ret < 0) {
		ret = 0;
		goto exit;
	}

	resp->status_code = HTTP_STATUS_OK;

	/* Build class1 BLC command */
	struct caniot_blc1_command cmd;
	caniot_cmd_blc1_init(&cmd);
	for (uint32_t i = 0; i < post.count; i++) {
		caniot_complex_digital_cmd_t xps = ha_parse_xps_command(post.xps[i]);
		LOG_INF("BLC1 XPS[%u] = %s (%u)", i, post.xps[i], xps);
		caniot_cmd_blc1_set_xps(&cmd, i, xps);
	}

	/* Convert to CANIOT command */
	struct caniot_frame q;
	caniot_build_query_command(
		&q, CANIOT_ENDPOINT_BOARD_CONTROL, (uint8_t *)&cmd, sizeof(cmd));

	LOG_HEXDUMP_INF((uint8_t *)&cmd, sizeof(cmd), "BLC1 command: ");

	/* execute and build appropriate response */
	uint32_t timeout = MIN(req->timeout_ms, REST_CANIOT_QUERY_MAX_TIMEOUT_MS);
	ret		 = caniot_q_ct_to_json_resp(&q, did, &timeout, resp);

	LOG_INF("POST /devices/caniot/%u/endpoints/blc/command -> %d", did, ret);

exit:
	return ret;
}

int rest_devices_caniot_blc_command(http_request_t *req, http_response_t *resp)
{
	return -ENOTSUP;
}

int rest_devices_caniot_command(http_request_t *req, http_response_t *resp)
{
	int ret = 0u;
	uint8_t can_buf[CAN_MAX_DLEN];

	/* parse did, ep */
	uint32_t did = 0, ep = 0;
	route_arg_get(req, "did", &did);
	route_arg_get(req, "ep", &ep);

	if (!caniot_deviceid_valid(did) || !caniot_endpoint_valid(ep)) {
		http_response_set_status_code(resp, HTTP_STATUS_BAD_REQUEST);
		goto exit;
	}

	/* Parse payload */
	int dlc = json_parse_can_payload(req->payload.loc, req->payload.len, can_buf);
	if (dlc < 0) {
		http_response_set_status_code(resp, HTTP_STATUS_BAD_REQUEST);
		goto exit;
	}

	/* build CANIOT query */
	struct caniot_frame q;
	caniot_build_query_command(&q, ep, can_buf, dlc);

	/* execute and build appropriate response */
	uint32_t timeout = MIN(req->timeout_ms, REST_CANIOT_QUERY_MAX_TIMEOUT_MS);
	ret		 = caniot_q_ct_to_json_resp(&q, did, &timeout, resp);

	LOG_INF("POST /devices/caniot/%u/endpoints/%u/command -> %d [in %u ms]",
		did,
		ep,
		ret,
		timeout);

exit:
	return ret;
}

struct json_caniot_attr {
	char *status;
	int duration;
	int32_t caniot_error;

	uint32_t key;
	uint32_t value;

	char *key_repr;
	char *value_repr;
};

static const struct json_obj_descr json_caniot_attr_ok_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, status, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, duration, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, caniot_error, JSON_TOK_NUMBER),

	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, key, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, value, JSON_TOK_NUMBER),

	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, key_repr, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, value_repr, JSON_TOK_STRING),
};

static const struct json_obj_descr json_caniot_attr_nok_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, status, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, duration, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, caniot_error, JSON_TOK_NUMBER),

	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, key, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, key_repr, JSON_TOK_STRING),
};

struct json_caniot_attr_write_value {
	char *value;
};

const struct json_obj_descr json_caniot_attr_write_value_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr_write_value, value, JSON_TOK_STRING),
};

int rest_devices_caniot_attr_read_write(http_request_t *req, http_response_t *resp)
{
	int ret	     = 0;
	uint32_t did = 0, key = 0;
	struct caniot_frame q, r;
	const struct json_obj_descr *descr = json_caniot_attr_nok_descr;
	size_t descr_size		   = ARRAY_SIZE(json_caniot_attr_nok_descr);

	/* Parse request*/
	route_arg_get(req, "did", &did);
	route_arg_get(req, "key", &key);

	/* default status code */
	resp->status_code = 400U;

	if (!caniot_deviceid_valid(did)) {
		goto exit;
	}

	if (req->method == HTTP_PUT) {
		/* It's an attribute write */
		if (key > 0xFFFFLU) {
			goto exit;
		}

		/* try to parse content */
		char *value_str;
		int map = json_obj_parse(req->payload.loc,
					 req->payload.len,
					 json_caniot_attr_write_value_descr,
					 ARRAY_SIZE(json_caniot_attr_write_value_descr),
					 &value_str);
		if ((map <= 0) || !FIELD_SET(map, 0U)) {
			goto exit;
		}

		uint32_t value;
		if (sscanf(value_str, "0x%x", &value) != 1) {
			if (sscanf(value_str, "%u", &value) != 1) {
				goto exit;
			}
		}

		caniot_build_query_write_attribute(&q, key, value);
	} else {
		/* It's an attribute read */
		caniot_build_query_read_attribute(&q, key);
	}

	/* Make CANIOT request */

	uint32_t timeout = MIN(req->timeout_ms, REST_CANIOT_QUERY_MAX_TIMEOUT_MS);

	ret = ha_caniot_controller_query(&q, &r, did, &timeout);

	/* Prepare response*/

	char key_repr[sizeof("0xFFFF")];
	char val_repr[sizeof("0xFFFFFFFF")];

	struct json_caniot_attr json = {
		.duration     = timeout,
		.caniot_error = 0,

		.key   = key,
		.value = 0,

		.key_repr   = key_repr,
		.value_repr = val_repr,
	};

	switch (ret) {
	case 1:
		/* Ok */
		resp->status_code = 200U;

		json.status = "OK";
		json.key    = r.attr.key;
		json.value  = r.attr.val;

		snprintf(val_repr, sizeof(val_repr), "0x%08X", r.attr.val);

		/* Use ok descriptor */
		descr	   = json_caniot_attr_ok_descr;
		descr_size = ARRAY_SIZE(json_caniot_attr_ok_descr);

		break;
	case 0:
		/* No response was expected */
		resp->status_code = 200U;

		json.status = "NO_RESP";
		break;
	case 2:
		/* returned but with CANIOT error */
		resp->status_code = 200U;

		json.status	  = "ERROR";
		json.caniot_error = r.err;
		break;
	case -EAGAIN:
		/* timeout */
		resp->status_code = 200U;

		json.status = "TIMEOUT";
		break;
	case -EINVAL:
		/* Invalid arguments */
		resp->status_code = 400U;
		break;
	default:
		/* Other unhandled error */
		resp->status_code = 500U;
		goto exit;
	}

	snprintf(key_repr, sizeof(key_repr), "0x%04X", r.attr.key);

	ret = rest_encode_response_json(resp, &json, descr, descr_size);

exit:
	return ret;
}

int rest_devices_caniot_blc_action(http_request_t *req, http_response_t *resp)
{
	int ret = 0;
	const struct route_descr *descr;
	uint32_t did = 0;
	struct caniot_blc_command cmd;
	struct caniot_frame q;

	/* Check did */
	route_arg_get(req, "did", &did);
	if (!caniot_deviceid_valid(did)) {
		resp->status_code = 400U;
		goto exit;
	}

	descr = req->route_parse_results[req->route_parse_results_len - 1u].descr;

	caniot_blc_command_init(&cmd);

	if (strcmp(descr->part.str, "reboot") == 0) {
		caniot_blc_sys_req_reboot(&cmd.sys);
	} else if (strcmp(descr->part.str, "factory_reset") == 0) {
		caniot_blc_sys_req_factory_reset(&cmd.sys);
	}

	ret = caniot_build_query_command(
		&q, CANIOT_ENDPOINT_BOARD_CONTROL, (uint8_t *)&cmd, sizeof(cmd));
	if (ret) {
		goto exit;
	}

	uint32_t timeout = MIN(req->timeout_ms, REST_CANIOT_QUERY_MAX_TIMEOUT_MS);
	ret		 = caniot_q_ct_to_json_resp(&q, did, &timeout, resp);

	LOG_INF("POST /devices/caniot/%u/%s -> %d [in %u ms]",
		did,
		descr->part.str,
		ret,
		timeout);
exit:
	return ret;
}

#endif /* CONFIG_APP_CANIOT_CONTROLLER */

#if defined(CONFIG_APP_CAN_INTERFACE)

int rest_if_can(http_request_t *req, http_response_t *resp)
{
	int ret = 0;

	uint32_t arbitration_id = 0u;
	route_arg_get(req, "id", &arbitration_id);

	struct can_frame frame = {0};

	int dlc = json_parse_can_payload(req->payload.loc, req->payload.len, frame.data);
	if (dlc < 0) {
		http_response_set_status_code(resp, HTTP_STATUS_BAD_REQUEST);
		goto exit;
	}

	frame.id      = arbitration_id;
	frame.id_type = (arbitration_id <= CAN_STD_ID_MASK) ? CAN_ID_STD : CAN_ID_EXT;
	frame.rtr     = 0u;
	frame.dlc     = dlc;
	ret	      = if_can_send(CAN_BUS_CANIOT, &frame);

	LOG_INF("POST /if/can/%x [dlc=%u] -> %d", frame.id, dlc, ret);
exit:
	return ret;
}

#endif /* CONFIG_APP_CAN_INTERFACE */

#define REST_FS_FILES_LIST_MAX_COUNT 32U

#if defined(CONFIG_LUA)

struct json_fs_file_entry {
	char *name;
	uint32_t size;
};

struct json_fs_file_entries_list {
	/* TODO could be great to not needing pfiles intermediate array */
	char names[REST_FS_FILES_LIST_MAX_COUNT][REST_FS_FILES_LIST_MAX_COUNT];
	struct json_fs_file_entry entries[REST_FS_FILES_LIST_MAX_COUNT];
	size_t nb_entries;
};

static const struct json_obj_descr json_fs_file_entry_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_fs_file_entry, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_fs_file_entry, size, JSON_TOK_NUMBER),
};

static const struct json_obj_descr json_fs_file_entries_array_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_fs_file_entries_list,
				 entries,
				 REST_FS_FILES_LIST_MAX_COUNT,
				 nb_entries,
				 json_fs_file_entry_descr,
				 ARRAY_SIZE(json_fs_file_entry_descr))};

static bool fs_list_lua_scripts_detailled_cb(const char *path,
					     struct fs_dirent *dirent,
					     void *user_data)
{
	bool ret			       = true;
	struct json_fs_file_entries_list *data = user_data;

	if (dirent->type == FS_DIR_ENTRY_FILE) {
		data->entries[data->nb_entries].name = data->names[data->nb_entries];
		strncpy(data->entries[data->nb_entries].name, dirent->name, 32U);
		data->entries[data->nb_entries].size = dirent->size;

		data->nb_entries++;

		ret = data->nb_entries < 32U;
	}

	return true;
}

int rest_fs_list_lua_scripts(http_request_t *req, http_response_t *resp)
{
	/* willingly not clearing the whole buffer */
	struct json_fs_file_entries_list data;
	data.nb_entries = 0;

	app_fs_iterate_dir_files(CONFIG_APP_LUA_FS_SCRIPTS_DIR,
				 fs_list_lua_scripts_detailled_cb,
				 (void *)&data);

	return rest_encode_response_json_array(
		resp, &data, json_fs_file_entries_array_descr);

	/* TODO return actual path in the reponse header */
}

int rest_fs_remove_lua_script(http_request_t *req, http_response_t *resp)
{

	return -ENOTSUP;
}

struct json_lua_run_script {
	char *name;
	size_t lua_ret;

	/* TODO add execution time and other LUA debug info/context */
};

static const struct json_obj_descr json_lua_run_script_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_lua_run_script, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_lua_run_script, lua_ret, JSON_TOK_NUMBER),
};

int rest_lua_run_script(http_request_t *req, http_response_t *resp)
{
	int ret = 0;

	const char *reqpath = http_header_get_value(req, "App-Script-Filename");
	if (reqpath == NULL) {
		resp->status_code = 400u;
		goto exit;
	}
	char path[128u];
	snprintf(path, sizeof(path), "%s/%s", CONFIG_APP_LUA_FS_SCRIPTS_DIR, reqpath);

	int lua_ret;
	ret = lua_orch_run_script(path, &lua_ret);
	if (ret != 0) {
		resp->status_code = 400u;
		goto exit;
	}

	struct json_lua_run_script data;
	data.name    = path;
	data.lua_ret = lua_ret;
	ret	     = rest_encode_response_json(resp,
					 &data,
					 json_lua_run_script_descr,
					 ARRAY_SIZE(json_lua_run_script_descr));

exit:
	return ret;
}

#endif /* CONFIG_LUA */

#if defined(CONFIG_APP_CREDS_FLASH)

struct json_flash_cred_entry {
	uint32_t slot;
	const char *id;
	const char *format;
	uint32_t strength;
	uint32_t version;
	uint32_t size;
};

static const struct json_obj_descr json_flash_cred_entry_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_flash_cred_entry, slot, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_flash_cred_entry, id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_flash_cred_entry, format, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_flash_cred_entry, strength, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_flash_cred_entry, version, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_flash_cred_entry, size, JSON_TOK_NUMBER),
};

struct json_flash_creds_list {
	struct json_flash_cred_entry creds[FLASH_CREDS_SLOTS_MAX_COUNT];
	size_t nb_entries;
};

static const struct json_obj_descr json_flash_creds_list_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_flash_creds_list,
				 creds,
				 FLASH_CREDS_SLOTS_MAX_COUNT,
				 nb_entries,
				 json_flash_cred_entry_descr,
				 ARRAY_SIZE(json_flash_cred_entry_descr))};

static bool flash_creds_list_cb(struct flash_cred_buf *cred,
				flash_cred_status_t status,
				void *user_data)
{
	struct json_flash_creds_list *arr = user_data;

	if (status != FLASH_CRED_UNALLOCATED) {
		struct json_flash_cred_entry *entry = &arr->creds[arr->nb_entries++];

		entry->slot	= flash_cred_get_slot_from_addr(cred);
		entry->id	= cred_id_to_str(cred->header.id);
		entry->format	= cred_format_to_str(cred->header.format);
		entry->strength = cred->header.strength;
		entry->version	= cred->header.version;
		entry->size	= cred->header.size;
	}

	return true;
}

int rest_flash_credentials_list(http_request_t *req, http_response_t *resp)
{
	int ret;

	struct json_flash_creds_list arr;
	arr.nb_entries = 0u;

	ret = flash_creds_iterate(flash_creds_list_cb, &arr);

	return rest_encode_response_json_array(resp, &arr, json_flash_creds_list_descr);
}

#endif

#define MY_ARRAY_SIZE 4u

struct mystruct_obj {
	uint32_t a;
	/* Uncomment following line to get an encoding error */
	uint32_t b;
	uint32_t n;
};

static const struct json_obj_descr descr_mystruct_obj[] = {
	JSON_OBJ_DESCR_PRIM(struct mystruct_obj, a, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct mystruct_obj, n, JSON_TOK_NUMBER),
};

struct mystruct_arr {
	struct mystruct_obj items[MY_ARRAY_SIZE];
	size_t count;
};

static const struct json_obj_descr descr_mystruct_arr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(struct mystruct_arr,
				 items,
				 MY_ARRAY_SIZE,
				 count,
				 descr_mystruct_obj,
				 ARRAY_SIZE(descr_mystruct_obj))};

void test(void)
{
	char buf[0x400];

	struct mystruct_arr arr = {.count = 0u};

	for (uint8_t i = 0u; i < MY_ARRAY_SIZE; i++) {
		arr.items[i].a = 3;
		arr.items[i].b = 0xFFFFFFFF;
		arr.items[i].n = i;
		arr.count++;
	}

	json_arr_encode_buf(descr_mystruct_arr, &arr, buf, sizeof(buf));

	LOG_HEXDUMP_WRN(buf, strlen(buf), "JSON");
}

int rest_demo_json(http_request_t *req, http_response_t *resp)
{
	struct mystruct_arr arr = {.count = 0u};

	for (uint8_t i = 0u; i < MY_ARRAY_SIZE; i++) {
		arr.items[i].a = 3;
		// arr.items[i].b = 0xFFFFFFFF;
		arr.items[i].n = i;
		arr.count++;
	}

	int ret = json_arr_encode_buf(
		descr_mystruct_arr, &arr, resp->buffer.data, resp->buffer.size);

	if (ret == 0) {
		resp->buffer.filling = strlen(resp->buffer.data);

		http_response_set_content_length(resp, resp->buffer.filling);
	}

	return ret;

	// return rest_encode_response_json_array(
	// 	resp, &arr, descr_mystruct_arr
	// );
}