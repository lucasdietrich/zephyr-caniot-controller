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
	/**
	 * @brief Request method (GET, POST, PUT, DELETE)
	 */
        enum http_method method;

	/* Timeout of the request in ms (to be used by the app)
	 * - Timeout of 0 means no timeout
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

	/* Request length */
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

#endif