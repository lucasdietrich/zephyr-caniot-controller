#ifndef _HTTP_CONN_H_
#define _HTTP_CONN_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <net/net_ip.h>
#include <net/http_parser.h>

#include "http_request.h"

typedef struct
{
        /* INTERNAL */
        struct sockaddr addr;

        /* struct sockaddr_in addr; */
        struct http_parser parser;

        enum {
                HEADER_NONE = 0,
                HEADER_CONNECTION,
                HEADER_AUTH,

		/* TODO : https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Keep-Alive */
		HEADER_KEEP_ALIVE,
        } parsing_header;

        
        uint8_t complete : 1;

	struct {
		/* enabled if keep-alive is set in the request */
		uint8_t enabled : 1;

		/* in ms */
		uint32_t timeout;

		/* last process uptime in ms */
		uint32_t last_activity;
	} keep_alive;

        struct http_request *req;
        struct http_response *resp;
} http_connection_t;

void http_conn_init_pool(void);

http_connection_t *http_conn_get(int index);

int http_conn_get_index(http_connection_t *conn);

http_connection_t *http_conn_alloc(void);

void http_conn_free(http_connection_t *conn);

bool http_conn_closed(http_connection_t *conn);

// iterate over the connections
void http_conn_iterate(void (*callback)(http_connection_t *conn,
					void *user_data),
		       void *user_data);

bool http_conn_is_outdated(http_connection_t *conn);

// Function to get the next time we need to process an outdated connection
int http_conn_get_duration_to_next_outdated_conn(void);

#endif