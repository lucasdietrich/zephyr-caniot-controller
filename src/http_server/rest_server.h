#ifndef _REST_SERVER_H_
#define _REST_SERVER_H_

#include <stdint.h>

#include <data/json.h>
#include <net/http_parser.h>

#include "http_utils.h"
#include "http_request.h"

int rest_encode_response_json(struct http_response *resp,
			      const void *val,
			      const struct json_obj_descr *descr,
			      size_t descr_len);

int rest_encode_response_json_array(struct http_response *resp,
				    const void *val,
				    const struct json_obj_descr *descr,
				    size_t descr_len);

/*___________________________________________________________________________*/

/* That saves time but it's a bad practice for sure :') */
#define REST_HANDLE_DECL(name) \
	int rest_##name(struct http_request *req, struct http_response *resp)

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

int rest_devices_garage_get(struct http_request *req,
			    struct http_response *resp);

int rest_devices_garage_post(struct http_request *req,
			     struct http_response *resp);

int rest_devices_caniot_telemetry(struct http_request *req,
				  struct http_response *resp);

int rest_devices_caniot_command(struct http_request *req,
				struct http_response *resp);

int rest_devices_caniot_attr_read(struct http_request *req,
				  struct http_response *resp);

int rest_devices_caniot_attr_write(struct http_request *req,
				   struct http_response *resp);

#endif