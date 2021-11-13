#include "rest_server.h"

#include <kernel.h>

#include <string.h>

#include <net/http_parser.h>

#define REST REST_RESSOURCE

#define DELETE HTTP_DELETE
#define GET HTTP_GET
#define POST HTTP_POST
#define PUT HTTP_PUT

static const struct rest_ressource map[] = {
        REST(GET, "", rest_index),
        REST(GET, "/", rest_index),
};

static inline const struct rest_ressource* map_last(void) 
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

int rest_index(struct http_request *req,
               struct http_response *resp)
{
        resp->status_code = 200;
        resp->content_len = 0;

        return 0;
}