#ifndef _REST_SERVER_H_
#define _REST_SERVER_H_

#include <stdint.h>

#include <data/json.h>
#include <net/http_parser.h>

#include "http_utils.h"
#include "http_request.h"

int rest_encode_response_json(const struct json_obj_descr *descr,
                              size_t descr_len, const void *val,
                              struct http_response *resp);

int rest_encode_response_json_array(const struct json_obj_descr *descr,
				    size_t descr_len, const void *val,
				    struct http_response *resp);


/*___________________________________________________________________________*/

int rest_index(struct http_request *req,
               struct http_response *resp);

int rest_info(struct http_request *req,
              struct http_response *resp);

int rest_caniot_records(struct http_request *req,
			struct http_response *resp);

int rest_xiaomi_records(struct http_request *req,
			struct http_response *resp);

int rest_devices_list(struct http_request *req,
		      struct http_response *resp);

int rest_caniot_info(struct http_request *req,
		     struct http_response *resp);

int rest_caniot_command(struct http_request *req,
			struct http_response *resp);

int rest_caniot_query_telemetry(struct http_request *req,
				struct http_response *resp);

#endif