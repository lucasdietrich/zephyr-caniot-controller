/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_server.h"
#include "http_session.h"

/* sessions sessions */

static http_session_t sessions[CONFIG_APP_HTTP_MAX_SESSIONS];

static sys_dlist_t sessions_list	  = SYS_DLIST_STATIC_INIT(&sessions_list);
static sys_slist_t sessions_free_list = SYS_SLIST_STATIC_INIT(&sessions_free_list);

void http_session_init(void)
{
	/* Initialize the free list of blocks
	 * I think there is no zephyr structure that allows to do this.
	 */
	for (uint32_t i = 0; i < CONFIG_APP_HTTP_MAX_SESSIONS; i++) {
		sys_slist_append(&sessions_free_list, &sessions[i]._alloc_handle);
	}
}

static void clear_sess(http_session_t *sess)
{
	/* TODO, optimize the init */
	memset(sess, 0, sizeof(*sess));

	sess->sock = -1;
}

http_session_t *http_session_alloc(void)
{
	sys_snode_t *node;

	if ((node = sys_slist_get(&sessions_free_list)) != NULL) {
		http_session_t *sess = CONTAINER_OF(node, http_session_t, _alloc_handle);
		clear_sess(sess);
		sys_dlist_append(&sessions_list, &sess->_handle);
		return sess;
	}

	return NULL;
}

void http_session_free(http_session_t *sess)
{
	if (sess != NULL) {

		sys_dlist_remove(&sess->_handle);

		sess->sock = -1;

		sys_slist_append(&sessions_free_list, &sess->_alloc_handle);
	}
}

http_session_t *http_session_get_by_sock(int sock_fd)
{
	sys_dlist_t *node = NULL;

	SYS_DLIST_ITERATE_FROM_NODE(&sessions_list, node)
	{
		http_session_t *const sess = CONTAINER_OF(node, http_session_t, _handle);
		if (sess->sock == sock_fd) {
			return sess;
		}
	}

	return NULL;
}

bool http_session_is_closed(http_session_t *sess)
{
	bool closed = true;

	if (sess != NULL) {
		closed = sess->sock == -1;
	}

	return closed;
}

bool http_session_is_outdated(http_session_t *sess)
{
	const uint32_t now	= k_uptime_get_32();
	const uint32_t diff = now - sess->keep_alive.last_activity;

	return diff > sess->keep_alive.timeout;
}

int http_session_time_to_next_outdated(void)
{
	const uint32_t now = k_uptime_get_32();
	int timeout		   = SYS_FOREVER_MS;
	sys_dlist_t *node  = NULL;

	SYS_DLIST_ITERATE_FROM_NODE(&sessions_list, node)
	{
		http_session_t *const sess = CONTAINER_OF(node, http_session_t, _handle);
		const uint32_t diff		   = now - sess->keep_alive.last_activity;
		const bool outdated		   = diff > sess->keep_alive.timeout;
		if (outdated) {
			timeout = 0;
			break;
		} else {
			const uint32_t time_to_expire = sess->keep_alive.timeout - diff;
			if (timeout == SYS_FOREVER_MS) {
				timeout = time_to_expire;
			} else {
				timeout = MIN(timeout, time_to_expire);
			}
		}
	}

	return timeout;
}