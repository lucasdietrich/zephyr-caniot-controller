/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_CONN_H_
#define _HTTP_CONN_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/http_parser.h>

#include <zephyr/sys/dlist.h>

#include <zephyr/sys/dlist.h>
#include <zephyr/sys/slist.h>

#include "http_request.h"
#include "http_response.h"

struct http_connection;

struct http_connection
{
        /* INTERNAL */
        struct sockaddr addr;
	
	union {
		/* Handle when allocated */
		sys_dnode_t _handle;

		/* Handle in the free list for allocation */
		sys_snode_t _alloc_handle;
	};

	/**
	 * @brief Socket id (-1 if undefined/closed)
	 */
	int sock;

        /* struct sockaddr_in addr; */

	struct {
		/* keep alive enabled */
		uint8_t enabled: 1;

		/* in ms */
		uint32_t timeout;

		/* last process uptime in ms */
		uint32_t last_activity;
	} keep_alive;

        http_request_t *req;
        http_response_t *resp;

	/* Is the connection secure ? */
	bool secure;
	/* TODO add secure context: client certificate common name, ... */

	/* STATS */
	size_t requests_count;
	size_t rx_bytes;
	size_t tx_bytes;
};

typedef struct http_connection http_connection_t;

void http_conn_init(void);

http_connection_t *http_conn_alloc(void);

http_connection_t *http_conn_get_by_sock(int sock_fd);

void http_conn_free(http_connection_t *conn);

bool http_conn_is_closed(http_connection_t *conn);


bool http_conn_is_outdated(http_connection_t *conn);

// Function to get the next time we need to process an outdated connection
int http_conn_time_to_next_outdated(void);

#endif