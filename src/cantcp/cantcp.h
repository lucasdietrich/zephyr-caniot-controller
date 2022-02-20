#ifndef _CANTCP_H
#define _CANTCP_H

#include <kernel.h>

#include <drivers/can.h>
#include <net/net_ip.h>

/*___________________________________________________________________________*/

#define CANTCP_DEFAULT_PORT 5555U

/*___________________________________________________________________________*/

typedef enum
{
	CANTCP_UNSECURE = 0,
	CANTCP_SECURE = 1,
} cantcp_secure_t;

typedef enum
{
	CANTCP_BLOCKING = 0,
	CANTCP_NONBLOCKING = 1,
} cantcp_blocking_mode_t;

typedef enum
{
	CANTCP_CLIENT = 0,
	CANTCP_SERVER = 1,
} cantcp_mode_t;

typedef enum
{
	CANTCP_BUS_DEFAULT = 0,
	CANTCP_BUS_1 = 1,
	CANTCP_BUS_2 = 2,
	CANTCP_BUS_3 = 3,
	CANTCP_BUS_4 = 4,
	CANTCP_BUS_5 = 5,
	CANTCP_BUS_6 = 6,
	CANTCP_BUS_7 = 7,
} cantcp_busindex_t;

typedef struct cantcp_tunnel cantcp_tunnel_t;

typedef void (*cantcp_rx_callback_t)(cantcp_tunnel_t *tunnel, struct zcan_frame *msg);

struct cantcp_tunnel
{
	/* server */
	struct
	{
		char *hostname;
		uint16_t port;
		union {
			struct sockaddr addr;
			struct sockaddr_in addr4;
			// struct sockaddr_in6 addr6;
		};
	} server;

	/* flags */
	struct {
		cantcp_mode_t mode: 1;
		cantcp_secure_t secure: 1;
		cantcp_blocking_mode_t nonblocking: 1;
		cantcp_busindex_t bus: 3;
	} flags;

	/* other configuration */
	uint8_t max_retries;
	uint8_t retry_delay;			/* in milliseconds */
	uint32_t keep_alive_timeout; 	/* in seconds */

	/* TLS secure parameters */
	struct {
		char *ca_cert; 		/* PEM */
		char *client_cert; 	/* PEM */
		char *client_key; 	/* PEM */
	} tls;

	/* internal */
	int sock;

	/* handlers */
	cantcp_rx_callback_t rx_callback;
};

void cantcp_tunnel_init(cantcp_tunnel_t *tunnel);

int cantcp_connect(cantcp_tunnel_t *tunnel);

int cantcp_attach_rxcb(cantcp_tunnel_t *tunnel, cantcp_rx_callback_t rx_cb);

int cantcp_send(cantcp_tunnel_t *tunnel, struct zcan_frame *msg);

int cantcp_recv(cantcp_tunnel_t *tunnel, struct zcan_frame *msg);

int cantcp_disconnect(cantcp_tunnel_t *tunnel);

bool cantcp_connected(cantcp_tunnel_t *tunnel);

// int cantcp_control(cantcp_tunnel_t *tunnel, uint8_t cmd, void *arg);


#endif /* _CANTCP_H */