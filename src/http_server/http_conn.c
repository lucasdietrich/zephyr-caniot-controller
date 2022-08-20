/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_conn.h"

#include "http_server.h"


/* connections connections */

// uint32_t conns_count = 0;
static http_connection_t connections[CONFIG_HTTP_MAX_CONNECTIONS];

static sys_dlist_t conns_list = SYS_DLIST_STATIC_INIT(&conns_list);
static sys_slist_t conns_free_list = SYS_SLIST_STATIC_INIT(&conns_free_list);

void http_conn_init(void)
{
	/* Initialize the free list of blocks 
	 * I think there is no zephyr structure that allows to do this.
	 */
	for (uint32_t i = 0; i < CONFIG_HTTP_MAX_CONNECTIONS; i++) {
		sys_slist_append(&conns_free_list, &connections[i]._alloc_handle);
	}
}

static void clear_conn(http_connection_t *conn)
{
	/* TODO, optimize the init */
	memset(conn, 0, sizeof(*conn));

	conn->sock = -1;
}

http_connection_t *http_conn_alloc(void)
{
	sys_snode_t *node;

	if ((node = sys_slist_get(&conns_free_list)) != NULL) {
		http_connection_t *conn = CONTAINER_OF(node, http_connection_t, _alloc_handle);
		clear_conn(conn);
		sys_dlist_append(&conns_list, &conn->_handle);
		return conn;
	}

	return NULL;
}

void http_conn_free(http_connection_t *conn)
{
	if (conn != NULL) {
		
		sys_dlist_remove(&conn->_handle);

		conn->sock = -1;

		sys_slist_append(&conns_free_list, &conn->_alloc_handle);
	}
}

http_connection_t *http_conn_get_by_sock(int sock_fd)
{
	sys_dlist_t *node = NULL;

	SYS_DLIST_ITERATE_FROM_NODE(&conns_list, node) {
		http_connection_t *const conn = CONTAINER_OF(node, http_connection_t, _handle);
		if (conn->sock == sock_fd) {
			return conn;
		}
	}

	return NULL;
}

bool http_conn_is_closed(http_connection_t *conn)
{
	bool closed = true;

	if (conn != NULL) {
		closed = conn->sock == -1;
	}
	
	return closed;
}

bool http_conn_is_outdated(http_connection_t *conn)
{
	uint32_t now = k_uptime_get_32();

	return (now - conn->keep_alive.last_activity) > conn->keep_alive.timeout;
}

int http_conn_time_to_next_outdated(void)
{
	const uint32_t now = k_uptime_get_32();
	int timeout = SYS_FOREVER_MS;
	sys_dlist_t *node = NULL;

	SYS_DLIST_ITERATE_FROM_NODE(&conns_list, node) {
		http_connection_t *const conn = CONTAINER_OF(node, http_connection_t, _handle);
		const uint32_t diff = now - conn->keep_alive.last_activity;
		const bool outdated = diff > conn->keep_alive.timeout;
		if (outdated) {
			timeout = 0;
			break;
		} else {
			const uint32_t time_to_expire = conn->keep_alive.timeout - diff;
			if (timeout == SYS_FOREVER_MS) {
				timeout = time_to_expire;
			} else {
				timeout = MIN(timeout, time_to_expire);
			}
		}
	}

	return timeout;
}