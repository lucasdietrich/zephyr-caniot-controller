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

/*_____________________________________________________________________________________*/

/* forward declarations of static functions */
static int on_header_field(struct http_parser *parser, const char *at, size_t length);
static int on_header_value(struct http_parser *parser, const char *at, size_t length);
static int on_headers_complete(struct http_parser *parser);
static int on_message_begin(struct http_parser *parser);
static int on_body_messaging(struct http_parser *parser, const char *at, size_t length);
static int on_body_streaming(struct http_parser *parser, const char *at, size_t length);
static int on_body_discarding(struct http_parser *parser, const char *at, size_t length);
static int on_message_complete(struct http_parser *parser);
static int on_url(struct http_parser *parser, const char *at, size_t length);
// static int on_status(struct http_parser *parser, const char *at, size_t length);
static int on_chunk_header(struct http_parser *parser);
static int on_chunk_complete(struct http_parser *parser);



/*_____________________________________________________________________________________*/
const struct http_parser_settings parser_settings_messaging = {
	.on_status = NULL, /* no status for requests */
	.on_header_field = on_header_field,
	.on_url = on_url,
	.on_header_value = on_header_value,
	.on_headers_complete = on_headers_complete,
	.on_message_begin = on_message_begin,
	.on_message_complete = on_message_complete,
	.on_body = on_body_messaging,

	.on_chunk_header = NULL,
	.on_chunk_complete = NULL
};

const struct http_parser_settings parser_settings_streaming = {
	.on_status = NULL, /* no status for requests */
	.on_url = on_url,
	.on_header_field = on_header_field,
	.on_header_value = on_header_value,
	.on_headers_complete = on_headers_complete,
	.on_message_begin = on_message_begin,
	.on_message_complete = on_message_complete,
	.on_body = on_body_streaming,

	.on_chunk_header = on_chunk_header,
	.on_chunk_complete = on_chunk_complete
};

const struct http_parser_settings parser_settings_discarding = {
	.on_status = NULL, /* no status for requests */
	.on_url = on_url,
	.on_header_field = on_header_field,
	.on_header_value = on_header_value,
	.on_headers_complete = on_headers_complete,
	.on_message_begin = on_message_begin,
	.on_message_complete = on_message_complete,
	.on_body = on_body_discarding,

	.on_chunk_header = NULL,
	.on_chunk_complete = NULL
};

/*_____________________________________________________________________________________*/

void http_request_init(http_request_t *req)
{
	/* TODO optimize the way the request is initialized */
	memset(req, 0x00U, sizeof(http_request_t));

	req->chunk.loc = NULL;
	req->chunk.len = 0U;
	req->chunk.id = 0U;

	req->keep_alive = 0U;
	req->timeout_ms = 0U;
	req->content_type = HTTP_CONTENT_TYPE_NONE;

	req->complete = 0U;
	req->headers_complete = 0U;
	req->handling_method = HTTP_REQUEST_AS_MESSAGE;

	req->payload.len = 0U;
	req->payload.loc = NULL;

	req->parsed_content_length = 0U;

	req->route = 0;
	req->calls_count = 0;

	http_parser_init(&req->parser, HTTP_REQUEST);
	req->parser_settings = &parser_settings_messaging;
}


int http_request_handle_received_data(http_request_t *req,
				      const char *data,
				      size_t len)
{
	return http_parser_execute(&req->parser, req->parser_settings, data, len);
}

void http_request_mark_discarded(http_request_t *req,
				http_request_discard_reason_t reason)
{
	req->handling_method = HTTP_REQUEST_DISCARD;
	req->discard_reason = reason;
	req->parser_settings = &parser_settings_discarding;
}

/*_____________________________________________________________________________________*/

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

static int header_keepalive_handler(http_request_t *req,
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
		req->handling_method = HTTP_REQUEST_AS_STREAM;
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
	HEADER("Connection", header_keepalive_handler),
	HEADER("Authorization", header_default_handler),
	HEADER("Timeout-ms", header_timeout_handler),
	HEADER("Transfer-Encoding", header_transfer_encoding_handler),
	HEADER("Content-Type", header_content_type_handler),
	HEADER("Content-Length", header_content_length_handler),
};

static int on_header_field(struct http_parser *parser, const char *at, size_t length)
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

static int on_header_value(struct http_parser *parser, const char *at, size_t length)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	const struct header *const hdr = req->_parsing_cur_header;

	int ret = 0;

	if (hdr != NULL) {
		ret = hdr->handler(req, hdr, at);
	}

	return ret;
}

static int on_headers_complete(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	/* Resolve route as we enough information */
	const struct http_route *route =
		route_resolve(req->method, req->url,
			      req->url_len, &req->route_args);

	if (route == NULL) {
		http_request_mark_discarded(req, HTTP_REQUEST_ROUTE_UNKNOWN);
		LOG_WRN("(%p) Route not found, discarding ...", req);
	} else if (route->handler == NULL) {
		http_request_mark_discarded(req, HTTP_REQUEST_ROUTE_NO_HANDLER);
		LOG_WRN("(%p) Route missing handler, discarding ...", req);
	} else if (http_request_is_stream(req) && !route->support_stream) {
		http_request_mark_discarded(req, HTTP_REQUEST_STREAMING_UNSUPPORTED);
		LOG_WRN("(%p) Stream requested but route does not support it, discarding ...",
			req);
	}

	if (http_request_is_stream(req)) {
		req->parser_settings = &parser_settings_streaming;
	}

	req->route = route;
	req->headers_complete = 1U;

	/* We update the connection keep_alive configuration
	 * based on the request.
	 */
	req->_conn->keep_alive.enabled = req->keep_alive;

	LOG_INF("on_headers_complete, handling_method = %u",
		req->handling_method);

	return 0;
}

static int on_body_streaming(struct http_parser *parser,
			     const char *at,
			     size_t length)
{
	/* can be called several times */
	http_request_t *req = REQUEST_FROM_PARSER(parser);

	/* set chunk location */
	req->chunk.loc = (char *)at;
	req->chunk.len = length;

	/* route is necessarily valid at this point */
	int ret = req->route->handler(req, NULL);
	if (ret == 0) {
		req->chunk._offset += length;
	} else {
		http_request_mark_discarded(req, HTTP_REQUEST_STREAM_PROCESSING_ERROR);
		LOG_ERR("Stream processing error %d, discarding ...", ret);
	}

	req->calls_count++;

	LOG_DBG("on_body at=%p len=%u", at, length);

	return 0;
}

static int on_body_messaging(struct http_parser *parser,
			  const char *at,
			  size_t length)
{
	/* can be called several times */
	http_request_t *req = REQUEST_FROM_PARSER(parser);

	if (req->payload.loc == NULL) {
		/* If not a stream request and this is the first call of the callbaclk,
		 * we must mark the beginning of the request body (and initialize the length)
		 */
		req->payload.loc = (char *)at;
		req->payload.len = length;

	} else {
		__ASSERT_NO_MSG(req->payload.loc + req->payload.len == (char *)at);

		req->payload.len += length;
		req->len += length;
	}

	LOG_DBG("on_body at=%p len=%u (content-len = %llu)",
		at, length, parser->content_length);

	return 0;
}

static int on_body_discarding(struct http_parser *parser,
			   const char *at,
			   size_t length)
{
	ARG_UNUSED(at);

	http_request_t *req = REQUEST_FROM_PARSER(parser);

	req->len += length;

	LOG_WRN("on_body DISCARDING %u bytes", length);

	return 0;
}

static int on_message_complete(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	req->complete = 1U;

	LOG_INF("on_message_complete, message received len=%u (%p)",
		req->len, req);

	return 0;
}

static int on_chunk_header(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	__ASSERT_NO_MSG(http_request_is_stream(req) == true);

	/* reset chunk vars */
	req->chunk._offset = 0U;
	req->chunk.len = 0U;
	req->chunk.loc = NULL;

	LOG_DBG("on_chunk_header chunk=%u (%p)", req->chunk.id, req);

	return 0;
}

static int on_chunk_complete(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	__ASSERT_NO_MSG(http_request_is_stream(req) == true);

	/* increment chunk id */
	req->chunk.id++;

	req->len += req->chunk._offset;

	LOG_DBG("on_chunk_complete chunk=%u len=%u (%p)",
		req->chunk.id, req->chunk.len, parser);

	return 0;
}