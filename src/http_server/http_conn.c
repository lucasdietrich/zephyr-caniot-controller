#include "http_conn.h"

#include "http_server.h"

/*___________________________________________________________________________*/

/* connections connections */

uint32_t conns_count = 0;
static http_connection_t connections[CONFIG_MAX_HTTP_CONNECTIONS];

/* all active connections stacked a the top of the array */
static http_connection_t *pool[CONFIG_MAX_HTTP_CONNECTIONS];

void http_conn_init_pool(void)
{
        for (uint32_t i = 0; i < CONFIG_MAX_HTTP_CONNECTIONS; i++) {
                pool[i] = &connections[i];
        }
}

http_connection_t *http_conn_get(int index)
{
        __ASSERT((0 <= index) && (index < ARRAY_SIZE(connections)),
                 "index out of range");

        return pool[index];
}

static void clear_conn(http_connection_t *conn)
{
        http_parser_init(&conn->parser, HTTP_REQUEST);

        conn->req = NULL;
        conn->resp = NULL;

        conn->complete = 0;
        conn->keep_alive.enabled = 0;
}

int http_conn_get_index(http_connection_t *conn)
{
        for (uint_fast8_t i = 0; i < conns_count; i++) {
                if (pool[i] == conn) {
                        return i;
                }
        }
        return -1;
}

http_connection_t *http_conn_alloc(void)
{
        http_connection_t *conn =  NULL;
        
        if (conns_count < CONFIG_MAX_HTTP_CONNECTIONS) {
                conn = pool[conns_count++];
                clear_conn(conn);
        }

        return conn;
}

void http_conn_free(http_connection_t *conn)
{
        if (conn == NULL) {
                return;
        }

        int index = http_conn_get_index(conn);
        int move_count = CONFIG_MAX_HTTP_CONNECTIONS - index - 1;
        if (move_count > 0) {
                memmove(&pool[index],
                        &pool[index + 1],
                        move_count * sizeof(http_connection_t *));

                /* put the freed connection at the end of the list */
                pool[CONFIG_MAX_HTTP_CONNECTIONS - 1] = conn; 
        }
        conns_count--;
}

bool http_conn_closed(http_connection_t *conn)
{
        return http_conn_get_index(conn) == -1;
}

void http_conn_iterate(void (*callback)(http_connection_t *conn,
					void *user_data),
		       void *user_data)
{
	for (uint32_t i = 0; i < conns_count; i++) {
		callback(pool[i], user_data);
	}
}


bool http_conn_is_outdated(http_connection_t *conn)
{
	uint32_t now = k_uptime_get_32();

	return (now - conn->keep_alive.last_activity) > conn->keep_alive.timeout;
}

int http_conn_get_duration_to_next_outdated_conn(void)
{
	uint32_t now = k_uptime_get_32();

	int timeout = SYS_FOREVER_MS;

	for (uint32_t i = 0U; i < conns_count; i++) {
		const uint32_t diff = now - pool[i]->keep_alive.last_activity;
		if (diff >= pool[i]->keep_alive.timeout) {
			timeout = 0;
			break;
		} else {
			const uint32_t duration = pool[i]->keep_alive.timeout - diff;
			if (timeout == SYS_FOREVER_MS) {
				timeout = duration;
			} else {
				timeout = MIN(timeout, duration);
			}
		}
	}

	return timeout;
}