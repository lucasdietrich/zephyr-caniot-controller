#include "http_conn.h"

#include "http_server.h"

/*___________________________________________________________________________*/

/* connections connections */

uint32_t conns_count = 0;
static http_connection_t connections[CONFIG_CONTROLLER_MAX_HTTP_CONNECTIONS];

/* all active connections stacked a the top of the array */
static http_connection_t *pool[CONFIG_CONTROLLER_MAX_HTTP_CONNECTIONS];

void http_conn_init_pool(void)
{
        for (uint32_t i = 0; i < CONFIG_CONTROLLER_MAX_HTTP_CONNECTIONS; i++) {
                pool[i] = &connections[i];
        }
}

http_connection_t *http_connect_get(int index)
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
        conn->keep_alive = 1;
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
        
        if (conns_count < CONFIG_CONTROLLER_MAX_HTTP_CONNECTIONS) {
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
        int move_count = CONFIG_CONTROLLER_MAX_HTTP_CONNECTIONS - index - 1;
        if (move_count > 0) {
                memmove(&pool[index],
                        &pool[index + 1],
                        move_count * sizeof(http_connection_t *));

                /* put the freed connection at the end of the list */
                pool[CONFIG_CONTROLLER_MAX_HTTP_CONNECTIONS - 1] = conn; 
        }
        conns_count--;
}

bool http_conn_closed(http_connection_t *conn)
{
        return http_conn_get_index(conn) == -1;
}