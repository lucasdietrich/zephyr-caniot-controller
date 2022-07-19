#if defined(CONFIG_CANTCP_SERVER)

#include <net/socket.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_config.h>
#include <poll.h>
#include <posix/sys/eventfd.h>

#include "cantcp.h"
#include "cantcp_core.h"
#include "cantcp_server.h"

/*____________________________________________________________________________*/

#include <logging/log.h>
LOG_MODULE_REGISTER(cantcp_server, LOG_LEVEL_DBG);

/*____________________________________________________________________________*/

static void server(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(cantcp_thread, 0x1000, server, NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, 0);

// TODO
// create a function to attach a msgq for received messages
// K_MSGQ_DEFINE(cantcp_)

/*____________________________________________________________________________*/

#define CANTCP_TUNNEL_PORT  CANTCP_DEFAULT_PORT

#define CANTCP_SERVER_FD_COUNT  1U
#define CANTCP_MAX_CLIENTS  	1U

#define CANTCP_BASE_FD_COUNT 	(CANTCP_SERVER_FD_COUNT + 1U)

static union
{
	struct pollfd array[1U + CANTCP_SERVER_FD_COUNT + CANTCP_MAX_CLIENTS];
	struct
	{
		struct pollfd control;
		struct pollfd srv;
		struct pollfd cli[CANTCP_MAX_CLIENTS];
	};
} fds;

static cantcp_tunnel_t *tunnels[CANTCP_MAX_CLIENTS];

static uint32_t connections_count = 0U;

K_MSGQ_DEFINE(tx_msgq, sizeof(struct zcan_frame),
	      CANTCP_DEFAULT_MAX_TX_QUEUE_SIZE, 4U);

/*____________________________________________________________________________*/

static int control_event_fd = -1;

typedef enum {
	/**
	 * @brief Notify a message is pending in the TX queue
	 */
	READY_TX_MESSAGE = 1 << 0U,
} control_event_type_t;

static int setup_control_fd(void)
{
	int ret;

	ret = eventfd(0U, EFD_NONBLOCK);
	if (ret < 0) {
		LOG_ERR("eventfd failed: %d", ret);
	} else {
		control_event_fd = ret;
		fds.control.fd = control_event_fd;
		fds.control.events = POLLIN;
	}

	return ret;
}

static inline int notify_control_fd(control_event_type_t type)
{
	return eventfd_write(control_event_fd, (eventfd_t) 1U);
}

int cantcp_server_broadcast(struct zcan_frame *msg)
{
	int ret;

	LOG_DBG("Broadcasting message to listening CAN clients");
	
	ret = k_msgq_put(&tx_msgq, msg, K_NO_WAIT);

	if (ret == 0) {
		ret = notify_control_fd(READY_TX_MESSAGE);
	}

	return ret;
}

/*____________________________________________________________________________*/

static struct k_msgq *common_rx_msgq = NULL;

int cantcp_server_attach_rx_msgq(struct k_msgq *msgq)
{
	common_rx_msgq = msgq;

	return 0;
}

/*____________________________________________________________________________*/

// Get the time until the first tunnel keep-alive timeout
static uint32_t get_neareset_timeout(void)
{
	uint32_t now = k_uptime_get_32();
	uint32_t timeout = UINT32_MAX;

	// compare i to "connections_count" rather ?
	for (uint32_t i = 0U; i < CANTCP_MAX_CLIENTS; i++) {
		cantcp_tunnel_t *const tun = tunnels[i];
		if (tun != NULL) {
			uint32_t diff = now - tun->last_keep_alive;
			uint32_t tunnel_timeout = tun->keep_alive_timeout;

			if (diff < tunnel_timeout) {
				timeout = MIN(timeout, tunnel_timeout - diff);
			} else {
				timeout = 1000U;
				break;
			}
		}
	}

	return timeout;
}

/*____________________________________________________________________________*/

static void cantcp_server_tunnel_init(cantcp_tunnel_t *tunnel)
{
	cantcp_core_tunnel_init(tunnel);

	tunnel->flags.mode = CANTCP_SERVER;
}

/*____________________________________________________________________________*/

K_MEM_SLAB_DEFINE(tunnels_pool, sizeof(struct cantcp_tunnel),
		  CANTCP_MAX_CLIENTS, 4);

static int allocate_tunnel(cantcp_tunnel_t **tunnel)
{
	int ret = k_mem_slab_alloc(&tunnels_pool, (void **)tunnel, K_NO_WAIT);

	if (ret == 0) {
		cantcp_server_tunnel_init(*tunnel);
	}

	return ret;
}

static void free_tunnel(cantcp_tunnel_t **tunnel)
{
	k_mem_slab_free(&tunnels_pool, (void **)tunnel);
}

/*____________________________________________________________________________*/

static int setup_socket(void)
{
	int sock, ret;
	struct sockaddr_in addr;

	sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		return sock;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(CANTCP_TUNNEL_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = zsock_bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		zsock_close(sock);
		return ret;
	}

	ret = zsock_listen(sock, 1);
	if (ret < 0) {
		LOG_ERR("failed to listen socket(%d) = %d", sock, ret);
		zsock_close(sock);
		return ret;
	}

	fds.srv.fd = sock;
	fds.srv.events = POLLIN | POLLERR | POLLHUP;

	return 0U;
}

int accept_connection(int serv_sock)
{
	int ret, sock;
	struct sockaddr_in addr;
	socklen_t len = sizeof(struct sockaddr_in);

	sock = zsock_accept(serv_sock, (struct sockaddr *)&addr, &len);
	if (sock < 0) {
		LOG_ERR("(%d) Accept failed = %d", serv_sock, sock);
		ret = sock;
		goto exit;
	}

	char ipv4_str[NET_IPV4_ADDR_LEN];
	net_addr_ntop(AF_INET, &addr.sin_addr, ipv4_str, sizeof(ipv4_str));

	cantcp_tunnel_t *tunnel = NULL;

	ret = allocate_tunnel(&tunnel);
	if (ret != 0U) {
		LOG_WRN("(%d) Connection refused from %s:%d, cli sock = %d", serv_sock,
			log_strdup(ipv4_str), htons(addr.sin_port), sock);

		zsock_close(sock);

		goto exit;
	}

	LOG_INF("(%d) Connection accepted from %s:%d, cli sock = %d", serv_sock,
		log_strdup(ipv4_str), htons(addr.sin_port), sock);

	// prepare tunnel
	tunnel->sock = sock;
	tunnel->last_keep_alive = k_uptime_get_32();
	tunnel->rx_msgq = common_rx_msgq; /* TODO, make it configurable */

	// prepare next poll
	fds.cli[connections_count].fd = sock;
	fds.cli[connections_count].events = POLLIN | POLLERR | POLLHUP;
	tunnels[connections_count] = tunnel;
	connections_count++;

	return 0;
exit:
	return ret;
}

static void handle_incoming_connection(struct pollfd *pfd)
{
	if (pfd->revents & POLLIN) {
		accept_connection(pfd->fd);
	} else if (pfd->revents & (POLLERR | POLLHUP)) {
		LOG_ERR("(%d) server socket error or hangup (revents = %hhx)", 
			pfd->fd, pfd->revents);
	}
}

static void handle_outgoing_msgs(void)
{
	int ret;
	struct zcan_frame msg;

	if (k_msgq_get(&tx_msgq, &msg, K_NO_WAIT) == 0) {

		/* send to all clients */
		for (uint32_t i = 0U; i < connections_count; i++) {
			cantcp_tunnel_t * tun = tunnels[i];

			LOG_INF("Send CAN message to tunnel %x", (uint32_t)tun);

			ret = cantcp_send(tun, &msg);
			if (ret < 0) {
				cantcp_disconnect(tun);
				free_tunnel(&tun);
				tunnels[0] = NULL;
				connections_count--;
			}
		}
	}
}

static int handle_connection(struct pollfd *pfd, cantcp_tunnel_t *tunnel)
{
	if (!pfd || !tunnel || (pfd->fd != tunnel->sock)) {
		return -EINVAL;
	}

	int rcvd = 0, ret;
	struct zcan_frame msg;

	if (pfd->revents & POLLIN) {
		rcvd = cantcp_core_recv_frame(tunnel, &msg);
		if (rcvd > 0) {
			LOG_HEXDUMP_DBG(&msg, rcvd, "Received");

			tunnel->last_keep_alive = k_uptime_get_32();

			if (tunnel->rx_msgq != NULL) {
				ret = k_msgq_put(tunnel->rx_msgq, &msg, K_NO_WAIT);
				if (ret != 0) {
					LOG_ERR("Failed to queue msg to rx_msgq %x",
						(uint32_t)tunnel->rx_msgq);
				}
			} else {
				LOG_WRN("No msgq to queue msg (%d)", 0);
			}

		} else {
			goto cleanup;
		}

	} else if (pfd->revents & (POLLERR | POLLHUP)) {
		LOG_ERR("client socket error or hangup (revents = %hhx)", pfd->revents);
		goto cleanup;
	}

	return rcvd;

cleanup:
	cantcp_disconnect(tunnel);
	free_tunnel(&tunnel);
	tunnels[0] = NULL;
	connections_count--;
	return rcvd;
}

static void server(void *_a, void *_b, void *_c)
{
	ARG_UNUSED(_a);
	ARG_UNUSED(_b);
	ARG_UNUSED(_c);

	int ret, sock;

	if (setup_control_fd() < 0) {
		return;
	}

	sock = setup_socket();

	for (;;) {
		const uint32_t timeout = get_neareset_timeout();
		ret = zsock_poll(fds.array, CANTCP_BASE_FD_COUNT + connections_count, timeout);
		if (ret > 0) {
			if (fds.control.revents & POLLIN) {
				handle_outgoing_msgs();
			}

			handle_incoming_connection(&fds.srv);

			if (connections_count > 0U) {
				handle_connection(&fds.cli[0], tunnels[0]);
			}
		} else if (ret == 0) {
			LOG_ERR("timeout (%u)", timeout);
		} else {
			LOG_ERR("failed to poll socket(%d) = %d", sock, ret);
		}

		// for each tunnel, check if the only tunnel has been inactive for too long
#if CANTCP_MAX_CLIENTS != 1
#    error More than one client is not supported for now ! HERE !!
#endif		
		if (connections_count > 0U) {
			uint32_t now = k_uptime_get_32();
			cantcp_tunnel_t *tun = tunnels[0U];
			if (now - tun->last_keep_alive >= tun->keep_alive_timeout) {
				LOG_WRN("(%d) keep-alive timeout for tunnel %x", tun->sock, (uint32_t)tun);
				cantcp_disconnect(tun);
				free_tunnel(&tun);
				tunnels[0U] = NULL;
				connections_count--;
			}
		}
	}

	zsock_close(sock);
}

#endif /* defined(CONFIG_CANTCP_SERVER) */