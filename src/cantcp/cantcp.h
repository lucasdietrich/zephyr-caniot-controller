/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANTCP_H
#define _CANTCP_H

#include <zephyr/kernel.h>

#include <zephyr/drivers/can.h>
#include <zephyr/net/net_ip.h>




#define CANTCP_DEFAULT_PORT 			5555U

#define CANTCP_DEFAULT_KEEP_ALIVE_TIMEOUT	10000U

#define CANTCP_DEFAULT_MAX_RETRIES		0U

#define CANTCP_DEFAULT_RETRY_DELAY		1000U

#define CANTCP_DEFAULT_MAX_TX_QUEUE_SIZE	10U




typedef enum
{
	CANTCP_UNSECURE = 0,
	CANTCP_SECURE = 1,
} cantcp_secure_t;

typedef enum
{
	CANTCP_BLOCKING = 0,
	CANTCP_BLOCKING_MODE = 1,
} CANTCP_BLOCKING_MODE_t;

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

// typedef void (*cantcp_rx_callback_t)(cantcp_tunnel_t *tunnel, struct can_frame *msg);

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
		
		cantcp_busindex_t bus: 3;
		CANTCP_BLOCKING_MODE_t blocking_mode: 1;
	} flags;

	/* other configuration */
	uint8_t max_retries;
	uint32_t retry_delay;			/* in milliseconds */
	uint32_t keep_alive_timeout; 		/* in milliseconds */

	/* TLS secure parameters */
	struct {
		char *ca_cert; 		/* PEM */
		char *client_cert; 	/* PEM */
		char *client_key; 	/* PEM */
	} tls;

	/* internal */
	int sock;

	uint32_t last_keep_alive; /* last keep alive time (in milliseconds) */

	struct k_msgq *rx_msgq;
};

/* client */
void cantcp_client_tunnel_init(cantcp_tunnel_t *tunnel);

int cantcp_connect(cantcp_tunnel_t *tunnel);

int cantcp_disconnect(cantcp_tunnel_t *tunnel);

/* server & client */
int cantcp_send(cantcp_tunnel_t *tunnel, struct can_frame *msg);

int cantcp_recv(cantcp_tunnel_t *tunnel, struct can_frame *msg);

int cantcp_live(cantcp_tunnel_t *tunnel);

int cantcp_attach_msgq(cantcp_tunnel_t *tunnel, struct k_msgq *rx_queue);

bool cantcp_connected(cantcp_tunnel_t *tunnel);

#endif /* _CANTCP_H */