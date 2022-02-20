#include <net/socket.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_config.h>
#include <poll.h>

#include "cantcp.h"

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

static uint32_t connections_count = 0U;

static uint32_t keep_alive_timeout[CANTCP_MAX_CLIENTS];

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

	if (connections_count < CANTCP_MAX_CLIENTS) {

		LOG_INF("(%d) Connection accepted from %s:%d, cli sock = %d", serv_sock,
			log_strdup(ipv4_str), htons(addr.sin_port), sock);

		fds.cli[connections_count].fd = sock;
		fds.cli[connections_count].events = POLLIN | POLLERR | POLLHUP;

		// todo
		keep_alive_timeout[connections_count] = k_uptime_get_32();

		connections_count++;
	} else {
		LOG_WRN("(%d) Connection refused from %s:%d, cli sock = %d", serv_sock,
			log_strdup(ipv4_str), htons(addr.sin_port), sock);

		zsock_close(sock);

		ret = -1;
		goto exit;
	}

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

static int recv_all(int sock, uint8_t *buf, size_t len)
{
	ssize_t ret = -EINVAL;

	while (len > 0U) {
		ret = zsock_recv(sock, buf, len, 0);
		if (ret > 0) {
			buf += ret;
			len -= ret;
			LOG_HEXDUMP_DBG(buf - ret, ret, "received");
		} else if (ret == 0) {
			zsock_close(sock);
			break;
		} else if (ret < 0) {
			LOG_ERR("(%d) failed to recv = %d", sock, ret);
			break;
		}
	}

	return ret;
}

static int send_all(int sock, uint8_t *buf, size_t len)
{
	ssize_t ret = -EINVAL;

	while (len > 0U) {
		ret = zsock_send(sock, buf, len, 0);
		if (ret > 0) {
			buf += ret;
			len -= ret;
		} else if (ret == 0) {
			zsock_close(sock);
			break;
		} else if (ret < 0) {
			LOG_ERR("(%d) failed to send = %d", sock, ret);
			break;
		}
	}

	return ret;
}

static void handle_connection(struct pollfd *pfd)
{
	static uint8_t buffer[10];

	int received;

	if (pfd->revents & POLLIN) {
		received = recv_all(pfd->fd, buffer, sizeof(buffer));
		if (received <= 0U) {
			connections_count--;
			return;
		}

		if (send_all(pfd->fd, buffer, received) <= 0U) {
			connections_count--;
			return;
		}

	} else if (pfd->revents & (POLLERR | POLLHUP)) {
		LOG_ERR("client socket error or hangup (revents = %hhx)", pfd->revents);
	}
}

static void server(void *_a, void *_b, void *_c)
{
	ARG_UNUSED(_a);
	ARG_UNUSED(_b);
	ARG_UNUSED(_c);

	int ret, sock;

	sock = setup_socket();

	for (;;) {
		ret = zsock_poll(fds.array, 1U + connections_count, SYS_FOREVER_MS);
		if (ret > 0) {
			handle_incoming_connection(&fds.srv);

			if (connections_count > 0U) {
				handle_connection(&fds.cli[0]);
			}
		} else if (ret == 0) {
			LOG_ERR("timeout (%u)", SYS_FOREVER_MS);
		} else {
			LOG_ERR("failed to poll socket(%d) = %d", sock, ret);
		}
	}

	zsock_close(sock);
}