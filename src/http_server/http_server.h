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

#include "http_request.h"
#include "http_conn.h"

int setup_sockets(void);

void http_srv_thread(void *_a, void *_b, void *_c);

#endif