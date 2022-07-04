#include "http_request.h"

#include "utils.h"

#include "http_conn.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(http_req, LOG_LEVEL_INF);

#define header http_request_header

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

struct header;

typedef int (*header_value_handler_t)(const struct header *hdr,
				      struct http_connection *conn,
				      const char *value);

#define HEADER(_name, _handler) \
	{ \
		.name = _name, \
		.len = sizeof(_name) - 1, \
		.handler = _handler, \
	}

int header_default_handler(const struct header *hdr,
			   struct http_connection *conn,
			   const char *value)
{
	return 0;
}

int header_conn_handler(const struct header *hdr,
			struct http_connection *conn,
			const char *value)
{
#define KEEPALIVE_STR "keep-alive"

	if ((strncicmp(value, KEEPALIVE_STR, strlen(KEEPALIVE_STR)) == 0)) {
		LOG_INF("(%p) Header Keep-alive found !", conn);
		conn->keep_alive.enabled = 1;
	}

	return 0;
}

int header_timeout_handler(const struct header *hdr,
			   http_connection_t *conn,
			   const char *value)
{
	uint32_t timeout_ms;
	if (sscanf(value, "%u", &timeout_ms) == 1) {
		LOG_INF("(%p) Timeout-ms : %u ms", conn, timeout_ms);
		conn->req->timeout_ms = timeout_ms;
	}

	return 0;
}

static const struct header headers[] = {
	HEADER("Connection", header_conn_handler),
	HEADER("Authorization", header_default_handler),
	HEADER("Timeout-ms", header_timeout_handler),
};

int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
	http_connection_t *const conn = CONNECTION_FROM_PARSER(parser);

	conn->_parsing_cur_header = NULL;

	/* iterate over "headers" array and check if "at" matches the header name */
	for (size_t i = 0; i < ARRAY_SIZE(headers); i++) {
		const struct header *h = &headers[i];

		if (length == strlen(h->name) &&
		    !strncicmp(at, h->name, length)) {
			conn->_parsing_cur_header = h;
			break;
		}
	}

        return 0;
}

int on_header_value(struct http_parser *parser, const char *at, size_t length)
{
        http_connection_t *const conn = CONNECTION_FROM_PARSER(parser);
	const struct header *const hdr = conn->_parsing_cur_header;

	int ret = 0;

	if (hdr != NULL) {
		ret = hdr->handler(hdr, conn, at);
	}

	return ret;
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
		req->payload.loc = (char *)at;
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
