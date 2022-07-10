#include "http_request.h"

#include "utils.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(http_req, LOG_LEVEL_DBG);

/* parsing */

#define REQUEST_FROM_PARSER(p_parser) \
        ((http_request_t *) \
        CONTAINER_OF(p_parser, http_request_t, parser))



int on_message_begin(struct http_parser *parser)
{
        LOG_DBG("on_message_begin (%d)", 0);
        return 0;
}

int on_url(struct http_parser *parser, const char *at, size_t length)
{
	http_request_t *req = REQUEST_FROM_PARSER(parser);

        if (length >= sizeof(req->url)) {
                LOG_ERR("(%p) URL too long (%d >= %u)",
                        req, length, sizeof(req->url));

                return -EINVAL;
        }

        LOG_HEXDUMP_DBG(at, length, "url");

        memcpy(req->url, at, length);
        req->url[length] = '\0';
        req->url_len = length;

        req->method = parser->method;
                
        return 0;
}

struct http_request_header
{
	/* header anem "Timeout", ... */
	const char *name;

	uint16_t len;

	int (*handler)(http_request_t *req,
		       const struct http_request_header *hdr,
		       const char *value);
};

#define header http_request_header

typedef int (*header_value_handler_t)(http_request_t *req,
				      const struct header *hdr,
				      const char *value);

#define HEADER(_name, _handler) \
	{ \
		.name = _name, \
		.len = sizeof(_name) - 1, \
		.handler = _handler, \
	}

static int header_default_handler(http_request_t *req,
				  const struct header *hdr,
				  const char *value)
{
	return 0;
}

static int header_conn_handler(http_request_t *req,
			       const struct header *hdr,
			       const char *value)
{
#define KEEPALIVE_STR "keep-alive"

	if ((strncicmp(value, KEEPALIVE_STR, strlen(KEEPALIVE_STR)) == 0)) {
		LOG_INF("(%p) Header Keep-alive found !", req);
		req->keep_alive = 1;
	}

	return 0;
}

static int header_timeout_handler(http_request_t *req,
				  const struct header *hdr,
				  const char *value)
{
	uint32_t timeout_ms;
	if (sscanf(value, "%u", &timeout_ms) == 1) {
		LOG_INF("(%p) Timeout-ms : %u ms", req, timeout_ms);
		req->timeout_ms = timeout_ms;
	}

	return 0;
}

static int header_transfer_encoding_handler(http_request_t *req,
					    const struct header *hdr,
					    const char *value)
{
#define CHUNKED_TRANSFERT_STR "chunked"

	if ((strncicmp(value, CHUNKED_TRANSFERT_STR, strlen(CHUNKED_TRANSFERT_STR)) == 0)) {
		LOG_INF("(%p) Header Transfer Encoding (= chunked) found !", req);
		req->chunked = 1U;
	}

	return 0;
}

static int header_content_type_handler(http_request_t *req,
				       const struct header *hdr,
				       const char *value)
{
#define CONTENT_TYPE_MULTIPART_STR "multipart/form-data"

	if (strncicmp(value, CONTENT_TYPE_MULTIPART_STR, strlen(CONTENT_TYPE_MULTIPART_STR) == 0)) {
		LOG_INF("(%p) Content-Type " CONTENT_TYPE_MULTIPART_STR, req);
		req->content_type = HTTP_CONTENT_TYPE_MULTIPART;
	}

	return 0;
}

static const struct header headers[] = {
	HEADER("Connection", header_conn_handler),
	HEADER("Authorization", header_default_handler),
	HEADER("Timeout-ms", header_timeout_handler),
	HEADER("Transfer-Encoding", header_transfer_encoding_handler),
	HEADER("Content-Type", header_content_type_handler),
};

int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	req->_parsing_cur_header = NULL;

	/* iterate over "headers" array and check if "at" matches the header name */
	for (size_t i = 0; i < ARRAY_SIZE(headers); i++) {
		const struct header *h = &headers[i];

		if (length == strlen(h->name) &&
		    !strncicmp(at, h->name, length)) {
			req->_parsing_cur_header = h;
			break;
		}
	}

        return 0;
}

int on_header_value(struct http_parser *parser, const char *at, size_t length)
{
        http_request_t *const req = REQUEST_FROM_PARSER(parser);

	const struct header *const hdr = req->_parsing_cur_header;

	int ret = 0;

	if (hdr != NULL) {
		ret = hdr->handler(req, hdr, at);
	}

	return ret;
}

int on_headers_complete(struct http_parser *parser)
{
	
	http_request_t *const req = REQUEST_FROM_PARSER(parser);
	
	/* TODO if multipart or content length to long or chunk -> stream */

	/* HERE we should decide to process the request as a stream or a message */
	bool process_as_stream = false;



	LOG_DBG("on_headers_complete (%d)", 0);
	return 0;
}

int on_body(struct http_parser *parser, const char *at, size_t length)
{
        /* can be called several times */
        http_request_t *req = REQUEST_FROM_PARSER(parser);

        if (req->payload.loc == NULL) {
		req->payload.loc = (char *)at;
                req->payload.len = 0;
        } else {
                req->payload.len += length;
        }
        
        LOG_DBG("on_body at=%p len=%u (content-len = %llu)",
                at, length, parser->content_length);

	LOG_HEXDUMP_DBG(at, length, "body");
        return 0;
}

int on_message_complete(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);
	
	req->complete = 1U;

        LOG_DBG("on_message_complete (%d)", 0);
        return 0;
}

int on_chunk_header(struct http_parser *parser)
{
        LOG_DBG("on_chunk_header (%p)", parser);
        return 0;
}

int on_chunk_complete(struct http_parser *parser)
{
        LOG_DBG("on_chunk_complete (%p)", parser);
        return 0;
}

const struct http_parser_settings settings = {
        .on_status = NULL, /* no status for requests */
        .on_url = on_url,
        .on_header_field = on_header_field,
        .on_header_value = on_header_value,
        .on_headers_complete = on_headers_complete,
        .on_message_begin = on_message_begin,
        .on_message_complete = on_message_complete,
        .on_body = on_body,

        /* not supported for now */
        .on_chunk_header = on_chunk_header,
        .on_chunk_complete = on_chunk_complete
};
