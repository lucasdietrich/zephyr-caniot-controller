#include "rest_server.h"

#include <kernel.h>
#include <assert.h>

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
#include "ha/data.h"

#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <ha/caniot_controller.h>
#include <ha/utils.h>

#if defined(CONFIG_UART_IPC)
#include <uart_ipc/ipc.h>
#endif /* CONFIG_UART_IPC */

#include "system.h"
#include "config.h"
#include "net_time.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(rest_server, LOG_LEVEL_DBG);

#define REST_CANIOT_QUERY_MAX_TIMEOUT_MS		(5000U)

#define FIELD_SET(ret, n) (((ret) & (1 << (n))) != 0)

#define route_arg_get http_request_route_arg_get

int rest_encode_response_json(http_response_t *resp, const void *val,
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

		/* set response "content-length" */
		resp->buffer.filling = json_len;

		ret = json_obj_encode_buf(descr, descr_len, val,
					  resp->buffer.data, resp->buffer.size);
	}

exit:
	return ret;
}

int rest_encode_response_json_array(http_response_t *resp, const void *val,
				    const struct json_obj_descr *descr,
				    size_t descr_len)
{
	int ret = -EINVAL;

	if (!resp || !resp->buffer.data || !http_code_has_payload(resp->status_code)) {
		LOG_WRN("unexpected payload for code %d or buffer not set !",
			resp->status_code);
		goto exit;
	}

	ret = json_arr_encode_buf(descr,
				  val,
				  resp->buffer.data,
				  resp->buffer.size);

	if (ret == 0) {
		/* set response content-length */
		resp->buffer.filling = strlen(resp->buffer.data);
	}

exit:
	return ret;
}

/*____________________________________________________________________________*/


int rest_index(http_request_t *req,
	       http_response_t *resp)
{
	resp->status_code = 200;
	resp->buffer.filling = 0;

	return 0;
}

/*____________________________________________________________________________*/

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

int rest_info(http_request_t *req,
	      http_response_t *resp)
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
	return rest_encode_response_json(resp, &data, info_descr, ARRAY_SIZE(info_descr));
}

/*____________________________________________________________________________*/

struct json_device_base
{
	// const char *device_name;

	// const char *datetime;
	// int32_t rel_time;

	uint32_t timestamp;
};

/*____________________________________________________________________________*/

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

int rest_xiaomi_records(http_request_t *req,
			http_response_t *resp)
{
	struct xiaomi_records_encoding_context ctx;

	ctx.arr.count = 0;

	ha_dev_xiaomi_iterate(xiaomi_device_cb, &ctx);

	return rest_encode_response_json_array(resp, &ctx.arr, json_xiaomi_record_array_descr,
					       ARRAY_SIZE(json_xiaomi_record_array_descr));
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

/* todo, rename to json_caniot_telemetry */
struct json_caniot_telemetry
{
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
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_caniot_telemetry, "timestamp", base.timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, dio, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, pdio, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_caniot_telemetry, temperatures, HA_CANIOT_MAX_TEMPERATURES, temperatures_count,
		json_caniot_temperature_record_descr, ARRAY_SIZE(json_caniot_temperature_record_descr)),
};


struct json_caniot_telemetry_array
{
	struct json_caniot_telemetry records[HA_CANIOT_MAX_DEVICES];
	size_t count;
};

const struct json_obj_descr json_caniot_telemetry_array_descr[] = {
  JSON_OBJ_DESCR_OBJ_ARRAY(struct json_caniot_telemetry_array, records, HA_CANIOT_MAX_DEVICES,
	count, json_caniot_telemetry_descr, ARRAY_SIZE(json_caniot_telemetry_descr))
};

struct caniot_records_encoding_context
{
	struct json_caniot_telemetry_array arr;
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
	struct json_caniot_telemetry *const rec = &ctx->arr.records[ctx->arr.count];
	struct ha_caniot_blt_dataset *const dt = &dev->data.caniot;

	rec->base.timestamp = dev->data.measurements_timestamp;
	rec->did = (uint32_t)dev->addr.mac.addr.caniot;
	rec->temperatures_count = 0U;
	rec->dio = dev->data.caniot.dio;
	rec->pdio = dev->data.caniot.pdio;

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

	ctx->arr.count++;
}

int rest_caniot_records(http_request_t *req,
			http_response_t *resp)
{
	struct caniot_records_encoding_context ctx;

	ctx.arr.count = 0;

	ha_dev_caniot_iterate(caniot_device_cb, &ctx);

	return rest_encode_response_json_array(resp, &ctx.arr, json_caniot_telemetry_array_descr,
					       ARRAY_SIZE(json_caniot_telemetry_array_descr));
}

struct json_device_repr {
	uint32_t index;
	const char *addr_repr;
	uint32_t registered_timestamp;
	uint32_t last_measurement;

	// TODO stats
};

int rest_devices_list(http_request_t *req,
		      http_response_t *resp)
{
	// TODO

	return -EINVAL;
}

int rest_caniot_info(http_request_t *req,
		     http_response_t *resp)
{
	return -EINVAL;
}

int rest_caniot_command(http_request_t *req,
			http_response_t *resp)
{
	// const uint8_t did = 24;
	// const uint8_t enpoint = CANIOT_ENDPOINT_BOARD_CONTROL;

	return -EINVAL;
}
int rest_test_caniot_query_telemetry(http_request_t *req,
				     http_response_t *resp)
{
	struct caniot_frame query, response;

	uint64_t buf = 1U;

	caniot_build_query_command(&query, CANIOT_ENDPOINT_APP, (uint8_t *)&buf, sizeof(buf));
	const caniot_did_t did = CANIOT_DID(CANIOT_DEVICE_CLASS0, CANIOT_DEVICE_SID4);

	uint32_t timeout = 1000U;
	ha_ciot_ctrl_query(&query, &response, did, &timeout);

	return 0;
}

/*____________________________________________________________________________*/

#if defined(CONFIG_CANIOT_CONTROLLER)

int rest_devices_garage_get(http_request_t *req,
			    http_response_t *resp)
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

int rest_devices_garage_post(http_request_t *req,
			     http_response_t *resp)
{
	int ret;
	struct json_garage_post post;

	int map = json_obj_parse(req->payload.loc, req->payload.len,
				 json_garage_post_descr,
				 ARRAY_SIZE(json_garage_post_descr),
				 &post);

	if (map > 0) {
		struct ha_dev_garage_cmd cmd;
		ha_dev_garage_cmd_init(&cmd);

		if (FIELD_SET(map, 0U) && ((ret = ha_parse_ss_command(post.left_door)) > 0)) {
			cmd.actuate_left = 1U;
		}

		if (FIELD_SET(map, 1U) && ((ret = ha_parse_ss_command(post.right_door)) > 0)) {
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
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_caniot_telemetry, "timestamp", base.timestamp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, duration, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, dio, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_telemetry, pdio, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct json_caniot_telemetry, temperatures, HA_CANIOT_MAX_TEMPERATURES, temperatures_count,
		json_caniot_temperature_record_descr, ARRAY_SIZE(json_caniot_temperature_record_descr)),
};

/*
[lucas@fedora stm32f429zi-caniot-controller]$ python3 scripts/api.py
<Response [200]>
200
{'did': 32, 'dio': 240, 'duration': 2006, 'pdio': 0, 'temperatures': []}
*/

static int json_format_caniot_telemetry_resp(struct caniot_frame *r,
					     http_response_t *resp,
					     uint32_t timeout)
{
	struct ha_caniot_blt_dataset blt;
	ha_data_can_to_blt(&blt, AS_BOARD_CONTROL_TELEMETRY(r->buf));

	struct json_caniot_telemetry json = {
		.did = CANIOT_DID(r->id.cls, r->id.sid),
		.base = {
			.timestamp = net_time_get(),
		},
		.duration = timeout,
		.dio = blt.dio,
		.pdio = blt.dio,
		.temperatures_count = 0U, /* TODO temperatures */
	};

	char temp_repr[HA_CANIOT_MAX_TEMPERATURES][9U];
	for (size_t i = 0; i < HA_CANIOT_MAX_TEMPERATURES; i++) {
		if (blt.temperatures[i].type == HA_DEV_SENSOR_TYPE_NONE) {
			continue;
		}

		const size_t j = json.temperatures_count++;

		sprintf(temp_repr[j], "%.2f",
			blt.temperatures[i].value / 100.0);

		json.temperatures[j].repr = temp_repr[j];
		json.temperatures[j].sens_type = blt.temperatures[i].type;
		json.temperatures[j].value = blt.temperatures[i].value;
	}

	resp->status_code = 200U;

	return rest_encode_response_json(resp, &json, json_caniot_query_telemetry_descr,
					 ARRAY_SIZE(json_caniot_query_telemetry_descr));
}

/* Example:
{
  "addr": 8208,
  "repr": "2010",
  "value": 0
}
*/

struct json_caniot_attr {
	uint32_t key;
	char *key_repr;
	uint32_t value;
	char *value_repr;
};

static const struct json_obj_descr json_caniot_attr_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, key, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, key_repr, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, value, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr, value_repr, JSON_TOK_STRING),
};

static int json_format_caniot_attr_resp(struct caniot_frame *r,
					http_response_t *resp,
					uint32_t timeout)
{
	char key_repr[sizeof("0xFFFF")];
	char val_repr[sizeof("0xFFFFFFFF")];

	snprintf(key_repr, sizeof(key_repr), "0x%04X", r->attr.key);
	snprintf(val_repr, sizeof(val_repr), "0x%04X", r->attr.val);

	struct json_caniot_attr json = {
		.key = r->attr.key,
		.key_repr = key_repr,
		.value = r->attr.val,
		.value_repr = val_repr,
	};

	rest_encode_response_json(resp, &json, json_caniot_attr_descr,
				  ARRAY_SIZE(json_caniot_attr_descr));

	return 0;
}

/* QUERY CANIOT COMMAND/TELEMETRY and BUILD JSON RESPONSE */
int caniot_q_ct_to_json_resp(struct caniot_frame *q,
			     caniot_did_t did,
			     uint32_t *timeout,
			     http_response_t *resp)
{
	struct caniot_frame r;

	int ret = ha_ciot_ctrl_query(q, &r, did, timeout);

	switch (ret) {
	case 1:
		/* Ok */
		resp->status_code = 200U;
		ret = json_format_caniot_telemetry_resp(&r, resp, *timeout);
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

int caniot_q_attr_to_json_resp(struct caniot_frame *q,
			       caniot_did_t did,
			       uint32_t *timeout,
			       http_response_t *resp)
{
	struct caniot_frame r;

	int ret = ha_ciot_ctrl_query(q, &r, did, timeout);

	switch (ret) {
	case 1:
		/* Ok */
		resp->status_code = 200U;
		ret = json_format_caniot_attr_resp(&r, resp, *timeout);
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

int rest_devices_caniot_telemetry(http_request_t *req,
				  http_response_t *resp)
{
	/* get ids */
	uint32_t did = 0, ep = 0;
	route_arg_get(req, 0U, &did);
	route_arg_get(req, 1U, &ep);

	/* build CANIOT query */
	struct caniot_frame q;
	caniot_build_query_telemetry(&q, ep);

	/* execute and build appropriate response */
	uint32_t timeout = MIN(req->timeout_ms, REST_CANIOT_QUERY_MAX_TIMEOUT_MS);
	int ret = caniot_q_ct_to_json_resp(&q, did, &timeout, resp);
	LOG_INF("GET /devices/caniot/%u/endpoints/%u/telemetry -> %d [in %u ms]", did, ep, ret, timeout);

	return 0;
}


struct json_caniot_blcommand_post {
	const char *coc1;
	const char *coc2;
	const char *crl1;
	const char *crl2;
};

const struct json_obj_descr json_caniot_blcommand_post_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_caniot_blcommand_post, coc1, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_blcommand_post, coc2, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_blcommand_post, crl1, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_caniot_blcommand_post, crl2, JSON_TOK_STRING),
};

int rest_devices_caniot_command(http_request_t *req,
				http_response_t *resp)
{
	int ret = 0;
	struct json_caniot_blcommand_post post;

	int map = json_obj_parse(req->payload.loc, req->payload.len,
				 json_caniot_blcommand_post_descr,
				 ARRAY_SIZE(json_caniot_blcommand_post_descr),
				 &post);

	/* if no commands are given, we do nothing */
	if (map <= 0) {
		resp->status_code = 400U;
		goto exit;
	}

	/* build command */
	struct caniot_board_control_command cmd;
	caniot_board_control_command_init(&cmd);

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

	/* parse did, ep */
	uint32_t did = 0, ep = 0;
	route_arg_get(req, 0U, &did);
	route_arg_get(req, 1U, &ep);

	/* build CANIOT query */
	struct caniot_frame q;
	caniot_build_query_command(&q, ep, (uint8_t *)&cmd, sizeof(cmd));

	/* execute and build appropriate response */
	uint32_t timeout = MIN(req->timeout_ms, REST_CANIOT_QUERY_MAX_TIMEOUT_MS);
	ret = caniot_q_ct_to_json_resp(&q, did, &timeout, resp);

	LOG_INF("GET /devices/caniot/%u/endpoints/%u/command -> %d [in %u ms]", did, ep, ret, timeout);

exit:
	return ret;
}

int rest_devices_caniot_attr_read(http_request_t *req,
				  http_response_t *resp)
{
	int ret = 0;
	uint32_t did = 0, key = 0;
	route_arg_get(req, 0U, &did);
	route_arg_get(req, 1U, &key);

	/* If doesn't fit in a uint16_t, we reject */
	if ((key > 0xFFFFLU) || (did > CANIOT_DID_MAX_VALUE)) {
		resp->status_code = 400U;
		goto exit;
	}

	struct caniot_frame q;
	caniot_build_query_read_attribute(&q, (uint16_t)key);

	uint32_t timeout = MIN(req->timeout_ms, REST_CANIOT_QUERY_MAX_TIMEOUT_MS);
	ret = caniot_q_attr_to_json_resp(&q, did, &timeout, resp);

exit:
	return ret;
}

struct json_caniot_attr_write_value {
	uint32_t value;
};

const struct json_obj_descr json_caniot_attr_write_value_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_caniot_attr_write_value, value, JSON_TOK_NUMBER),
};

int rest_devices_caniot_attr_write(http_request_t *req,
				   http_response_t *resp)
{
	int ret = 0;
	uint32_t did = 0, key = 0;
	route_arg_get(req, 0U, &did);
	route_arg_get(req, 1U, &key);

	/* default status code */
	resp->status_code = 400U;

	/* If doesn't fit in a uint16_t, we reject */
	if ((key > 0xFFFFLU) || (did > CANIOT_DID_MAX_VALUE)) {
		goto exit;
	}

	/* try to parse content */
	uint32_t value;
	int map = json_obj_parse(req->payload.loc, req->payload.len,
				 json_caniot_blcommand_post_descr,
				 ARRAY_SIZE(json_caniot_blcommand_post_descr),
				 &value);
	if ((map > 0) && FIELD_SET(map, 0U)) {
		struct caniot_frame q;
		caniot_build_query_write_attribute(&q, (uint16_t)key, value);

		uint32_t timeout = MIN(req->timeout_ms, REST_CANIOT_QUERY_MAX_TIMEOUT_MS);
		ret = caniot_q_attr_to_json_resp(&q, did, &timeout, resp);
	}

exit:
	return ret;
}

#endif /* CONFIG_CANIOT_CONTROLLER */