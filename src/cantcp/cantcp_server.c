#include <net/socket.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_config.h>
#include <poll.h>

#include "cantcp.h"
#include "cantcp_core.h"

#include "can_if/can_if.h"

/*___________________________________________________________________________*/

#include <logging/log.h>
LOG_MODULE_REGISTER(cantcp_server, LOG_LEVEL_DBG);

/*___________________________________________________________________________*/

static void server(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(cantcp_thread, 0x1000, server, NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, 0);

// TODO
// create a function to attach a msgq for received messages
// K_MSGQ_DEFINE(cantcp_)

/*___________________________________________________________________________*/

#define CANTCP_TUNNEL_PORT  CANTCP_DEFAULT_PORT

#define CANTCP_MAX_CLIENTS  1U

static union
{
	struct pollfd array[1U + CANTCP_MAX_CLIENTS];
	struct
	{
		struct pollfd srv;
		struct pollfd cli[CANTCP_MAX_CLIENTS];
	};
} fds;

static cantcp_tunnel_t *tunnels[CANTCP_MAX_CLIENTS];

static uint32_t connections_count = 0U;

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

/*___________________________________________________________________________*/

static void cantcp_server_tunnel_init(cantcp_tunnel_t *tunnel)
{
	cantcp_core_tunnel_init(tunnel);

	tunnel->flags.mode = CANTCP_SERVER;
}

/*___________________________________________________________________________*/

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

/*___________________________________________________________________________*/

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

static int handle_connection(struct pollfd *pfd, cantcp_tunnel_t *tunnel)
{
	if (!pfd || !tunnel || (pfd->fd != tunnel->sock)) {
		return -EINVAL;
	}

	int rcvd;
	struct zcan_frame msg;

	if (pfd->revents & POLLIN) {
		rcvd = cantcp_core_recv_frame(tunnel, &msg);
		if (rcvd > 0) {
			LOG_HEXDUMP_INF(&msg, rcvd, "Received");

			tunnel->last_keep_alive = k_uptime_get_32();
			if (tunnel->rx_callback) {
				tunnel->rx_callback(tunnel, &msg);
			}

			// TODO concept
			can_queue(&msg);
			
		} else if (rcvd == 0) {
			LOG_WRN("(%d) connection closed", pfd->fd);
			goto cleanup;
		} else {
			LOG_ERR("(%d) failed to recv = %d", pfd->fd, rcvd);
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

	sock = setup_socket();

	for (;;) {
		const uint32_t timeout = get_neareset_timeout();
		ret = zsock_poll(fds.array, 1U + connections_count, timeout);
		if (ret > 0) {
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