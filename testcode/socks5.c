#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include "my_socket.h"
#include "sockbuf.h"

typedef struct {
	char *host;
	int port;
	char *username;
	char *password;
	int status;
	int our_idx, their_idx;
	int sock;
} proxy_info_t;

static int socks5_on_read(int idx, sockbuf_iobuf_t *data, proxy_info_t *info);
static int socks5_on_eof_and_err(int idx, void *err, proxy_info_t *info);
static int socks5_on_connect(int idx, void *ignore, proxy_info_t *info);

sockbuf_event_t socks5_events = {
	(Function) 5,
	(Function) "socks5 proxy",
	socks5_on_read,
	NULL,
	socks5_on_eof_and_err,
	socks5_on_eof_and_err,
	socks5_on_connect
};

static void socks5_free(proxy_info_t *info)
{
	free(info->host);
	free(info->username);
	free(info->password);
	sockbuf_delete(info->our_idx);
	free(info);
}

/* When there's an error on our idx, we shut down and notify the other idx. */
/* All errors are non-recoverable. */
static void socks5_err(proxy_info_t *info, int err)
{
	errno = err;
	sockbuf_filter(info->their_idx, SOCKBUF_ERR, -1, (void *)err);
	socks5_free(info);
}

/* This sends the connection request once we've authenticated. It calculates
	what address type we're dealing with (ipv4, ipv6, domain name) and
	sends the necessary CONNECT request. */
static void send_connect_request(proxy_info_t *info)
{
	char buf[512];
	int len;
	unsigned short port;
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;

	/* VER        CMD        RESERVED  */
	buf[0] = 5; buf[1] = 1; buf[2] = 0;

	/* Try a regular ipv4 address first. */
	if (inet_pton(AF_INET, info->host, &addr) > 0) {
		buf[3] = 1;
		memcpy(buf+4, &addr.sin_addr, 4);
		len = 8;
	}
	else if (inet_pton(AF_INET6, info->host, &addr6) > 0) {
		buf[3] = 4;
		memcpy(buf+4, &addr6.sin6_addr, 16);
		len = 20;
	}
	else {
		buf[3] = 3;
		len = strlen(info->host);
		buf[4] = len;
		memcpy(buf+5, info->host, len);
		len += 5;
	}

	port = htons(info->port);
	memcpy(buf+len, &port, 2);
	len += 2;

	sockbuf_write(info->our_idx, buf, len);
}

/* This parses the server's auth method reply. Basically, when we connect
	we send the server a list of authentication methods we support. Then
	the server picks one and sends it back. This procedure identifies
	which one was chosen, and then sends the proper login sequence. */
static void socks5_auth_method(sockbuf_iobuf_t *data, proxy_info_t *info)
{
	char buf[520];

	/* If it's a bad reply, abort. */
	if (data->len < 2) {
		socks5_err(info, ECONNABORTED);
		return;
	}

	if ((int) data->data[1] == 0) {
		/* No auth required. */
		send_connect_request(info);
		info->status = 3;
	}
	else if ((int) data->data[1] == 2) {
		/* User/password authentication */
		int ulen, plen;
		char buf[520];

		/* Username and password can be 255 max. */
		ulen = strlen(info->username) % 255;
		plen = strlen(info->password) % 255;

		buf[0] = 5;
		buf[1] = ulen;
		memcpy(buf+2, info->username, ulen);
		buf[2+ulen] = plen;
		memcpy(buf+2+ulen+1, info->password, plen);
		sockbuf_write(info->our_idx, buf, 1+1+ulen+1+plen);
		info->status = 2;
	}
	else {
		/* We can't authenticate with this server, boo. */
		socks5_err(info, ECONNABORTED);
	}
	return;
}

/* After we send our login information, the server respons with a status code
	to say whether it succeeded or not. */
static void socks5_auth_reply(sockbuf_iobuf_t *data, proxy_info_t *info)
{
	/* Abort if it's an invalid reply. */
	if (data->len < 2) {
		socks5_err(info, ECONNABORTED);
		return;
	}

	if (data->data[1] != 0) {
		/* Authentication failed! */
		socks5_err(info, ECONNREFUSED);
	}
	else {
		/* Send the connection request. */
		send_connect_request(info);
		info->status = 3;
	}
}

/* After we send our CONNECT command, the server tries to make the connection.
	When it makes the connection, or fails for some reason, it sends back
	a status code, which we parse here. If the code is successful, then
	we issue a CONNECT event on the original idx. */
static void socks5_connect_reply(sockbuf_iobuf_t *data, proxy_info_t *info)
{
	/* Abort if it's an invalid reply or the connection failed. */
	/* It's actually supposed to be more than 2 bytes, but we only care
		about the 2nd field (status). */

	/* Here are the reply field definitions (from rfc1928)
		0 - success
		1 - general SOCKS server failure
		2 - connection not allowed by ruleset
		3 - network unreachable
		4 - host unreachable
		5 - connection refused
		6 - ttl expired
		7 - command not supported
		8 - address type not supported
	*/
	if (data->len < 2 || data->data[1] != 0) {
		int err;

		if (data->len < 2) err = ECONNABORTED;
		else switch (data->data[1]) {
			case 1: err = ECONNABORTED; break;
			case 3: err = ENETUNREACH; break;
			case 4: err = EHOSTUNREACH; break;
			case 6: err = ETIMEDOUT; break;
			default: err = ECONNREFUSED;
		}
		socks5_err(info, err);
		return;
	}

	/* We're connected! Simulate a CONNECT event for the other idx. */
	sockbuf_set_sock(info->our_idx, -1, 0);
	sockbuf_set_sock(info->their_idx, info->sock, SOCKBUF_CLIENT);
	socks5_free(info);
	return;
}

/* When we get data from the server, this procedure redirects it to the
	appropriate handler, based on the status field of the idx. */
static int socks5_on_read(int idx, sockbuf_iobuf_t *data, proxy_info_t *info)
{
	/* printf("read %d bytes (status %d)\n", data->len, info->status); */
	switch (info->status) {
		case 1:
			socks5_auth_method(data, info);
			break;
		case 2:
			socks5_auth_reply(data, info);
			break;
		case 3:
			socks5_connect_reply(data, info);
			break;
	}
	return(0);
}

/* When there's an error or eof on the server's idx, we issue an error on
	the original idx. */
static int socks5_on_eof_and_err(int idx, void *err, proxy_info_t *info)
{
	if (!err) err = (void *)ECONNREFUSED;
	socks5_err(info, (int) err);
	return(0);
}

/* When we establish a connection to the server, we have to send it a list of
	authentication methods we support. Then it chooses one and sends back
	a request to use that method. Right now we support "none" (duh) and
	"user/pass". */
static int socks5_on_connect(int idx, void *ignore, proxy_info_t *info)
{
	char buf[] = "\005\002\001\000";

	/* Send the accepted auth methods (user/pass and none). */
	sockbuf_write(idx, buf, 4);
	info->status = 1;
	return(0);
}

/* To establish a SOCKS5 connection, we create 2 idx's: one for the caller, and
	one that we use to connect to the server. */
int socks5_connect(int idx, char *proxy_host, int proxy_port, char *username, char *password, char *dest_host, int dest_port)
{
	int sock;
	proxy_info_t *info;

	sock = socket_create(proxy_host, proxy_port, NULL, 0, SOCKET_CLIENT|SOCKET_TCP|SOCKET_NONBLOCK);
	if (sock < 0) return(-1);

	info = (proxy_info_t *)malloc(sizeof(*info));
	info->host = strdup(dest_host);
	info->port = dest_port;
	if (!username) username = "";
	if (!password) password = "";
	info->username = strdup(username);
	info->password = strdup(password);
	info->status = 0;

	info->our_idx = sockbuf_new(sock, SOCKBUF_CLIENT);
	if (idx >= 0) info->their_idx = idx;
	else info->their_idx = sockbuf_new(-1, 0);

	info->sock = sock;

	sockbuf_set_handler(info->our_idx, socks5_events, info);

	return(info->their_idx);
}