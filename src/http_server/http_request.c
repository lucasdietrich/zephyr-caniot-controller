#include "http_request.h"

#include "utils.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "http_utils.h"
#include "http_server.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(http_req, LOG_LEVEL_INF);

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
#define CONTENT_TYPE_APPLICATION_OCTET_STREAM_STR "application/octet-stream"
#define CONTENT_TYPE_MULTIPART_FORM_DATA_STR "multipart/form-data"

	if (strncicmp(value, CONTENT_TYPE_APPLICATION_OCTET_STREAM_STR,
		      strlen(CONTENT_TYPE_APPLICATION_OCTET_STREAM_STR) == 0)) {
		LOG_INF("(%p) Content-Type " CONTENT_TYPE_APPLICATION_OCTET_STREAM_STR, req);
		req->content_type = HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM;
	} else if (strncicmp(value, CONTENT_TYPE_MULTIPART_FORM_DATA_STR,
			     strlen(CONTENT_TYPE_MULTIPART_FORM_DATA_STR) == 0)) {
		LOG_INF("(%p) Content-Type " CONTENT_TYPE_MULTIPART_FORM_DATA_STR, req);
		req->content_type = HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA;
	}

	return 0;
}

static int header_content_length_handler(http_request_t *req,
					 const struct header *hdr,
					 const char *value)
{

	uint32_t content_length;
	if (sscanf(value, "%u", &content_length) == 1) {
		LOG_INF("(%p) Content-length : %u B", req, content_length);
		req->parsed_content_length = content_length;
	}

	return 0;
}

static const struct header headers[] = {
	HEADER("Connection", header_conn_handler),
	HEADER("Authorization", header_default_handler),
	HEADER("Timeout-ms", header_timeout_handler),
	HEADER("Transfer-Encoding", header_transfer_encoding_handler),
	HEADER("Content-Type", header_content_type_handler),
	HEADER("Content-Length", header_content_length_handler),
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

	/* Resolve route as we enough information */
	const struct http_route *route =
		route_resolve(req->method, req->url,
			      req->url_len, req->route_args);


	if (route_is_valid(route) == true) {
		req->discard = 0U;

		/* We decide to process the request as a stream or a message */
		req->stream =
			(route->server == HTTP_FILES_SERVER) ||
			(req->content_type == HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM) ||
			(req->parsed_content_length == HTTP_REQUEST_PAYLOAD_MAX_SIZE) ||
			(req->chunked == 1U);
	} else {
		req->discard = 1U;
	}

	req->route = route;
	req->headers_complete = 1U;

	req->chunk.id = 0U;

	LOG_INF("on_headers_complete, discard=%d stream=%d",
		req->discard, req->stream);

	return 0;
}

int on_body(struct http_parser *parser, const char *at, size_t length)
{
	/* can be called several times */
	http_request_t *req = REQUEST_FROM_PARSER(parser);

	if (req->stream) {
		req->chunk.loc = (char *)at;
		req->chunk.len = length;

		__ASSERT(req->route != NULL, "route is NULL");
		__ASSERT(req->route->handler != NULL, "route handler is null");

		int ret = req->route->handler(req, NULL);
		if (ret == 0) {
			req->chunk._offset += length;
		} else {
			req->discard = 1U;
		}
	} else {
		if (req->payload.loc == NULL) {
			req->payload.loc = (char *)at;
			req->payload.len = 0;
		}

		req->payload.len += length;
	}

	LOG_DBG("on_body at=%p len=%u (content-len = %llu)",
		at, length, parser->content_length);

	// LOG_HEXDUMP_DBG(at, length, "body");
	return 0;
}

int on_message_complete(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	req->request_complete = 1U;

	LOG_INF("on_message_complete, payload len=%u (%p)", 
		req->payload.len, req);
	
	return 0;
}

int on_chunk_header(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	if (req->stream) {
		/* reset chunk vars */
		req->chunk._offset = 0U;
		req->chunk.len = 0U;
		req->chunk.loc = NULL;

		LOG_DBG("on_chunk_header chunk=%u (%p)", req->chunk.id, req);
	}

	return 0;
}

int on_chunk_complete(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	if (req->stream) {
		/* increment chunk id */
		req->chunk.id++;

		req->payload.len += req->chunk._offset;

		LOG_DBG("on_chunk_complete chunk=%u len=%u (%p)",
			req->chunk.id, req->chunk.len, parser);
	}

	return 0;
}

const struct http_parser_settings parser_settings = {
	.on_status = NULL, /* no status for requests */
	.on_url = on_url,
	.on_header_field = on_header_field,
	.on_header_value = on_header_value,
	.on_headers_complete = on_headers_complete,
	.on_message_begin = on_message_begin,
	.on_message_complete = on_message_complete,
	.on_body = on_body,

	.on_chunk_header = on_chunk_header,
	.on_chunk_complete = on_chunk_complete
};
