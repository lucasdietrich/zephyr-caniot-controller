#include "http_conn.h"

#include "http_server.h"

int conns_count = 0;
static struct connection pool[MAX_CONNECTIONS];

/* all active connections stacked a the top of the array */
static struct connection *alloc[MAX_CONNECTIONS] = {
        &pool[0], &pool[1], &pool[2]
};

struct connection *get_connection(int index)
{
        __ASSERT((0 <= index) && (index < ARRAY_SIZE(pool)), "index out of range");

        return alloc[index];
}

int conn_get_index(struct connection *conn)
{
        for (uint_fast8_t i = 0; i < conns_count; i++) {
                if (alloc[i] == conn) {
                        return i;
                }
        }
        return -1;
}

void clear_conn(struct connection *conn)
{
        http_parser_init(&conn->parser, HTTP_REQUEST);
        
        conn->keep_alive = 0;
        conn->complete = 0;
}

struct connection *alloc_connection(void)
{
        struct connection *conn =  NULL;
        
        if (conns_count < MAX_CONNECTIONS) {
                conn = alloc[conns_count++];
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
                memmove(&alloc[index],
                        &alloc[index + 1],
                        move_count * sizeof(struct connection *));

                        /* put the freed connection at the end of the list */
                alloc[MAX_CONNECTIONS - 1] = conn; 
        }
        conns_count--;
}

bool conn_is_closed(struct connection *conn)
{
        return conn_get_index(conn) == -1;
}