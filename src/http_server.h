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

struct connection
{
        
};

int http_server_init(void);

int http_server_setup_sockets(void);

void http_server_thread(void *_a, void *_b, void *_c);

int http_server_accept_connection(struct pollfd *pfd, struct connection *conn);

int http_server_handle_connection(struct pollfd *pfd, struct connection *conn);

#endif