#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include "my_socket.h"
#include "sockbuf.h"

#define SOCKS4_LEVEL	SOCKBUF_LEVEL_PROXY

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

static int socks4_on_read(void *client_data, int idx, char *data, int len);
static int socks4_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int socks4_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);

static sockbuf_handler_t socks4_events = {
	"socks4 proxy",
	socks4_on_connect, socks4_on_eof, NULL,
	socks4_on_read, NULL
};

static void socks4_free(proxy_info_t *info)
{
	free(info->host);
	free(info->username);
	sockbuf_delete(info->our_idx);
	free(info);
}

static int socks4_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	proxy_info_t *info = client_data;

	sockbuf_on_eof(info->their_idx, SOCKBUF_LEVEL_INTERNAL, err, errmsg);
	socks4_free(info);
	return(0);
}

static int socks4_on_read(void *client_data, int idx, char *data, int len)
{
	proxy_info_t *info = client_data;
	socks4_request_t reply;

	if (len < sizeof(reply)) {
		socks4_on_eof(info, idx, ECONNABORTED, "Invalid reply from SOCKS4 server");
		return(0);
	}

	memcpy(&reply, data, sizeof(reply));
	if (reply.cmd != 90) {
		socks4_on_eof(info, idx, ECONNREFUSED, "SOCKS4 server refused to relay connection");
		return(0);
	}

	/* Switch over the socket to their idx. */
	sockbuf_set_sock(info->our_idx, -1, 0);
	sockbuf_set_sock(info->their_idx, info->sock, 0);
	sockbuf_on_connect(info->their_idx, SOCKBUF_LEVEL_INTERNAL, info->host, info->port);
	socks4_free(info);
	return(0);
}

static int socks4_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	proxy_info_t *info = client_data;
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

	info->our_idx = sockbuf_new();
	sockbuf_set_sock(info->our_idx, sock, SOCKBUF_CLIENT);
	if (idx >= 0) info->their_idx = idx;
	else info->their_idx = sockbuf_new();

	info->sock = sock;

	sockbuf_set_handler(info->our_idx, &socks4_events, info);

	return(info->their_idx);
}
