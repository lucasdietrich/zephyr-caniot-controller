#ifndef _HTTP_CONN_H_
#define _HTTP_CONN_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <net/net_ip.h>
#include <net/http_parser.h>

#include "http_request.h"

#define MAX_CONNECTIONS   3

struct connection
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
};

inline uint16_t get_conn_method(struct connection *conn)
{
        return conn->parser.method;
}

struct connection *get_connection(int index);
int conn_get_index(struct connection *conn);
void clear_conn(struct connection *conn);
struct connection *alloc_connection(void);
void free_connection(struct connection *conn);
bool conn_is_closed(struct connection *conn);

#endif