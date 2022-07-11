#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <stdio.h>
#include <stdint.h>

#include <net/http_parser.h>

#include "utils.h"
#include "http_utils.h"
#include "routes.h"

struct http_request
{
	/* TODO socket id will be required if requests are intended to be processed 
	 * in parallel. workqueue, ...*/
	int _sock;
	
	/* One parser per request, in order to process them asynchronously */
        struct http_parser parser;

	/* enabled if keep-alive is set in the request */
	uint8_t keep_alive : 1;

	/* flag telling whether HTTP headers are complete */
	uint8_t headers_complete: 1;

        /* flag telling whether HTTP request is complete */
        uint8_t complete : 1;
	
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
	 * @brief Tells if the body should be discarded because cannot be processed
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

	/* Buffer used to store the received bytes */
        struct {
                char* buf;
                size_t size;
        } buffer;

	http_chunk_t chunk;

	/* Received length */
        size_t len;

	/* HTTP content location */
        struct {
                char *loc;
                size_t len;
        } payload;
};

struct http_response
{
	union {
		/* This is as VERY DANGEROUS trick */
		buffer_t buffer;
		struct {
			char *buf;
			size_t buf_size;
			size_t content_len;
		};
	};
	
        uint16_t status_code;
	http_content_type_t content_type;
};

typedef struct http_request http_request_t;
typedef struct http_response http_response_t;

#endif