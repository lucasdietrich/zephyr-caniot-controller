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
        } parsing_header;

        
        uint8_t complete : 1;
        uint8_t keep_alive : 1;

        struct http_request *req;
        struct http_response *resp;
} http_connection_t;

void http_conn_init_pool(void);

http_connection_t *http_connect_get(int index);
int http_conn_get_index(http_connection_t *conn);
http_connection_t *http_conn_alloc(void);
void http_conn_free(http_connection_t *conn);
bool http_conn_closed(http_connection_t *conn);

#endif