/**
 * @file http_server.h
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-11-07
 * 
 * @copyright Copyright (c) 2021
 * 
 * Requirements
 * 
 * - webserver
 * - REST/JSON
 * - keep-alive
 * - client and server certificate
 */

#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include <poll.h>

#include <net/http_parser.h>
#include <net/http_client.h>

struct connection
{
        /* REQUEST RELATED */

        /* struct sockaddr_in addr; */
        struct http_parser parser;

        uint8_t complete: 1;
        uint8_t keep_alive: 1;

        /* RESPONSE RELATED */
        uint16_t status_code;

        uint16_t response_len;
};

int http_srv_setup_sockets(void);

void http_srv_thread(void *_a, void *_b, void *_c);

int http_srv_accept(int serv_sock);

int http_srv_handle_conn(int sock, struct connection *conn);

int http_srv_process_request(int sock, struct connection *conn);

#endif