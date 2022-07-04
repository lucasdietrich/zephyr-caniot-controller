#ifndef _HTTP_CONN_H_
#define _HTTP_CONN_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <net/net_ip.h>
#include <net/http_parser.h>

#include <sys/dlist.h>

#include "http_request.h"

struct http_connection;

struct http_request_header
{
	/* header anem "Timeout", ... */
	const char *name;

	uint16_t len;

	int (*handler)(const struct http_request_header *hdr,
		       struct http_connection *conn,
		       const char *value);
};

struct http_connection
{
        /* INTERNAL */
        struct sockaddr addr;

        /* struct sockaddr_in addr; */
	/* TODO parser could be shared among all connections as requests 
	 * are parsed as a whole. */
	/* TODO parser should belongs to the http_request structure 
	 * and not the connection structure, this solves also above problem */
        struct http_parser parser;

	/* Haader currently being parsed */
	const struct http_request_header *_parsing_cur_header;

	/* headers values (dynamically allocated and freed, using HEAP/MEMSLAB ) */
	sys_dlist_t _headers;

        /* tells if HTTP request is complete */
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
};

typedef struct http_connection http_connection_t;

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