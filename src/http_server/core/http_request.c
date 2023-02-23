/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_request.h"
#include "http_response.h"
#include "http_server.h"
#include "http_utils.h"
#include "utils/buffers.h"
#include "utils/misc.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>

#include <malloc.h>
LOG_MODULE_REGISTER(http_req, LOG_LEVEL_WRN);

/* parsing */

/*_____________________________________________________________________________________*/

/* forward declarations of static functions */
static int on_header_field(struct http_parser *parser, const char *at, size_t length);
static int on_header_value(struct http_parser *parser, const char *at, size_t length);
static int on_headers_complete(struct http_parser *parser);
static int on_message_begin(struct http_parser *parser);
static int on_body(struct http_parser *parser, const char *at, size_t length);
static int on_message_complete(struct http_parser *parser);
static int on_url(struct http_parser *parser, const char *at, size_t length);
static int on_chunk_header(struct http_parser *parser);
static int on_chunk_complete(struct http_parser *parser);

extern int http_call_req_handler(http_request_t *req);

/*_____________________________________________________________________________________*/
const struct http_parser_settings parser_settings = {
	.on_status	     = NULL, /* no status for requests */
	.on_url		     = on_url,
	.on_header_field     = on_header_field,
	.on_header_value     = on_header_value,
	.on_headers_complete = on_headers_complete,
	.on_message_begin    = on_message_begin,
	.on_message_complete = on_message_complete,
	.on_body	     = on_body,

	.on_chunk_header   = on_chunk_header,
	.on_chunk_complete = on_chunk_complete};

/*_____________________________________________________________________________________*/

void http_request_init(http_request_t *req)
{
	/* Rest of the request is initialize to 0 */
	*req = (http_request_t){
		.content_type		 = HTTP_CONTENT_TYPE_TEXT_PLAIN,
		.route_parse_results_len = CONFIG_APP_ROUTE_MAX_DEPTH,

		.url_len   = 0u,
		.flush_len = 0u,
	};

	sys_dlist_init(&req->headers);

	http_parser_init(&req->parser, HTTP_REQUEST);

#if defined(CONFIG_APP_HTTP_TEST)
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
	case HTTP_REQUEST_UNSECURE_ACCESS:
		return "Unsecure access";
	default:
		return "<unknown discard reason>";
	}
}

static void mark_discarded(http_request_t *req, http_request_discard_reason_t reason)
{
	req->discarded	    = 1u;
	req->discard_reason = reason;
}

/*_____________________________________________________________________________________*/

#define REQUEST_FROM_PARSER(p_parser)                                                    \
	((http_request_t *)CONTAINER_OF(p_parser, http_request_t, parser))

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

	if (req->url_len >= sizeof(req->url)) {
		LOG_ERR("(%p) URL too long (%d >= %u)", req, length, sizeof(req->url));

		return -EINVAL;
	}

	LOG_HEXDUMP_DBG(at, length, "url");

	memcpy(&req->url[req->url_len], at, length);
	req->url_len += length;
	req->url[req->url_len] = '\0';

	return 0;
}

struct http_header_handler {
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

#define HEADER(_name, _handler)                                                          \
	{                                                                                \
		.name = _name, .len = sizeof(_name) - 1, .handler = _handler,            \
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

	if ((strncicmp(value, CHUNKED_TRANSFERT_STR, strlen(CHUNKED_TRANSFERT_STR)) ==
	     0)) {
		LOG_INF("(%p) Header Transfer Encoding (= chunked) found !", req);
		req->chunked_encoding = 1;
	}

	return 0;
}

static int header_content_type_handler(http_request_t *req,
				       const struct header *hdr,
				       const char *value,
				       size_t length)
{
#define CONTENT_TYPE_APPLICATION_OCTET_STREAM_STR "application/octet-stream"
#define CONTENT_TYPE_MULTIPART_FORM_DATA_STR	  "multipart/form-data"

	if (strncicmp(value,
		      CONTENT_TYPE_APPLICATION_OCTET_STREAM_STR,
		      strlen(CONTENT_TYPE_APPLICATION_OCTET_STREAM_STR) == 0)) {
		LOG_INF("(%p) "
			"Content-"
			"Type " CONTENT_TYPE_APPLICATION_OCTET_STREAM_STR,
			req);
		req->content_type = HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM;
	} else if (strncicmp(value,
			     CONTENT_TYPE_MULTIPART_FORM_DATA_STR,
			     strlen(CONTENT_TYPE_MULTIPART_FORM_DATA_STR) == 0)) {
		LOG_INF("(%p) "
			"Content-Type " CONTENT_TYPE_MULTIPART_FORM_DATA_STR,
			req);
		req->content_type = HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA;
	}

	return 0;
}

static char hdrbuf[CONFIG_APP_HTTP_REQUEST_HEADERS_BUFFER_SIZE];
static size_t hdr_allocated = 0U;

static struct http_header *alloc_header_buffer(size_t value_size)
{
	struct http_header *hdr = NULL;
	const size_t allocsize	= sizeof(struct http_header) + value_size;

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
		LOG_WRN("(%p) Cannot allocate header buffer for %s", req, hdr->name);
	}

	return 0;
}

/* All headers that are not handled by a handler are discarded. */
static const struct header headers[] = {
	HEADER("Connection", header_keepalive_handler),
	HEADER("Timeout-ms", header_timeout_handler),
	HEADER("Transfer-Encoding", header_transfer_encoding_handler),
	HEADER("Content-Type", header_content_type_handler),

	HEADER("Authorization", header_keep),
	HEADER("App-Upload-Checksum", header_keep),
	HEADER("App-Script-Filename", header_keep),
	HEADER("App-sha1", header_keep),

#if defined(CONFIG_APP_HTTP_TEST_SERVER)
	HEADER("App-Test-Header1", header_keep),
	HEADER("App-Test-Header2", header_keep),
	HEADER("App-Test-Header3", header_keep),
#endif /* CONFIG_APP_HTTP_TEST_SERVER */
};

/* TODO check that the implementation leads only to a single call to the
 * handler when a header value is parsed. */
static int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
	/* TODO handle case where header field is split */

	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	req->_parsing_cur_header = NULL;

	/* iterate over "headers" array and check if "at" matches the header
	 * name */
	for (size_t i = 0; i < ARRAY_SIZE(headers); i++) {
		const struct header *h = &headers[i];

		if ((length == h->len) && !strncicmp(at, h->name, length)) {
			req->_parsing_cur_header = h;
			break;
		}
	}

	return 0;
}

static int on_header_value(struct http_parser *parser, const char *at, size_t length)
{
	/* TODO handle case where header value is split */

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

	const enum http_method method	  = parser->method;
	const bool content_length_present = parser->flags & F_CONTENTLENGTH;
	const int content_length = content_length_present ? parser->content_length : -1;

	LOG_INF("(%p) Headers complete %s %s content len=%d [hdr buf %u/%u]",
		req,
		http_method_str(method),
		req->url,
		content_length,
		hdr_allocated,
		CONFIG_APP_HTTP_REQUEST_HEADERS_BUFFER_SIZE);

	/* For debug */
	if (req->_url_copy != NULL) {
		strncpy(req->_url_copy, req->url, HTTP_URL_MAX_LEN);
	}

	/* Resolve route as we have enough information */
	const struct route_descr *route = route_resolve(method,
							req->url,
							req->route_parse_results,
							&req->route_parse_results_len,
							&req->query_string);

	req->method	      = method;
	req->route	      = route;
	req->headers_complete = 1U;
	req->streaming	      = route_supports_streaming(route);
	req->route_depth =
		req->route_parse_results[req->route_parse_results_len - 1u].depth;

	/* TODO Check whether secure params are compliant with the requested
	 * route */

	/* Checks */
	if (route == NULL) {
		mark_discarded(req, HTTP_REQUEST_ROUTE_UNKNOWN);
		LOG_WRN("(%p) Route not found %s %s",
			req,
			http_method_str(req->method),
			req->_url_copy);
	} else if (route->resp_handler == NULL) {
		mark_discarded(req, HTTP_REQUEST_ROUTE_NO_HANDLER);
		LOG_ERR("(%p) Route has no handler %s %s",
			req,
			http_method_str(req->method),
			req->_url_copy);
	}

	/* Check for secure */
#if defined(CONFIG_APP_HTTP_TEST)
	/* Here we decide if we want to test the
	 * request as a stream or a message
	 */
	req->_test_ctx.stream = http_request_is_stream(req);
#endif /* CONFIG_APP_HTTP_TEST */

	return 0;
}

static int on_body(struct http_parser *parser, const char *at, size_t length)
{
	int ret = 0;

	/* can be called several times */
	http_request_t *req = REQUEST_FROM_PARSER(parser);

	// LOG_HEXDUMP_DBG(at, length, "(%p) Body");

	req->payload_len += length;

	if (req->discarded) goto exit;

	if (req->streaming) {
		req->payload.loc = (char *)at;
		req->payload.len = length;

		if (req->chunked_encoding) {
			/* Increase chunk offset using previous part size */
			req->chunk._offset += req->chunk.len;

			/* set chunk location */
			req->chunk.loc = (char *)at;
			req->chunk.len = length;
		}

		if (req->payload.len >= req->flush_len) {
			/* We pause the parser, so that we
			 * can call the streaming handler */
			int ret = http_call_req_handler(req);
			if (ret < 0) {
				http_request_discard(req, HTTP_REQUEST_PROCESSING_ERROR);
				LOG_ERR("(%p) Streaming handler error %d", req, ret);
				ret = 0;
				goto exit;
			}
		}
	} else {
		if (req->payload.loc == NULL) {
			req->payload.loc = (char *)at;
			req->payload.len = length;
		} else {
			req->payload.len += length;
		}
	}

	LOG_DBG("(%p) on_body at=%p len=%u", req, (void *)at, length);

exit:
	return ret;
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
	 * same "http_parser_execute" call, before a reponse is sent from the
	 * current server for the 1st request. Then it means that the client
	 * sent (TCP) two HTTP requests while not waiting for the response for
	 * the 1st request. If this case happens the current server will closes
	 * the connection.
	 */
	http_parser_pause(parser, 1);

	LOG_DBG("(%p) on_message_complete", req);

	return 0;
}

static int on_chunk_header(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	__ASSERT_NO_MSG(http_request_is_stream(req) == true);

	if (!req->chunked_encoding) {
		mark_discarded(req, HTTP_REQUEST_BAD);
	} else {
		LOG_DBG("(%p) on_chunk_header chunk=%u", req, req->chunk.id);
	}

	return 0;
}

static int on_chunk_complete(struct http_parser *parser)
{
	http_request_t *const req = REQUEST_FROM_PARSER(parser);

	/* increment chunk id */
	req->chunk.id++;

	/* Reset chunk buffer infos, this could be done in on_chunk_header(),
	 * but when "on_chunk_complete" is called, chunk buffer addr should be
	 * NULL. This is to avoid confusion from the application thinking that
	 * there are actually available in a chunk on last call.
	 */
	req->chunk._offset = 0U;
	req->chunk.len	   = 0U;
	req->chunk.loc	   = NULL;

	LOG_DBG("(%p) on_chunk_complete chunk=%u len=%u",
		req,
		req->chunk.id,
		req->chunk.len);

	return 0;
}

int http_req_route_arg_get_number_by_index(http_request_t *req,
					   int32_t rel_index,
					   uint32_t *value)
{
	__ASSERT_NO_MSG(req != NULL);
	__ASSERT_NO_MSG(req->route != NULL);
	__ASSERT_NO_MSG(value != NULL);

	if (rel_index < 0) {
		rel_index = req->route_depth + rel_index;
	}

	return route_results_get_number_by_index(req->route_parse_results,
						 req->route_parse_results_len,
						 (uint32_t)rel_index,
						 value);
}

bool http_request_parse_buf(http_request_t *req, char *buf, size_t len)
{
	__ASSERT_NO_MSG(req != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	bool success = true;
	const size_t parsed =
		http_parser_execute(&req->parser, &parser_settings, buf, len);

	if (parsed != len) {
		const bool paused = HTTP_PARSER_ERRNO(&req->parser) == HPE_PAUSED;

		if (paused && req->complete) {
			LOG_ERR("(%p) Request malformed, more "
				"data to parse rem=%u",
				req,
				len);
		} else {
			LOG_ERR("Unexpected HTTP parser pause parsed/len = "
				"%u/%u",
				parsed,
				len);
		}

		success = false;
	}

	return success;
}

void http_request_discard(http_request_t *req, http_request_discard_reason_t reason)
{
	mark_discarded(req, reason);

	LOG_DBG("(%p) Discarding request, reason: %s (%u)",
		req,
		discard_reason_to_str(reason),
		reason);
}

const char *http_header_get_value(http_request_t *req, const char *hdr_name)
{
	const char *value	= NULL;
	struct http_header *hdr = NULL;
	SYS_DLIST_FOR_EACH_CONTAINER (&req->headers, hdr, handle) {
		if (strcmp(hdr->name, hdr_name) == 0) {
			value = (const char *)hdr->value;
			break;
		}
	}

	return value;
}

int http_req_route_arg_get(http_request_t *req, const char *name, uint32_t *value)
{
	__ASSERT_NO_MSG(req != NULL);
	__ASSERT_NO_MSG(req->route != NULL);
	__ASSERT_NO_MSG(value != NULL);

	return route_results_get(req->route_parse_results,
				 req->route_parse_results_len,
				 name,
				 ROUTE_ARG_UINT | ROUTE_ARG_HEX,
				 (void **)value);
}

int http_req_route_arg_get_string(http_request_t *req, const char *name, char **value)
{
	__ASSERT_NO_MSG(req != NULL);
	__ASSERT_NO_MSG(req->route != NULL);
	__ASSERT_NO_MSG(value != NULL);

	return route_results_get(req->route_parse_results,
				 req->route_parse_results_len,
				 name,
				 ROUTE_ARG_STR,
				 (void **)value);
}