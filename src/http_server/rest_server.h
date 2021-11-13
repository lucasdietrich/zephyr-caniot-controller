#ifndef _REST_SERVER_H_
#define _REST_SERVER_H_

#include <stdint.h>

#include <data/json.h>
#include <net/http_parser.h>

#include "http_utils.h"
#include "http_request.h"

typedef int (*rest_handler_t) (struct http_request *req,
                               struct http_response *resp);

struct rest_ressource
{
        const char *route;
        size_t route_len;

        enum http_method method;
        rest_handler_t handler;
};

#define REST_RESSOURCE(m, r, h) \
{ \
        .route = r, \
        .route_len = sizeof(r) - 1, \
        .method = m, \
        .handler = h, \
}

rest_handler_t rest_resolve(struct http_request *req);

int rest_encode_response_json(const struct json_obj_descr *descr,
                              size_t descr_len, const void *val,
                              struct http_response *resp);

/*___________________________________________________________________________*/

int rest_index(struct http_request *req,
               struct http_response *resp);

int rest_info(struct http_request *req,
              struct http_response *resp);
                 
#endif