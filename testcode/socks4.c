#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include "my_socket.h"
#include "sockbuf.h"

typedef struct {
	unsigned char ver;
	unsigned char cmd;
	unsigned short port;
	unsigned int ip;
} socks4_request_t;

typedef struct {
	char *host;
	int port;
	char *username;
	int our_idx, their_idx;
	int sock;
} proxy_info_t;

static int socks4_on_read(int idx, sockbuf_iobuf_t *data, proxy_info_t *info);
static int socks4_on_eof_and_err(int idx, void *err, proxy_info_t *info);
static int socks4_on_connect(int idx, void *ignore, proxy_info_t *info);

sockbuf_event_t socks4_events = {
	(Function) 5,
	(Function) "socks4 proxy",
	socks4_on_read,
	NULL,
	socks4_on_eof_and_err,
	socks4_on_eof_and_err,
	socks4_on_connect
};

static void socks4_free(proxy_info_t *info)
{
	free(info->host);
	free(info->username);
	sockbuf_delete(info->our_idx);
	free(info);
}

static void socks4_err(proxy_info_t *info, int err)
{
	errno = err;
	sockbuf_filter(info->their_idx, SOCKBUF_ERR, -1, (void *)err);
	socks4_free(info);
}

static int socks4_on_read(int idx, sockbuf_iobuf_t *data, proxy_info_t *info)
{
	socks4_request_t reply;

	if (data->len < sizeof(reply)) {
		socks4_err(info, ECONNABORTED);
		return(0);
	}

	memcpy(&reply, data->data, sizeof(reply));
	if (reply.cmd != 90) {
		socks4_err(info, ECONNREFUSED);
		return(0);
	}

	/* Switch over the socket to their idx. */
	sockbuf_set_sock(info->our_idx, -1, 0);
	sockbuf_set_sock(info->their_idx, info->sock, SOCKBUF_CLIENT);
	socks4_free(info);
	return(0);
}

static int socks4_on_eof_and_err(int idx, void *err, proxy_info_t *info)
{
	if (!err) err = (void *)ECONNREFUSED;
	socks4_err(info, (int) err);
	return(0);
}

static int socks4_on_connect(int idx, void *ignore, proxy_info_t *info)
{
	char buf[512];
	int len;
	struct sockaddr_in addr;
	socks4_request_t req;

	req.ver = 4;
	req.cmd = 1;
	req.port = htons(info->port);
	inet_pton(AF_INET, info->host, &addr);
	req.ip = addr.sin_addr.s_addr;

	memcpy(buf, &req, sizeof(req));

	/* We want to copy the username plus the terminating null. */
	len = strlen(info->username) + 1;
	if (len > sizeof(buf) - sizeof(req)) {
		len = sizeof(buf) - sizeof(req);
		info->username[len-1] = 0;
	}

	memcpy(buf+sizeof(req), info->username, len);
	len += sizeof(req);

	sockbuf_write(idx, buf, len);
	return(0);
}

int socks4_connect(int idx, char *proxy_host, int proxy_port, char *username, char *dest_ip, int dest_port)
{
	int sock;
	proxy_info_t *info;

	sock = socket_create(proxy_host, proxy_port, NULL, 0, SOCKET_CLIENT|SOCKET_TCP|SOCKET_NONBLOCK);
	if (sock < 0) return(-1);

	info = (proxy_info_t *)malloc(sizeof(*info));
	info->host = strdup(dest_ip);
	info->port = dest_port;
	if (!username) username = "";
	info->username = strdup(username);

	info->our_idx = sockbuf_new(sock, SOCKBUF_CLIENT);
	if (idx >= 0) info->their_idx = idx;
	else info->their_idx = sockbuf_new(-1, 0);

	info->sock = sock;

	sockbuf_set_handler(info->our_idx, socks4_events, info);

	return(info->their_idx);
}
