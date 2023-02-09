/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/http_utils.h"
#include "core/routes.h"
#include "dfu_server.h"
#include "files_server.h"
#include "prometheus_client.h"
#include "rest_server.h"
#include "test_server.h"
#include "web_server.h"

#include <embedc-url/parser.h>
#include <embedc-url/parser_internal.h>

#define REST	  ROUTE_ATTR_REST
#define HTML	  ROUTE_ATTR_HTML
#define TEXT	  ROUTE_ATTR_TEXT
#define FORM	  ROUTE_ATTR_FORM
#define MULTIPART ROUTE_ATTR_MULTIPART_FORM_DATA
#define BINARY	  ROUTE_ATTR_BINARY
#define SEC	  ROUTE_ATTR_SECURE

/*
Following code is automatically generated
=== ROUTES DEFINITION BEGIN  === */
#if defined(CONFIG_APP_HTTP_TEST_SERVER)
static const struct route_descr root_api_test_zs[] = {
	LEAF("mystr", GET, http_test_payload, NULL, 0u),
};
#endif

#if defined(CONFIG_APP_HTTP_TEST_SERVER)
static const struct route_descr root_api_test_route_args_firstzu_secondzu[] = {
	LEAF("third:u", GET | ARG_UINT, http_test_route_args, NULL, 0u),
};
#endif

#if defined(CONFIG_APP_HTTP_TEST_SERVER)
static const struct route_descr root_api_test_route_args_firstzu[] = {
	SECTION("second:u",
		ARG_UINT,
		root_api_test_route_args_firstzu_secondzu,
		ARRAY_SIZE(root_api_test_route_args_firstzu_secondzu),
		0u),
};
#endif

#if defined(CONFIG_APP_HTTP_TEST_SERVER)
static const struct route_descr root_api_test_route_args[] = {
	SECTION("first:u",
		ARG_UINT,
		root_api_test_route_args_firstzu,
		ARRAY_SIZE(root_api_test_route_args_firstzu),
		0u),
};
#endif

#if defined(CONFIG_APP_HTTP_TEST_SERVER)
static const struct route_descr root_api_test[] = {
	LEAF("any", POST, http_test_any, http_test_any, 0u),
	LEAF("messaging", POST, http_test_messaging, NULL, 0u),
	LEAF("streaming", POST, http_test_streaming, http_test_streaming, REST),
	SECTION("route_args",
		0u,
		root_api_test_route_args,
		ARRAY_SIZE(root_api_test_route_args),
		0u),
	LEAF("big_payload", POST, http_test_big_payload, NULL, BINARY),
	LEAF("headers", GET, http_test_headers, NULL, 0u),
	LEAF("payload", GET, http_test_payload, NULL, 0u),
	SECTION(":s", ARG_STR, root_api_test_zs, ARRAY_SIZE(root_api_test_zs), 0u),
};
#endif

#if defined(CONFIG_APP_CAN_INTERFACE)
static const struct route_descr root_api_if_can[] = {
	LEAF("id:x", POST | ARG_HEX, rest_if_can, NULL, 0u),
};
#endif

#if defined(CONFIG_APP_CAN_INTERFACE)
static const struct route_descr root_api_if[] = {
	SECTION("can", 0u, root_api_if_can, ARRAY_SIZE(root_api_if_can), 0u),
};
#endif

#if defined(CONFIG_APP_HA)
static const struct route_descr root_api_ha[] = {
	LEAF("stats", GET, rest_ha_stats, NULL, 0u),
};
#endif

#if defined(CONFIG_APP_HA)
static const struct route_descr root_api_device[] = {
	LEAF(":u", GET | ARG_UINT, rest_device_get, NULL, 0u),
};
#endif

#if defined(CONFIG_APP_CANIOT_CONTROLLER)
static const struct route_descr root_api_devices_caniot_didzu_attribute[] = {
	LEAF("key:x", GET | ARG_HEX, rest_devices_caniot_attr_read_write, NULL, 0u),
	LEAF("key:x", PUT | ARG_HEX, rest_devices_caniot_attr_read_write, NULL, 0u),
};
#endif

#if defined(CONFIG_APP_CANIOT_CONTROLLER)
static const struct route_descr root_api_devices_caniot_didzu_endpoint_epzu[] = {
	LEAF("telemetry", GET, rest_devices_caniot_telemetry, NULL, 0u),
	LEAF("command", POST, rest_devices_caniot_command, NULL, 0u),
};
#endif

#if defined(CONFIG_APP_CANIOT_CONTROLLER)
static const struct route_descr root_api_devices_caniot_didzu_endpoint_blc[] = {
	LEAF("command", POST, rest_devices_caniot_blc_command, NULL, 0u),
};
#endif

#if defined(CONFIG_APP_CANIOT_CONTROLLER)
static const struct route_descr root_api_devices_caniot_didzu_endpoint_blc1[] = {
	LEAF("command", POST, rest_devices_caniot_blc1_command, NULL, 0u),
};
#endif

#if defined(CONFIG_APP_CANIOT_CONTROLLER)
static const struct route_descr root_api_devices_caniot_didzu_endpoint_blc0[] = {
	LEAF("command", POST, rest_devices_caniot_blc0_command, NULL, 0u),
};
#endif

#if defined(CONFIG_APP_CANIOT_CONTROLLER)
static const struct route_descr root_api_devices_caniot_didzu_endpoint[] = {
	SECTION("blc0",
		0u,
		root_api_devices_caniot_didzu_endpoint_blc0,
		ARRAY_SIZE(root_api_devices_caniot_didzu_endpoint_blc0),
		0u),
	SECTION("blc1",
		0u,
		root_api_devices_caniot_didzu_endpoint_blc1,
		ARRAY_SIZE(root_api_devices_caniot_didzu_endpoint_blc1),
		0u),
	SECTION("blc",
		0u,
		root_api_devices_caniot_didzu_endpoint_blc,
		ARRAY_SIZE(root_api_devices_caniot_didzu_endpoint_blc),
		0u),
	SECTION("ep:u",
		ARG_UINT,
		root_api_devices_caniot_didzu_endpoint_epzu,
		ARRAY_SIZE(root_api_devices_caniot_didzu_endpoint_epzu),
		0u),
};
#endif

#if defined(CONFIG_APP_CANIOT_CONTROLLER)
static const struct route_descr root_api_devices_caniot_didzu[] = {
	SECTION("endpoint",
		0u,
		root_api_devices_caniot_didzu_endpoint,
		ARRAY_SIZE(root_api_devices_caniot_didzu_endpoint),
		0u),
	SECTION("attribute",
		0u,
		root_api_devices_caniot_didzu_attribute,
		ARRAY_SIZE(root_api_devices_caniot_didzu_attribute),
		0u),
	LEAF("reboot", POST, rest_devices_caniot_blc_action, NULL, 0u),
	LEAF("factory_reset", POST, rest_devices_caniot_blc_action, NULL, 0u),
};
#endif

#if defined(CONFIG_APP_CANIOT_CONTROLLER)
static const struct route_descr root_api_devices_caniot[] = {
#if defined(CONFIG_APP_HA)
	LEAF("", GET, rest_caniot_records, NULL, 0u),
#endif
	SECTION("did:u",
		ARG_UINT,
		root_api_devices_caniot_didzu,
		ARRAY_SIZE(root_api_devices_caniot_didzu),
		0u),
};
#endif

#if defined(CONFIG_APP_CANIOT_CONTROLLER) || defined(CONFIG_APP_HA)
static const struct route_descr root_api_devices[] = {
#if defined(CONFIG_APP_HA)
	LEAF("", GET, rest_devices_list, NULL, 0u),
#endif
#if defined(CONFIG_APP_HA)
	LEAF("xiaomi", GET, rest_xiaomi_records, NULL, 0u),
#endif
	LEAF("garage", GET, rest_devices_garage_get, NULL, 0u),
#if defined(CONFIG_APP_CANIOT_CONTROLLER)
	LEAF("garage", POST, rest_devices_garage_post, NULL, 0u),
#endif
#if defined(CONFIG_APP_CANIOT_CONTROLLER)
	SECTION("caniot",
		0u,
		root_api_devices_caniot,
		ARRAY_SIZE(root_api_devices_caniot),
		0u),
#endif
};
#endif

#if defined(CONFIG_APP_HA)
static const struct route_descr root_api_room[] = {
	LEAF(":u", GET | ARG_UINT, rest_room_devices_list, NULL, 0u),
};
#endif

static const struct route_descr root_api_http[] = {
	LEAF("stats", GET, rest_http_stats, NULL, REST),
};

#if defined(CONFIG_APP_DFU)
static const struct route_descr root_api_dfu[] = {
#if defined(CONFIG_APP_DFU)
	LEAF("", POST, http_dfu_image_upload, http_dfu_image_upload_response, MULTIPART),
#endif
#if defined(CONFIG_APP_DFU)
	LEAF("", GET, http_dfu_status, NULL, 0u),
#endif
	LEAF("status", GET, http_dfu_status, NULL, 0u),
};
#endif

static const struct route_descr root_api_demo[] = {
	LEAF("json", GET, rest_demo_json, NULL, 0u),
};

#if defined(CONFIG_LUA)
static const struct route_descr root_api_lua[] = {
	LEAF("execute", POST, rest_lua_run_script, NULL, 0u),
};
#endif

static const struct route_descr root_api_files_1zs_2zs_3zs[] = {
	LEAF("", POST, http_file_upload, http_file_upload, REST),
	LEAF("4:s", POST | ARG_STR, http_file_upload, http_file_upload, REST),
	LEAF("", GET, http_file_download, NULL, BINARY),
	LEAF("4:s", GET | ARG_STR, http_file_download, NULL, BINARY),
	LEAF("", DELETE, http_file_delete, NULL, REST),
	LEAF("4:s", DELETE | ARG_STR, http_file_delete, NULL, REST),
};

static const struct route_descr root_api_files_1zs_2zs[] = {
	LEAF("", POST, http_file_upload, http_file_upload, REST),
	SECTION("3:s",
		ARG_STR,
		root_api_files_1zs_2zs_3zs,
		ARRAY_SIZE(root_api_files_1zs_2zs_3zs),
		REST),
	LEAF("", GET, http_file_download, NULL, BINARY),
	LEAF("", DELETE, http_file_delete, NULL, REST),
};

static const struct route_descr root_api_files_1zs[] = {
	LEAF("", POST, http_file_upload, http_file_upload, REST),
	SECTION("2:s",
		ARG_STR,
		root_api_files_1zs_2zs,
		ARRAY_SIZE(root_api_files_1zs_2zs),
		REST),
	LEAF("", GET, http_file_download, NULL, BINARY),
	LEAF("", DELETE, http_file_delete, NULL, REST),
};

static const struct route_descr root_api_files[] = {
	LEAF("", POST, http_file_upload, http_file_upload, REST),
	SECTION("1:s", ARG_STR, root_api_files_1zs, ARRAY_SIZE(root_api_files_1zs), REST),
	LEAF("", GET, http_file_download, NULL, BINARY),
	LEAF("", DELETE, http_file_delete, NULL, REST),
#if defined(CONFIG_LUA)
	LEAF("lua", GET, rest_fs_list_lua_scripts, NULL, 0u),
#endif
#if defined(CONFIG_LUA)
	LEAF("lua", DELETE, rest_fs_remove_lua_script, NULL, 0u),
#endif
};

#if defined(CONFIG_APP_CREDS_FLASH)
static const struct route_descr root_api_credentials[] = {
	LEAF("flash", GET, rest_flash_credentials_list, NULL, REST),
};
#endif

static const struct route_descr root_api_interface[] = {
	LEAF("", GET, rest_interfaces_list, NULL, 0u),
	LEAF("idx:u", GET | ARG_UINT, rest_interface, NULL, 0u),
	LEAF("idx:u", POST | ARG_UINT, rest_interface_set, NULL, 0u),
};

static const struct route_descr root_api[] = {
	LEAF("info", GET, rest_info, NULL, 0u),
	SECTION("interface", 0u, root_api_interface, ARRAY_SIZE(root_api_interface), 0u),
#if defined(CONFIG_APP_CREDS_FLASH)
	SECTION("credentials",
		0u,
		root_api_credentials,
		ARRAY_SIZE(root_api_credentials),
		REST),
#endif
	LEAF("files_fetch", GET, web_server_files_html, NULL, HTML),
	SECTION("files", 0u, root_api_files, ARRAY_SIZE(root_api_files), REST),
#if defined(CONFIG_LUA)
	SECTION("lua", 0u, root_api_lua, ARRAY_SIZE(root_api_lua), 0u),
#endif
	SECTION("demo", 0u, root_api_demo, ARRAY_SIZE(root_api_demo), 0u),
#if defined(CONFIG_APP_DFU)
	SECTION("dfu", 0u, root_api_dfu, ARRAY_SIZE(root_api_dfu), 0u),
#endif
	SECTION("http", 0u, root_api_http, ARRAY_SIZE(root_api_http), REST),
#if defined(CONFIG_APP_HA)
	SECTION("room", 0u, root_api_room, ARRAY_SIZE(root_api_room), 0u),
#endif
#if defined(CONFIG_APP_CANIOT_CONTROLLER) && defined(CONFIG_APP_HA)
	SECTION("devices", 0u, root_api_devices, ARRAY_SIZE(root_api_devices), 0u),
#endif
#if defined(CONFIG_APP_HA)
	SECTION("device", 0u, root_api_device, ARRAY_SIZE(root_api_device), 0u),
#endif
#if defined(CONFIG_APP_HA)
	SECTION("ha", 0u, root_api_ha, ARRAY_SIZE(root_api_ha), 0u),
#endif
#if defined(CONFIG_APP_CAN_INTERFACE)
	SECTION("if", 0u, root_api_if, ARRAY_SIZE(root_api_if), 0u),
#endif
#if defined(CONFIG_APP_HTTP_TEST_SERVER)
	SECTION("test", 0u, root_api_test, ARRAY_SIZE(root_api_test), 0u),
#endif
};

static const struct route_descr root[] = {
	LEAF("", GET, web_server_index_html, NULL, HTML),
	LEAF("index.html", GET, web_server_index_html, NULL, HTML),
#if defined(CONFIG_APP_HA)
	LEAF("metrics", GET, prometheus_metrics, NULL, TEXT),
#endif
	LEAF("metrics_controller", GET, prometheus_metrics_controller, NULL, TEXT),
	LEAF("metrics_demo", GET, prometheus_metrics_demo, NULL, TEXT),
	SECTION("api", 0u, root_api, ARRAY_SIZE(root_api), 0u),
};

/* === ROUTES DEFINITION END === */

const struct route_descr *const routes_root = root;
const size_t routes_root_size		    = ARRAY_SIZE(root);