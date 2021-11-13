#include "http_conn.h"

#include "http_server.h"

/*___________________________________________________________________________*/

/* connections connections */

int conns_count = 0;
static struct connection connections[MAX_CONNECTIONS];

/* all active connections stacked a the top of the array */
static struct connection *pool[MAX_CONNECTIONS] = {
        &connections[0], &connections[1], &connections[2]
};

struct connection *get_connection(int index)
{
        __ASSERT((0 <= index) && (index < ARRAY_SIZE(connections)),
                 "index out of range");

        return pool[index];
}

int conn_get_index(struct connection *conn)
{
        for (uint_fast8_t i = 0; i < conns_count; i++) {
                if (pool[i] == conn) {
                        return i;
                }
        }
        return -1;
}

struct connection *alloc_connection(void)
{
        struct connection *conn =  NULL;
        
        if (conns_count < MAX_CONNECTIONS) {
                conn = pool[conns_count++];
                clear_conn(conn);
        }

        return conn;
}

void free_connection(struct connection *conn)
{
        if (conn == NULL) {
                return;
        }

        int index = conn_get_index(conn);
        int move_count = MAX_CONNECTIONS - index - 1;
        if (move_count > 0) {
                memmove(&pool[index],
                        &pool[index + 1],
                        move_count * sizeof(struct connection *));

                /* put the freed connection at the end of the list */
                pool[MAX_CONNECTIONS - 1] = conn; 
        }
        conns_count--;
}

bool conn_is_closed(struct connection *conn)
{
        return conn_get_index(conn) == -1;
}

void clear_conn(struct connection *conn)
{
        http_parser_init(&conn->parser, HTTP_REQUEST);

        conn->req = NULL;
        conn->resp = NULL;

        conn->complete = 0;
        conn->keep_alive = 1;
}