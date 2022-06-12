#include "rest_server.h"

#include <kernel.h>

#include <string.h>

#include <net/http_parser.h>

#include <utils.h>
#include <posix/time.h>
#include <net/net_if.h>
#include <net/net_ip.h>
#include <net/ethernet.h>

#include <net/net_stats.h>
#include <net/net_mgmt.h>
#include <net/net_if.h>

#include <mbedtls/memory_buffer_alloc.h>

#include <bluetooth/addr.h>
#include "ha/devices.h"

#include <caniot/caniot.h>

#include "uart_ipc/ipc.h"

#include "system.h"
#include "config.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(rest_server, LOG_LEVEL_DBG);

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

/* base on : net_if / struct net_if_ipv4 */
struct json_info_iface
{
	const char *ethernet_mac;
	const char *unicast;
	const char *mcast;
	const char *gateway;
	const char *netmask;
};

static const struct json_obj_descr json_info_iface_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_info_iface, ethernet_mac, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_info_iface, unicast, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_info_iface, mcast, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_info_iface, gateway, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_info_iface, netmask, JSON_TOK_STRING),
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
	JSON_OBJ_DESCR_PRIM(struct net_stats, processing_error,
			    JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, bytes, net_stats_bytes_descr),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, ip_errors,
			      net_stats_ip_errors_descr),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, ipv4, net_stats_ip_descr),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, icmp, net_stats_icmp_descr),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, tcp, net_stats_tcp_descr),
	JSON_OBJ_DESCR_OBJECT(struct net_stats, udp, net_stats_udp_descr),
};

#if defined(CONFIG_SYSTEM_MONITORING)
struct json_info_mbedtls_stats
{
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
#endif /* CONFIG_SYSTEM_MONITORING */

#if defined(CONFIG_UART_IPC_STATS)

// generate json descriptor for ipc_stats struct
static const struct json_obj_descr info_ipc_stats_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, rx.bytes, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, rx.frames, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, rx.discarded_bytes, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, rx.malformed, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, rx.crc_errors, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, rx.seq_gap, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, rx.frames_lost, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, rx.seq_reset, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, rx.dropped_frames, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, rx.unsupported_ver, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, tx.bytes, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, tx.frames, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, tx.retries, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, tx.errors, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, ping.rx, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, ping.tx, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct ipc_stats, misc.mem_alloc_fail, JSON_TOK_NUMBER),
};
#endif /* defined(CONFIG_UART_IPC_STATS) */

struct json_info
{
	uint32_t uptime;
	uint32_t timestamp;
	struct json_info_controller_status status;
	struct json_info_iface interface;
	struct net_stats net_stats;
	
#if defined(CONFIG_SYSTEM_MONITORING)
	struct json_info_mbedtls_stats mbedtls_stats;
#endif

#if defined(CONFIG_UART_IPC_STATS)
	struct ipc_stats ipc_stats;
#endif
};

static const struct json_obj_descr info_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_info, uptime, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_info, timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT(struct json_info, status, info_controller_status_descr),
	JSON_OBJ_DESCR_OBJECT(struct json_info, interface, json_info_iface_descr),
	JSON_OBJ_DESCR_OBJECT(struct json_info, net_stats, net_stats_descr),
#if defined(CONFIG_SYSTEM_MONITORING)
	JSON_OBJ_DESCR_OBJECT(struct json_info, mbedtls_stats, info_mbedtls_stats_descr),
#endif
#if defined(CONFIG_UART_IPC_STATS)
	JSON_OBJ_DESCR_OBJECT(struct json_info, ipc_stats, info_ipc_stats_descr),
#endif
};

#define ETH_ALEN sizeof(struct net_eth_addr)
#define ETH_STR_LEN sizeof("FF:FF:FF:FF:FF:FF")

int rest_info(struct http_request *req,
	      struct http_response *resp)
{
	struct json_info data;

	/* get time */
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	/* get iterface info */
	struct net_if_config *const ifcfg = &net_if_get_default()->config;
	char unicast_str[NET_IPV4_ADDR_LEN] = "";
	char mcast_str[NET_IPV4_ADDR_LEN] = "";
	char gateway_str[NET_IPV4_ADDR_LEN] = "";
	char netmask_str[NET_IPV4_ADDR_LEN] = "";
	char ethernet_mac_str[ETH_STR_LEN] = "";

	net_addr_ntop(AF_INET,
		      &ifcfg->ip.ipv4->unicast[0].address.in_addr,
		      unicast_str, sizeof(unicast_str));
	net_addr_ntop(AF_INET,
		      &ifcfg->ip.ipv4->mcast[0].address.in_addr,
		      mcast_str, sizeof(mcast_str));
	net_addr_ntop(AF_INET,
		      &ifcfg->ip.ipv4->gw,
		      gateway_str, sizeof(gateway_str));
	net_addr_ntop(AF_INET,
		      &ifcfg->ip.ipv4->netmask,
		      netmask_str, sizeof(netmask_str));

	struct net_linkaddr *l2_addr = net_if_get_link_addr(net_if_get_default());
	if (l2_addr->type == NET_LINK_ETHERNET) {
		sprintf(ethernet_mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
			l2_addr->addr[0], l2_addr->addr[1], l2_addr->addr[2],
			l2_addr->addr[3], l2_addr->addr[4], l2_addr->addr[5]);
	}

	/* get network stats */
	net_mgmt(NET_REQUEST_STATS_GET_ALL, net_if_get_default(),
		 &data.net_stats, sizeof(struct net_stats));

	/* system status */
	const controller_status_t status = {
		.atomic_val = atomic_get(&controller_status.atomic)
	};

	data.uptime = k_uptime_get() / MSEC_PER_SEC;
	data.timestamp = (uint32_t)ts.tv_sec;
	data.interface.ethernet_mac = ethernet_mac_str;
	data.interface.unicast = unicast_str;
	data.interface.mcast = mcast_str;
	data.interface.gateway = gateway_str;
	data.interface.netmask = netmask_str;
	data.status.has_ipv4_addr = status.has_ipv4_addr;
	data.status.valid_system_time = status.valid_system_time;

	/* mbedtls stats */
#if defined(CONFIG_SYSTEM_MONITORING)
	mbedtls_memory_buffer_alloc_cur_get(&data.mbedtls_stats.cur_used,
					    &data.mbedtls_stats.cur_blocks);
	mbedtls_memory_buffer_alloc_max_get(&data.mbedtls_stats.max_used,
					    &data.mbedtls_stats.max_blocks);
#endif 

#if defined(CONFIG_UART_IPC_STATS)
	/* ipc stats */
	ipc_stats_get(&data.ipc_stats);
#endif

	/* rencode response */
	return rest_encode_response_json(info_descr, ARRAY_SIZE(info_descr),
					 &data, resp);
}

/*___________________________________________________________________________*/

struct json_device_base
{
	// const char *device_name;

	// const char *datetime;
	// int32_t rel_time;

	uint32_t timestamp;
};

/*___________________________________________________________________________*/

struct json_xiaomi_record
{
	char *bt_mac;

	struct json_device_base base;

	int32_t rssi;

	char *temperature; /* °C */
	int32_t temperature_raw; /* 1e-2 °C */
	uint32_t humidity; /* 1e-2 % */
	uint32_t battery_level; /* % */
	uint32_t battery_voltage; /* mV */
};

static const struct json_obj_descr json_xiaomi_record_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, bt_mac, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, base.timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, rssi, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, temperature, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, temperature_raw, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, humidity, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, battery_level, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_xiaomi_record, battery_voltage, JSON_TOK_NUMBER),
};


struct json_xiaomi_record_array
{
	struct json_xiaomi_record records[HA_XIAOMI_MAX_DEVICES];
	size_t count;
};

const struct json_obj_descr json_xiaomi_record_array_descr[] = {
  JSON_OBJ_DESCR_OBJ_ARRAY(struct json_xiaomi_record_array, records, HA_XIAOMI_MAX_DEVICES,
	count, json_xiaomi_record_descr, ARRAY_SIZE(json_xiaomi_record_descr))
};

struct xiaomi_records_encoding_context
{
	struct json_xiaomi_record_array arr;
	struct {
		char addr[BT_ADDR_LE_STR_LEN];
		char temperature[9];
	} strings[HA_XIAOMI_MAX_DEVICES];
};

static void xiaomi_device_cb(ha_dev_t *dev,
			     void *user_data)
{
	struct xiaomi_records_encoding_context *ctx =
		(struct xiaomi_records_encoding_context *)user_data;

	struct ha_xiaomi_dataset *const data = &dev->data.xiaomi;
	struct json_xiaomi_record *const json = &ctx->arr.records[ctx->arr.count];

	json->bt_mac = ctx->strings[ctx->arr.count].addr;
	json->rssi = data->rssi;
	json->temperature = ctx->strings[ctx->arr.count].temperature;
	json->temperature_raw = data->temperature.value;
	json->humidity = data->humidity;
	json->battery_level = data->battery_level;
	json->battery_voltage = data->battery_mv;
	json->base.timestamp = dev->data.measurements_timestamp;

	bt_addr_le_to_str(&dev->addr.mac.addr.ble,
			  json->bt_mac,
			  BT_ADDR_LE_STR_LEN);
	sprintf(json->temperature,
		"%.2f",
		data->temperature.value / 100.0);

	ctx->arr.count++;
}

int rest_xiaomi_records(struct http_request *req,
			struct http_response *resp)
{
	struct xiaomi_records_encoding_context ctx;

	ctx.arr.count = 0;

	ha_dev_xiaomi_iterate(xiaomi_device_cb, &ctx);

	return rest_encode_response_json_array(json_xiaomi_record_array_descr,
					       ARRAY_SIZE(json_xiaomi_record_array_descr),
					       &ctx.arr, resp);
}

struct json_caniot_temperature_record
{
	const char *repr;
	int32_t value;
	uint32_t sens_type;
};

static const struct json_obj_descr json_caniot_temperature_record_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_caniot_temperature_record, repr, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_temperature_record, value, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_temperature_record, sens_type, JSON_TOK_NUMBER),
};

struct json_caniot_record
{
	uint32_t did;

	struct json_device_base base;

	struct json_caniot_temperature_record temperatures[HA_CANIOT_MAX_TEMPERATURES];
	uint32_t temperatures_count;
	uint32_t dio;
};

static const struct json_obj_descr json_caniot_record_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_caniot_record, did, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_caniot_record, "timestamp", base.timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_caniot_record, temperatures, HA_CANIOT_MAX_TEMPERATURES, temperatures_count,
		json_caniot_temperature_record_descr, ARRAY_SIZE(json_caniot_temperature_record_descr)),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_record, dio, JSON_TOK_NUMBER),
};


struct json_caniot_record_array
{
	struct json_caniot_record records[HA_CANIOT_MAX_DEVICES];
	size_t count;
};

const struct json_obj_descr json_caniot_record_array_descr[] = {
  JSON_OBJ_DESCR_OBJ_ARRAY(struct json_caniot_record_array, records, HA_CANIOT_MAX_DEVICES,
	count, json_caniot_record_descr, ARRAY_SIZE(json_caniot_record_descr))
};

struct caniot_records_encoding_context
{
	struct json_caniot_record_array arr;
	/* x = devices
	 * y = temperatures per device
	 * z = string length
	 */
	char temp_repr[HA_CANIOT_MAX_DEVICES][HA_CANIOT_MAX_TEMPERATURES][9U]; 
};

static void caniot_device_cb(ha_dev_t *dev,
			     void *user_data)
{
	struct caniot_records_encoding_context *const ctx =
		(struct caniot_records_encoding_context *)user_data;
	struct json_caniot_record *const rec = &ctx->arr.records[ctx->arr.count];
	struct ha_caniot_dataset *const dt = &dev->data.caniot;

	rec->base.timestamp = dev->data.measurements_timestamp;
	rec->did = (uint32_t) dev->addr.mac.addr.caniot;
	rec->temperatures_count = 0U;

	/* encode temperatures */
	for (size_t i = 0; i < HA_CANIOT_MAX_TEMPERATURES; i++) {
		/* if temperature is valid */
		if (dt->temperatures[i].type != HA_DEV_SENSOR_TYPE_NONE) {
			const size_t j = rec->temperatures_count;

			sprintf(ctx->temp_repr[ctx->arr.count][j],
				"%.2f",
				dt->temperatures[i].value / 100.0);

			rec->temperatures[j].repr = ctx->temp_repr[ctx->arr.count][j];
			rec->temperatures[j].sens_type = dt->temperatures[i].type;
			rec->temperatures[j].value = dt->temperatures[i].value;
			rec->temperatures_count++;
		}
	}

	rec->dio = dev->data.caniot.dio;

	ctx->arr.count++;
}

int rest_caniot_records(struct http_request *req,
			struct http_response *resp)
{
	struct caniot_records_encoding_context ctx;

	ctx.arr.count = 0;

	ha_dev_caniot_iterate(caniot_device_cb, &ctx);

	return rest_encode_response_json_array(json_caniot_record_array_descr,
					       ARRAY_SIZE(json_caniot_record_array_descr),
					       &ctx.arr, resp);
}

struct json_device_repr {
	uint32_t index;
	const char *addr_repr;
	uint32_t registered_timestamp;
	uint32_t last_measurement;

	// TODO stats
};

int rest_devices_list(struct http_request *req,
		      struct http_response *resp)
{
	// TODO

	return -EINVAL;
}

int rest_caniot_info(struct http_request *req,
		     struct http_response *resp)
{
	return -EINVAL;
}

int rest_caniot_command(struct http_request *req,
			struct http_response *resp)
{
	// const uint8_t did = 24;
	// const uint8_t enpoint = CANIOT_ENDPOINT_BOARD_CONTROL;

	return -EINVAL;
}

int rest_caniot_query_telemetry(struct http_request *req,
				struct http_response *resp)
{
	// const uint8_t did = 24;
	// const uint8_t enpoint = CANIOT_ENDPOINT_BOARD_CONTROL;
	
	return -EINVAL;
}