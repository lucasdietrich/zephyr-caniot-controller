#include "http_request.h"

#include "utils.h"

#include "http_conn.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_req, LOG_LEVEL_INF);

/* parsing */

#define CONNECTION_FROM_PARSER(p_parser) \
        ((http_connection_t *) \
        CONTAINER_OF(p_parser, http_connection_t, parser))


int on_message_begin(struct http_parser *parser)
{
        LOG_DBG("on_message_begin (%d)", 0);
        return 0;
}

int on_url(struct http_parser *parser, const char *at, size_t length)
{
        http_connection_t *conn = CONNECTION_FROM_PARSER(parser);
        if (length >= sizeof(conn->req->url)) {
                LOG_ERR("conn (%p) URL too long (%d >= %u)",
                        conn, length, sizeof(conn->req->url));

                return -EINVAL;
        }

        LOG_HEXDUMP_DBG(at, length, "url");

        memcpy(conn->req->url, at, length);
        conn->req->url[length] = '\0';
        conn->req->url_len = length;

        conn->req->method = parser->method;
                
        return 0;
}

int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
        http_connection_t *conn = CONNECTION_FROM_PARSER(parser);
        if (strncicmp(at, "Connection", length) == 0) {
                conn->parsing_header = HEADER_CONNECTION;
        } else if (strncicmp(at, "Authorization", length) == 0) {
                conn->parsing_header = HEADER_AUTH;
        } else {
                conn->parsing_header = HEADER_NONE;
        }
        return 0;
}

int on_header_value(struct http_parser *parser, const char *at, size_t length)
{
        http_connection_t *conn = CONNECTION_FROM_PARSER(parser);

        if ((conn->parsing_header == HEADER_CONNECTION) &&
            (strncicmp(at, "keep-alive", length) == 0)) {
                LOG_INF("(%p) Header Keep-alive found !", conn);
                conn->keep_alive.enabled = 1;
        }
        return 0;
}

int on_headers_complete(struct http_parser *parser)
{
        LOG_DBG("on_headers_complete (%d)", 0);
        return 0;
}

int on_body(struct http_parser *parser, const char *at, size_t length)
{
        /* can be called several times */
        struct http_request *req = CONNECTION_FROM_PARSER(parser)->req;
        if (req->payload.loc == NULL) {
                req->payload.loc = at;
                req->payload.len = 0;
        } else {
                req->payload.len += length;
        }
        
        LOG_DBG("on_body at=%p len=%u (content-len = %llu)",
                at, length, parser->content_length);
        return 0;
}

int on_message_complete(struct http_parser *parser)
{
        CONNECTION_FROM_PARSER(parser)->complete = 1;

        LOG_DBG("on_message_complete (%d)", 0);
        return 0;
}

int on_chunk_header(struct http_parser *parser)
{
        LOG_DBG("on_chunk_header (%d)", 0);
        return 0;
}

int on_chunk_complete(struct http_parser *parser)
{
        LOG_DBG("on_chunk_complete (%d)", 0);
        return 0;
}

const struct http_parser_settings settings = {
        .on_status = NULL, /* no status for requests */
        .on_url = on_url,
        .on_header_field = on_header_field,
        .on_header_value = on_header_value,
        .on_headers_complete = NULL,
        .on_message_begin = on_message_begin,
        .on_message_complete = on_message_complete,
        .on_body = on_body,

        /* not supported for now */
        .on_chunk_header = NULL,
        .on_chunk_complete = NULL
};
