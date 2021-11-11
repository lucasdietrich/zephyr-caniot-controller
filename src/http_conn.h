#ifndef _HTTP_CONN_H_
#define _HTTP_CONN_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <net/http_parser.h>

#define MAX_CONNECTIONS   3

struct connection
{
        /* INTERNAL */

        int sock;

        /* REQUEST RELATED */

        /* struct sockaddr_in addr; */
        struct http_parser parser;

        uint8_t complete: 1;
        uint8_t keep_alive: 1;

        /* RESPONSE RELATED */
        uint16_t status_code;

        uint16_t response_len;
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