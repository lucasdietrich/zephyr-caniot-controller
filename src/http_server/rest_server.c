#include "rest_server.h"

#include <kernel.h>

#include <string.h>

#include <net/http_parser.h>

#include <utils.h>
#include <posix/time.h>
#include <net/net_if.h>
#include <net/net_ip.h>
#include <net/ethernet.h>

#include <bluetooth/addr.h>
#include "../devices_controller.h"

#include "system.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(rest_server, LOG_LEVEL_DBG);

#define REST REST_RESSOURCE

#define DELETE HTTP_DELETE
#define GET HTTP_GET
#define POST HTTP_POST
#define PUT HTTP_PUT

static const struct rest_ressource map[] = {
	REST(GET, "", rest_index),
	REST(GET, "/", rest_index),
	REST(GET, "/info", rest_info),
	REST(GET, "/records/xiaomi", rest_xiaomi_records),
};

static inline const struct rest_ressource *map_last(void)
{
	return &map[ARRAY_SIZE(map) - 1];
}

static bool url_match(const struct rest_ressource *res,
		      const char *url, size_t url_len)
{
	size_t check_len = MAX(res->route_len, url_len);

	return strncmp(res->route, url, check_len) == 0;
}

rest_handler_t rest_resolve(struct http_request *req)
{
	for (const struct rest_ressource *res = map; res <= map_last(); res++) {
		if (res->method != req->method) {
			continue;
		}

		if (url_match(res, req->url, req->url_len)) {
			return res->handler;
		}
	}

	return NULL;
}

int rest_encode_response_json(const struct json_obj_descr *descr,
			      size_t descr_len, const void *val,
			      struct http_response *resp)
{
	int ret = -EINVAL;
	ssize_t json_len;

	if (!resp || !resp->buf || !http_code_has_payload(resp->status_code)) {
		LOG_WRN("unexpected payload for code %d or buffer not set !",
			resp->status_code);
		goto exit;
	}

	/* calculate needed size to encode the response */
	json_len = json_calc_encoded_len(descr, descr_len, val);

	if (json_len <= resp->buf_size) {

		/* set response content-length */
		resp->content_len = json_len;

		ret = json_obj_encode_buf(descr, descr_len, val,
					  resp->buf, resp->buf_size);
	}

exit:
	return ret;
}

int rest_encode_response_json_array(const struct json_obj_descr *descr,
				    size_t descr_len, const void *val,
				    struct http_response *resp)
{
	int ret = -EINVAL;

	if (!resp || !resp->buf || !http_code_has_payload(resp->status_code)) {
		LOG_WRN("unexpected payload for code %d or buffer not set !",
			resp->status_code);
		goto exit;
	}

	ret = json_arr_encode_buf(descr,
				  val,
				  resp->buf,
				  resp->buf_size);

	if (ret == 0) {
		/* set response content-length */
		resp->content_len = strlen(resp->buf);
	}

exit:
	return ret;
}

/*___________________________________________________________________________*/


int rest_index(struct http_request *req,
	       struct http_response *resp)
{
	resp->status_code = 200;
	resp->content_len = 0;

	return 0;
}

/*___________________________________________________________________________*/

struct json_info_controller_status
{
	bool has_ipv4_addr;
	bool valid_system_time;
};

static const struct json_obj_descr info_controller_status_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_info_controller_status, has_ipv4_addr,
			    JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM(struct json_info_controller_status, valid_system_time,
			    JSON_TOK_TRUE),
};

struct json_info
{
	uint32_t timestamp;
	const char *ip;
	const char *mac;
	struct json_info_controller_status status;
};

static const struct json_obj_descr info_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_info, timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_info, ip, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_info, mac, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct json_info, status, info_controller_status_descr),
};

#define ETH_ALEN sizeof(struct net_eth_addr)
#define ETH_STR_LEN sizeof("FF:FF:FF:FF:FF:FF")

int rest_info(struct http_request *req,
	      struct http_response *resp)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	struct net_if_config *const ifcfg = &net_if_get_default()->config;
	char ipv4_str[NET_IPV4_ADDR_LEN];
	net_addr_ntop(AF_INET,
		      &ifcfg->ip.ipv4->unicast[0].address.in_addr,
		      ipv4_str, sizeof(ipv4_str));

	/* TODO add gateway/dns/netmask/... */

	char mac_str[ETH_STR_LEN] = "";
	struct net_linkaddr *l2_addr = net_if_get_link_addr(net_if_get_default());
	if (l2_addr->type == NET_LINK_ETHERNET) {
		sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
			l2_addr->addr[0], l2_addr->addr[1], l2_addr->addr[2],
			l2_addr->addr[3], l2_addr->addr[4], l2_addr->addr[5]);
	}

	const controller_status_t status = {
		.atomic_val = atomic_get(&controller_status.atomic)
	};
	
	struct json_info data = { 
		(uint32_t)ts.tv_sec,
		ipv4_str,
		mac_str,
		.status = {
			.has_ipv4_addr = status.has_ipv4_addr,
			.valid_system_time = status.valid_system_time
		}
	 };

	return rest_encode_response_json(info_descr, ARRAY_SIZE(info_descr),
					 &data, resp);
}

/*___________________________________________________________________________*/

#define XIAOMI_MAX_DEVICES 15

struct json_xiaomi_record
{
	// const char *device_name;
	char *bt_mac;

	uint32_t timestamp;
	// const char *datetime;
	// int32_t rel_time;

	char *temperature;
	int32_t temperature_raw;
	uint32_t humidity;
	uint32_t battery_level;
};

static const struct json_obj_descr json_xiaomi_record_descr[] = {
	// JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, device_name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, bt_mac, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, timestamp, JSON_TOK_NUMBER),
	// JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, datetime, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, temperature, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, temperature_raw, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, humidity, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, battery_level, JSON_TOK_NUMBER),
};


struct json_xiaomi_record_array
{
	struct json_xiaomi_record records[XIAOMI_MAX_DEVICES];
	size_t count;
};

const struct json_obj_descr json_xiaomi_record_array_descr[] = {
  JSON_OBJ_DESCR_OBJ_ARRAY(struct json_xiaomi_record_array, records, XIAOMI_MAX_DEVICES,
	count, json_xiaomi_record_descr, ARRAY_SIZE(json_xiaomi_record_descr))
};

struct xiaomi_records_encoding_context
{
	struct json_xiaomi_record_array arr;
	struct {
		char addr[BT_ADDR_LE_STR_LEN];
		char temperature[9];
	} strings[XIAOMI_MAX_DEVICES];
};

static void rest_xiaomi_record_cb(xiaomi_record_t *rec,
				  void *user_data)
{
	struct xiaomi_records_encoding_context *ctx =
		(struct xiaomi_records_encoding_context *)user_data;

	struct json_xiaomi_record *arr_record = &ctx->arr.records[ctx->arr.count];

	arr_record->bt_mac = ctx->strings[ctx->arr.count].addr;
	arr_record->temperature = ctx->strings[ctx->arr.count].temperature;
	arr_record->temperature_raw = rec->measurements.temperature;
	arr_record->humidity = rec->measurements.humidity;
	arr_record->battery_level = rec->measurements.battery;
	arr_record->timestamp = rec->time;

	bt_addr_le_to_str(&rec->addr,
			  arr_record->bt_mac,
			  BT_ADDR_LE_STR_LEN);
	sprintf(arr_record->temperature,
		"%.2f",
		rec->measurements.temperature / 100.0);

	ctx->arr.count++;
}

int rest_xiaomi_records(struct http_request *req,
			struct http_response *resp)
{
	struct xiaomi_records_encoding_context ctx;

	ctx.arr.count = 0;

	dev_ble_xiaomi_iterate(rest_xiaomi_record_cb, &ctx);

	return rest_encode_response_json_array(json_xiaomi_record_array_descr,
					       ARRAY_SIZE(json_xiaomi_record_array_descr),
					       &ctx.arr, resp);
}