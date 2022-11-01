/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_request.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>

#include "http_utils.h"
#include "http_server.h"
#include "http_response.h"

#include "utils/buffers.h"
#include "utils/misc.h"


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(http_req, LOG_LEVEL_WRN);

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
	/* Rest of the request is initialize to 0 */
	*req = (http_request_t) {
		.content_type = HTTP_CONTENT_TYPE_NONE,
		.handling_mode = HTTP_REQUEST_MESSAGE,
		.parser_settings = &parser_settings_messaging,
	};

	sys_dlist_init(&req->headers);

	http_parser_init(&req->parser, HTTP_REQUEST);

#if defined(CONFIG_HTTP_TEST)
	http_test_init_context(&req->_test_ctx);
#endif
}

static const char *discard_reason_to_str(http_request_discard_reason_t reason)
{
	switch (reason) {
	case HTTP_REQUEST_ROUTE_UNKNOWN:
		return "Unknown route";
	case HTTP_REQUEST_BAD:
		return "Bad request";
	case HTTP_REQUEST_ROUTE_NO_HANDLER:
		return "Route has no handler";
	case HTTP_REQUEST_STREAMING_UNSUPPORTED:
		return "Streaming unsupported";
	case HTTP_REQUEST_PAYLOAD_TOO_LARGE:
		return "Payload too large";
	case HTTP_REQUEST_PROCESSING_ERROR:
		return "Processing error";
	default:
		return "<unknown discard reason>";
	}
}

static void mark_discarded(http_request_t *req,
			   http_request_discard_reason_t reason)
{
	req->handling_mode = HTTP_REQUEST_DISCARD;
	req->discard_reason = reason;
	req->parser_settings = &parser_settings_discarding;
}

/*_____________________________________________________________________________________*/

#define REQUEST_FROM_PARSER(p_parser) \
        ((http_request_t *) \
        CONTAINER_OF(p_parser, http_request_t, parser))

/* forward declaration */
static void reset_header_buffers_cursor(void);

int on_message_begin(struct http_parser *parser)
{
	http_request_t *req = REQUEST_FROM_PARSER(parser);

	LOG_DBG("(%p) on_message_begin", req);

	/* Reset headers buffer context */
	reset_header_buffers_cursor();

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

struct http_header_handler
{
	/* header anem "Timeout", ... */
	const char *name;

	uint16_t len;
	
	int (*handler)(http_request_t *req,
		       const struct http_header_handler *hdr,
		       const char *value,
		       size_t length);
};

#define header http_header_handler

typedef int (*header_value_handler_t)(http_request_t *req,
				      const struct header *hdr,
				      const char *value);

#define HEADER(_name, _handler) \
	{ \
		.name = _name, \
		.len = sizeof(_name) - 1, \
		.handler = _handler, \
	}

static int header_keepalive_handler(http_request_t *req,
				    const struct header *hdr,
				    const char *value,
				    size_t length)
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
				  const char *value,
				  size_t length)
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
					    const char *value,
					    size_t length)
{
#define CHUNKED_TRANSFERT_STR "chunked"

	if ((strncicmp(value, CHUNKED_TRANSFERT_STR, strlen(CHUNKED_TRANSFERT_STR)) == 0)) {
		LOG_INF("(%p) Header Transfer Encoding (= chunked) found !", req);
		req->handling_mode = HTTP_REQUEST_STREAM;
	}

	return 0;
}

static int header_content_type_handler(http_request_t *req,
				       const struct header *hdr,
				       const char *value,
				       size_t length)
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
					 const char *value,
					 size_t length)
{
	uint32_t content_length;
	if (sscanf(value, "%u", &content_length) == 1) {
		LOG_INF("(%p) Content-length : %u B", req, content_length);
		req->parsed_content_length = content_length;
	}

	return 0;
}




static char hdrbuf[CONFIG_HTTP_REQUEST_HEADERS_BUFFER_SIZE];
static size_t hdr_allocated = 0U;

static struct http_header *alloc_header_buffer(size_t value_size)
{
	struct http_header *hdr = NULL;
	const size_t allocsize = sizeof(struct http_header) + value_size;

	if (hdr_allocated + allocsize <= sizeof(hdrbuf)) {
		hdr = (struct http_header *)&hdrbuf[hdr_allocated];
		hdr_allocated += allocsize;
	}

	return hdr;
}

static void reset_header_buffers_cursor(void)
{
	hdr_allocated = 0U;
}

/**
 * @brief Store the header (name and value) to a buffer and keep it until 
 *  the application finishes to process the request.
 * 
 * If cannot allocate context for the header, the request is discarded.
 * 
 * Header name and value are stored until a certain size.
 * - Header name max size: 24
 * - Header value max size: 64
 * - Maximal number of headers: 8
 * 
 * @param req 
 * @param hdr 
 * @param value 
 * @return int 
 */
static int header_keep(http_request_t *req,
		       const struct header *hdr,
		       const char *value,
		       size_t length)
{
	/* We should include EOS in the length */
	struct http_header *buf = alloc_header_buffer(length + 1U);

	if (buf != NULL) {
		buf->name = hdr->name;
		memcpy(buf->value, value, length);
		buf->value[length] = '\0';
		sys_dlist_append(&req->headers, &buf->handle);
	} else {
		LOG_WRN("(%p) Cannot allocate header buffer for %s", 
			req, hdr->name);
	}

	return 0;
}




/* All headers that are not handled by a handler are discarded. */
static const struct header headers[] = {
	HEADER("Connection", header_keepalive_handler),
	HEADER("Timeout-ms", header_timeout_handler),
	HEADER("Transfer-Encoding", header_transfer_encoding_handler),
	HEADER("Content-Type", header_content_type_handler),
	HEADER("Content-Length", header_content_length_handler),

	HEADER("Authorization", header_keep),
	HEADER("App-Upload-Filepath", header_keep),
	HEADER("App-Upload-Checksum", header_keep),
	HEADER("App-Script-Filename", header_keep),

#if defined(CONFIG_HTTP_TEST_SERVER)
	HEADER("App-Test-Header1", header_keep),
	HEADER("App-Test-Header2", header_keep),
	HEADER("App-Test-Header3", header_keep),
#endif /* CONFIG_HTTP_TEST_SERVER */
};

/* TODO check that the implementation leads only to a single call to the
 * handler when a header value is parsed. */
static int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	req->_parsing_cur_header = NULL;

	/* iterate over "headers" array and check if "at" matches the header name */
	for (size_t i = 0; i < ARRAY_SIZE(headers); i++) {
		const struct header *h = &headers[i];

		if ((length == h->len) &&
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
		ret = hdr->handler(req, hdr, at, length);
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

	LOG_DBG("(%p) content-length : %u / %llu parser content-length, flags = %x",
		req, req->parsed_content_length, parser->content_length, (uint32_t)parser->flags);

	LOG_INF("(%p) Headers complete url=%s stream=%u [hdr buf %u/%u]",
		req, req->url, http_request_is_stream(req),
		hdr_allocated, CONFIG_HTTP_REQUEST_HEADERS_BUFFER_SIZE);

	/* TODO add explicit logs to know which route has not been found */
	if (route == NULL) {
		mark_discarded(req, HTTP_REQUEST_ROUTE_UNKNOWN);
		LOG_WRN("(%p) Route not found %s %s", req,
			http_method_str(req->method), req->url);
	} else if (route->resp_handler == NULL) {
		mark_discarded(req, HTTP_REQUEST_ROUTE_NO_HANDLER);
		LOG_ERR("(%p) Route has no handler %s", req, req->url);
	} else if (http_request_is_stream(req) && !route_supports_streaming(route)) {
		mark_discarded(req, HTTP_REQUEST_STREAMING_UNSUPPORTED);
		LOG_ERR("(%p) Route doesn't support streaming %s", req, req->url);
	}

	if (http_request_is_stream(req) == true) {
		req->parser_settings = &parser_settings_streaming;
	}

#if defined(CONFIG_HTTP_TEST)
	/* Here we decide if we want to test the
	 * request as a stream or a message
	 */
	req->_test_ctx.stream = http_request_is_stream(req);
#endif /* CONFIG_HTTP_TEST */

	req->route = route;
	req->headers_complete = 1U;

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
	req->payload_len += length;

#if defined(CONFIG_HTTP_TEST)
	http_test_run(&req->_test_ctx, req, NULL, HTTP_TEST_HANDLER_REQ);
#endif /* CONFIG_HTTP_TEST */

	/* route is necessarily valid at this point */
	int ret = req->route->req_handler(req, NULL);
	if (ret < 0) {
		mark_discarded(req, HTTP_REQUEST_PROCESSING_ERROR);
		LOG_ERR("(%p) Stream processing error %d", req, ret);
	}

	req->chunk._offset += length;

	req->calls_count++;

	LOG_DBG("(%p) on_body at=%p len=%u", req, at, length);

	return 0;
}

static int on_body_messaging(struct http_parser *parser,
			     const char *at,
			     size_t length)
{
	/* can be called several times */
	http_request_t *req = REQUEST_FROM_PARSER(parser);

	LOG_INF("(%p) at=%p len=%u (payload loc=%p len=%u)",
		req, at, length, req->payload.loc, req->payload.len);

	if (req->payload.loc == NULL) {
		/* If not a stream request and this is the first call of the callbaclk,
		 * we must mark the beginning of the request body (and initialize the length)
		 */
		req->payload.loc = (char *)at;
		req->payload.len = length;

	} else {
		__ASSERT_NO_MSG(req->payload.loc + req->payload.len == (char *)at);

		req->payload.len += length;
	}

	req->payload_len += length;

	return 0;
}

static int on_body_discarding(struct http_parser *parser,
			      const char *at,
			      size_t length)
{
	ARG_UNUSED(at);

	http_request_t *req = REQUEST_FROM_PARSER(parser);

	req->payload_len += length;

	LOG_DBG("(%p) on_body DISCARDING %u bytes", req, length);

	return 0;
}

static int on_message_complete(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	/* Mark request as complete */
	req->complete = 1U;

	/* Force the parser to pause and give control back to the application
	 * so that it can process it and send the response.
	 *
	 * Note: in case of bad implementation of the HTTP client :
	 * If two HTTP requests are being received and parsed in the
	 * same "http_parser_execute" call, before a reponse is sent from the current
	 * server for the 1st request. Then it means that the client sent (TCP) two HTTP
	 * requests while not waiting for the response for the 1st request.
	 * If this case happens the current server will closes the connection.
	 */
	http_parser_pause(parser, 1);

	LOG_INF("(%p) on_message_complete, message received len=%u",
		req, req->payload_len);

	return 0;
}

static int on_chunk_header(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	__ASSERT_NO_MSG(http_request_is_stream(req) == true);

	LOG_DBG("(%p) on_chunk_header chunk=%u", req, req->chunk.id);

	return 0;
}

static int on_chunk_complete(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	__ASSERT_NO_MSG(http_request_is_stream(req) == true);

	/* increment chunk id */
	req->chunk.id++;

	/* Reset chunk buffer infos, this could be done in on_chunk_header(),
	 * but when "on_chunk_complete" is called, chunk buffer addr should be NULL.
	 * This is to avoid confusion from the application thinking
	 * that there are actually available in a chunk on last call.
	 */
	req->chunk._offset = 0U;
	req->chunk.len = 0U;
	req->chunk.loc = NULL;

	LOG_DBG("(%p) on_chunk_complete chunk=%u len=%u",
		req, req->chunk.id, req->chunk.len);

	return 0;
}


int http_request_route_arg_get(http_request_t *req,
			       uint32_t index,
			       uint32_t *arg)
{
	__ASSERT_NO_MSG(req != NULL);
	__ASSERT_NO_MSG(req->route != NULL);
	__ASSERT_NO_MSG(arg != NULL);

	int ret = -EINVAL;

	if (index < req->route->path_args_count) {
		*arg = req->route_args[index];
		ret = 0;
	}

	return ret;
}

bool http_request_parse(http_request_t *req,
			const char *data,
			size_t received)
{
	size_t parsed = http_parser_execute(&req->parser,
					    req->parser_settings,
					    data, received);
	LOG_DBG("parsed = %u, received = %u", parsed, received);
	if (parsed == received) {
		if (HTTP_PARSER_ERRNO(&req->parser) == HPE_PAUSED) {
			http_parser_pause(&req->parser, 0);
		} else {
			__ASSERT(HTTP_PARSER_ERRNO(&req->parser) == HPE_OK,
				 "HTTP parser error");
		}
	} else if (HTTP_PARSER_ERRNO(&req->parser) == HPE_PAUSED) {
		LOG_ERR("(%p) More (unexpected) data to parse, parsed=%u / to parse=%d",
			req, parsed, received);
		return false;
	} else {
		LOG_ERR("(%p) Parser error = %d",
			req, HTTP_PARSER_ERRNO(&req->parser));
		return false;
	}

	return true;
}

void http_request_discard(http_request_t *req,
			  http_request_discard_reason_t reason)
{
	mark_discarded(req, reason);
	
	LOG_DBG("(%p) Discarding request, reason: %s (%u)",
		req, discard_reason_to_str(reason), reason);
}

const char *http_header_get_value(http_request_t *req,
				  const char *hdr_name)
{
	const char *value = NULL;
	struct http_header *hdr = NULL;
	SYS_DLIST_FOR_EACH_CONTAINER(&req->headers, hdr, handle)
	{
		if (strcmp(hdr->name, hdr_name) == 0) {
			value = (const char *)hdr->value;
			break;
		}
	}

	return value;
}

bool http_discard_reason_to_status_code(http_request_discard_reason_t reason,
					uint16_t *status_code)
{
	if (status_code == NULL) {
		return false;
	}

	switch (reason) {
	case HTTP_REQUEST_ROUTE_UNKNOWN:
		*status_code = HTTP_STATUS_NOT_FOUND;
		break;
	case HTTP_REQUEST_BAD:
		*status_code = HTTP_STATUS_BAD_REQUEST;
		break;
	case HTTP_REQUEST_ROUTE_NO_HANDLER:
		*status_code = HTTP_STATUS_NOT_IMPLEMENTED;
		break;
	case HTTP_REQUEST_STREAMING_UNSUPPORTED:
		*status_code = HTTP_STATUS_NOT_IMPLEMENTED;
		break;
	case HTTP_REQUEST_PAYLOAD_TOO_LARGE:
		*status_code = HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE;
		break;
	case HTTP_REQUEST_PROCESSING_ERROR:
	default:
		*status_code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
		break;
	}

	return true;
}

const char *http_route_extract_subpath(http_request_t *req)
{
	const char *subpath = NULL;

	if (route_is_valid(req->route) &&
	    (req->route->match_type == HTTP_ROUTE_MATCH_LEASE_NOARGS)) {
		subpath = req->url + req->route->route_len;
	}

	return subpath;
}