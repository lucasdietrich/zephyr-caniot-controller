#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <stdio.h>
#include <stdint.h>

#include <net/http_parser.h>

#include "utils.h"
#include "http_utils.h"
#include "routes.h"

struct http_connection;
typedef struct http_connection http_connection_t;

typedef enum {
	/**
	 * @brief Route not in the list or failed to match
	 * Note: status code 404
	 */
	HTTP_REQUEST_ROUTE_UNKNOWN,

	/**
	 * @brief Route matched, but no handler was found
	 * Note: status code 501
	 */
	HTTP_REQUEST_ROUTE_NO_HANDLER,

	/**
	 * @brief Streaming not supported for the route or the global request
	 * Note: status code 501
	 */
	HTTP_REQUEST_STREAMING_UNSUPPORTED,

	/**
	 * @brief Route OK, but payload is too large to be handled as
	 *  as a single message (consider using chunk encoding = streaming)
	 * Note: status code 413
	 */
	HTTP_REQUEST_PAYLOAD_TOO_LARGE,

	/**
	 * @brief Route OK, but processing of the stream request processing failed
	 * Note: status code 500
	 */
	HTTP_REQUEST_STREAM_PROCESSING_ERROR,

} http_request_discard_reason_t;

struct http_request
{
	/**
	 * @brief Flag telling whether keep-alive is set in the request
	 * Note: determined in headers
	 */
	uint8_t keep_alive : 1;

	/**
	 * @brief Flag telling whether HTTP headers are complete
	 * Note: determined in headers
	 */
	uint8_t headers_complete : 1;

        /**
         * @brief Flag telling whether HTTP request is complete
	 * 
	 * Note: 
	 * - No more parsing is done after this flag is set
	 * - This flag can be used to determine if a stream is complete
	 */
        uint8_t complete : 1;

	enum {
		/**
		 * @brief Normal handling method, processed as a single message
		 * 
		 * Route handler is called once in this case
		 */
		HTTP_REQUEST_AS_MESSAGE,

		/**
		 * @brief Is the request sended using "chunk" encoding, if yes we should
		 * handle the request as a stream.
		 * 
		 * Note: determined in headers
		 */
		HTTP_REQUEST_AS_STREAM,
		
		/**
		 * @brief Tells if the rest of the request should be discarded
		 * 
		 * Reasons could be:
		 * - The request is too large
		 * - Error in parsing the request
		 * 
		 * Note: Determined when headers are parsed.
		 */
		HTTP_REQUEST_DISCARD,
	} handling_method;

	http_request_discard_reason_t discard_reason;

	/**
	 * @brief Parsed content length, TODO should be compared against "len" 
	 * when the request was totally received
	 */
	uint16_t parsed_content_length;

	/**
	 * @brief Request method (GET, POST, PUT, DELETE)
	 */
        enum http_method method;

	/* Header currently being parsed */
	const struct http_request_header *_parsing_cur_header;

	/* TODO headers values (dynamically allocated and freed, using HEAP/MEMSLAB ) */
	sys_dlist_t _headers;
	
	/* Request content type */
	http_content_type_t content_type;

	/**
	 * @brief Timeout of the request in ms (to be used by the app)
	 * Note: Timeout of 0 means no timeout value defined (FOREVER or NONE)
	 */
	uint32_t timeout_ms;

	/* parsed url */
        char url[64U];
        size_t url_len;

	/* route for the current request */
	const http_route_t *route;

	http_route_args_t route_args;

	/**
	 * @brief Number of times the request route handler has been called
	 * 
	 * Note: Can help to used determine if the handler is called
	 *  to process a new request (if 0).
	 */
	size_t calls_count;
	
	/* parsed authentification */
	/*
        struct {
                char user[16];
                char password[16];
        };
	*/

	union {
		/* Chunk if "stream" is set */
		struct {
			/**
			 * @brief Current chunk of data being parsed
			 */
			uint16_t id;

			/**
			 * @brief Current chunk data buffer location
			 */
			char *loc;

			/**
			 * @brief Number of bytes in the data buffer
			 */
			uint16_t len;

			/**
			 * @brief Offset of the data being parsed within the buffer
			 */
			uint16_t _offset;
		} chunk;

		/* Complete message payload if "stream" is not set */
		struct {
			/**
			 * @brief Current payload data buffer location
			 */
			char *loc;

			/**
			 * @brief Number of bytes in the data buffer
			 */
			uint32_t len;
		} payload;
	};

	/**
	 * @brief Parser settings, which is reseted after each request
	 * (e.g. Message, stream, discard)
	 */
	const struct http_parser_settings *parser_settings;

	/* Total HTTP payload length */
        size_t len;
};

typedef struct http_request http_request_t;

void http_request_init(http_request_t *req);

static inline bool http_request_is_discarded(const http_request_t *req)
{
	return req->handling_method == HTTP_REQUEST_DISCARD;
}

static inline bool http_request_is_stream(http_request_t *req)
{
	return req->handling_method == HTTP_REQUEST_AS_STREAM;
}

static inline bool http_request_is_message(http_request_t *req)
{
	return req->handling_method == HTTP_REQUEST_AS_MESSAGE;
}

int http_request_route_arg_get(http_request_t *req,
		       uint32_t index,
		       uint32_t *arg);

#endif