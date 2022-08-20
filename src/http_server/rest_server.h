/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _REST_SERVER_H_
#define _REST_SERVER_H_

#include <stdint.h>

#include <data/json.h>
#include <net/http_parser.h>

#include "http_utils.h"
#include "http_request.h"
#include "http_response.h"

int rest_encode_response_json(http_response_t *resp,
			      const void *val,
			      const struct json_obj_descr *descr,
			      size_t descr_len);

int rest_encode_response_json_array(http_response_t *resp,
				    const void *val,
				    const struct json_obj_descr *descr,
				    size_t descr_len);


/* That saves time but it's a bad practice for sure :') */
#define REST_HANDLE_DECL(name) \
	int rest_##name(http_request_t *req, http_response_t *resp)

int rest_index(http_request_t *req,
               http_response_t *resp);

int rest_info(http_request_t *req,
              http_response_t *resp);

int rest_caniot_records(http_request_t *req,
			http_response_t *resp);

int rest_xiaomi_records(http_request_t *req,
			http_response_t *resp);

int rest_devices_list(http_request_t *req,
		      http_response_t *resp);

int rest_caniot_info(http_request_t *req,
		     http_response_t *resp);

int rest_caniot_command(http_request_t *req,
			http_response_t *resp);

int rest_test_caniot_query_telemetry(http_request_t *req,
				     http_response_t *resp);

int rest_devices_garage_get(http_request_t *req,
			    http_response_t *resp);

int rest_devices_garage_post(http_request_t *req,
			     http_response_t *resp);

int rest_devices_caniot_telemetry(http_request_t *req,
				  http_response_t *resp);

int rest_devices_caniot_command(http_request_t *req,
				http_response_t *resp);

int rest_devices_caniot_attr_read(http_request_t *req,
				  http_response_t *resp);

int rest_devices_caniot_attr_write(http_request_t *req,
				   http_response_t *resp);

int rest_fs_list_lua_scripts(http_request_t *req,
			     http_response_t *resp);

int rest_fs_remove_lua_script(http_request_t *req,
			      http_response_t *resp);

int rest_lua_run_script(http_request_t *req,
			http_response_t *resp);

#endif