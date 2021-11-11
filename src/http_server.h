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

struct connection;

int http_srv_setup_sockets(void);

void http_srv_thread(void *_a, void *_b, void *_c);

int http_srv_accept(int serv_sock);

int http_srv_handle_conn(struct connection *conn);

int http_srv_process_request(struct connection *conn);

#endif