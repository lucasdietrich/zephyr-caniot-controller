#include "http_conn.h"

#include "http_server.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_conn, LOG_LEVEL_DBG);

/*___________________________________________________________________________*/

/* connections pool */

int conns_count = 0;
static struct connection pool[MAX_CONNECTIONS];

/* all active connections stacked a the top of the array */
static struct connection *alloc[MAX_CONNECTIONS] = {
        &pool[0], &pool[1], &pool[2]
};

struct connection *get_connection(int index)
{
        __ASSERT((0 <= index) && (index < ARRAY_SIZE(pool)),
                 "index out of range");

        return alloc[index];
}

int conn_get_index(struct connection *conn)
{
        for (uint_fast8_t i = 0; i < conns_count; i++) {
                if (alloc[i] == conn) {
                        return i;
                }
        }
        return -1;
}

struct connection *alloc_connection(void)
{
        struct connection *conn =  NULL;
        
        if (conns_count < MAX_CONNECTIONS) {
                conn = alloc[conns_count++];
                clear_conn(conn);
        }

        return conn;
}

void free_connection(struct connection *conn)
{
        if (conn == NULL) {
                return;
        }

        int index = conn_get_index(conn);
        int move_count = MAX_CONNECTIONS - index - 1;
        if (move_count > 0) {
                memmove(&alloc[index],
                        &alloc[index + 1],
                        move_count * sizeof(struct connection *));

                /* put the freed connection at the end of the list */
                alloc[MAX_CONNECTIONS - 1] = conn; 
        }
        conns_count--;
}

bool conn_is_closed(struct connection *conn)
{
        return conn_get_index(conn) == -1;
}

void clear_conn(struct connection *conn)
{
        http_parser_init(&conn->parser, HTTP_REQUEST);

        conn->req = NULL;
        conn->resp = NULL;

        conn->complete = 0;
        conn->keep_alive = 1;
}

/*___________________________________________________________________________*/

/* parsing */

#define CONNECTION_FROM_PARSER(p_parser) \
        ((struct connection *) \
        CONTAINER_OF(p_parser, struct connection, parser))


int on_message_begin(struct http_parser *parser)
{
        LOG_DBG("on_message_begin (%d)", 0);
        return 0;
}

int on_url(struct http_parser *parser, const char *at, size_t length)
{
        struct connection *conn = CONNECTION_FROM_PARSER(parser);
        if (length >= sizeof(conn->req->url)) {
                LOG_ERR("conn (%p) URL too long (%d >= %u)",
                        conn, length, sizeof(conn->req->url));

                return -EINVAL;
        }

        LOG_HEXDUMP_DBG(at, length, "url");

        memcpy(conn->req->url, at, length);
        conn->req->url[length] = '\0';
                
        return 0;
}

int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
        struct connection *conn = CONNECTION_FROM_PARSER(parser);
        if (strncmp(at, "Connection", length) == 0) {
                conn->parsing_header = HEADER_CONNECTION;
        } else if (strncmp(at, "Connection", length) == 0) {
                conn->parsing_header = HEADER_AUTH;
        } else {
                conn->parsing_header = HEADER_NONE;
        }
        return 0;
}

int on_header_value(struct http_parser *parser, const char *at, size_t length)
{
        struct connection *conn = CONNECTION_FROM_PARSER(parser);

        if ((conn->parsing_header == HEADER_CONNECTION) &&
            (strncmp(at, "keep-alive", length) == 0)) {
                LOG_INF("(%p) Header Keep-alive found !", conn);
                conn->keep_alive = 1;
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
