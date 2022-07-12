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

struct http_request
{
	/**
	 * @brief Connection currently owning the current request
	 */
	http_connection_t *_conn;
	
	/**
	 * @brief One parser per request, in order to process them asynchronously
	 */
        struct http_parser parser;

	/**
	 * @brief Flag telling whether keep-alive is set in the request
	 */
	uint8_t keep_alive : 1;

	/**
	 * @brief Flag telling whether HTTP headers are complete
	 */
	uint8_t headers_complete: 1;

        /**
         * @brief Flag telling whether HTTP request is complete
         */
        uint8_t request_complete : 1;
	
	/**
	 * @brief Is the request sended using "chunk" encoding, if yes we should
	 * handle the request as a stream.
	 */
	uint8_t chunked: 1;

	/**
	 * @brief Process the request as a stream 
	 * Note: If chunked is set or if content-length is larger 
	 *       than HTTP_REQUEST_PAYLOAD_MAX_SIZE
	 */
	uint8_t stream: 1;

	/**
	 * @brief Tells if the rest of the request should be discarded
	 * 
	 * Reasons could be:
	 * - The request is too large
	 * - Error in parsing the request
	 * 
	 * Note: Determined when headers are parsed.
	 */
	uint32_t discard: 1;

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

	http_route_args_t *route_args;

	/* parsed authentification */
	/*
        struct {
                char user[16];
                char password[16];
        };
	*/

	/* payload / chunk */
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


	/* Total received length */
        // size_t len;
};

typedef struct http_request http_request_t;

#endif