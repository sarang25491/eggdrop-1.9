#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <eggdrop/eggdrop.h>
#include "proxy.h"

extern proxy_config_t proxy_config;

typedef struct {
	char *host;
	int port;
	char *username;
	char *password;
	int status;
	int our_idx, their_idx;
} proxy_info_t;

static int socks5_on_read(void *client_data, int idx, char *data, int len);
static int socks5_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int socks5_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int socks5_on_delete(void *client_data, int idx);

static int socks5_on_their_eof(void *client_data, int idx, int err, const char *errmsg);
static int socks5_on_their_delete(void *client_data, int idx);

static sockbuf_handler_t socks5_events = {
	"socks5 proxy",
	socks5_on_connect, socks5_on_eof, NULL,
	socks5_on_read, NULL,
	socks5_on_delete
};

static sockbuf_filter_t delete_listener = {
	"socks5 proxy",
	SOCKBUF_LEVEL_PROXY,
	NULL, socks5_on_their_eof, NULL,
	NULL, NULL, NULL,
	NULL, socks5_on_their_delete
};

/* When there's an error on our idx, we shut down and notify the other idx. */
/* All errors are non-recoverable. */
static void socks5_err(proxy_info_t *info, int err, const char *errmsg)
{
	/* putlog(LOG_MISC, "*", "socks5 err"); */
	sockbuf_detach_filter(info->their_idx, &delete_listener, NULL);
	errno = err;
	sockbuf_on_eof(info->their_idx, SOCKBUF_LEVEL_INTERNAL, err, errmsg);
	info->their_idx = -1;
	sockbuf_delete(info->our_idx);
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
		len = strlen(info->host) % 255;
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
static void socks5_auth_method(char *data, int len, proxy_info_t *info)
{
	/* If it's a bad reply, abort. */
	if (len < 2) {
		socks5_err(info, ECONNABORTED, "Invalid reply from SOCKS5 server");
		return;
	}

	if (data[0] != 5) {
		socks5_err(info, ECONNABORTED, "SOCKS5 server replied with SOCKS4 protocol");
		return;
	}

	if (data[1] == 0) {
		/* No auth required. */
		send_connect_request(info);
		info->status = 3;
	}
	else if (data[1] == 2) {
		/* User/password authentication */
		int ulen, plen;
		char buf[520];

		/* Username and password can be 255 max. */
		if (info->username) ulen = strlen(info->username) % 255;
		else ulen = 0;
		if (info->password) plen = strlen(info->password) % 255;
		else plen = 0;

		buf[0] = 1;
		buf[1] = ulen;
		memcpy(buf+2, info->username, ulen);
		buf[2+ulen] = plen;
		memcpy(buf+2+ulen+1, info->password, plen);
		sockbuf_write(info->our_idx, buf, 1+1+ulen+1+plen);
		info->status = 2;
	}
	else {
		/* We can't authenticate with this server, boo. */
		socks5_err(info, ECONNABORTED, "SOCKS5 server doesn't accept our methods of authentication");
	}
	return;
}

/* After we send our login information, the server responds with a status code
	to say whether it succeeded or not. */
static void socks5_auth_reply(char *data, int len, proxy_info_t *info)
{
	/* Abort if it's an invalid reply. */
	if (len < 2) {
		socks5_err(info, ECONNABORTED, "Invalid reply from SOCKS5 server");
		return;
	}

	if (data[1] != 0) {
		/* Authentication failed! */
		socks5_err(info, ECONNREFUSED, "SOCKS5 authentication failed");
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
static void socks5_connect_reply(char *data, int len, proxy_info_t *info)
{
	int our_idx, their_idx, sock;

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
	if (len < 2 || data[1] != 0) {
		char *errmsg;

		if (len < 2) errmsg = "Invalid reply from SOCKS5 server";
		else switch (data[1]) {
			case 1: errmsg = "SOCKS5 general server failure"; break;
			case 2: errmsg = "SOCKS5 connection now allowed by ruleset"; break;
			case 3: errmsg = "SOCKS5 network unreachable"; break;
			case 4: errmsg = "SOCKS5 host unreachable"; break;
			case 5: errmsg = "SOCKS5 connection refused"; break;
			case 6: errmsg = "SOCKS5 TTL expired"; break;
			case 7: errmsg = "SOCKS5 command not supported"; break;
			case 8: errmsg = "SOCKS5 address type not supported"; break;
			default: errmsg = "SOCKS5 unknown error";
		}
		socks5_err(info, ECONNABORTED, errmsg);
		return;
	}

	/* We're connected! Simulate a CONNECT event for the other idx. */
	/* putlog(LOG_MISC, "*", "socks5 connected"); */
	our_idx = info->our_idx;
	their_idx = info->their_idx;
	info->their_idx = -1;
	sockbuf_detach_filter(their_idx, &delete_listener, NULL);
	sock = sockbuf_get_sock(our_idx);
	sockbuf_set_sock(our_idx, -1, 0);
	sockbuf_set_sock(their_idx, sock, 0);
	sockbuf_on_connect(their_idx, SOCKBUF_LEVEL_INTERNAL, info->host, info->port);
	sockbuf_delete(our_idx);
	return;
}

/* When we get data from the server, this procedure redirects it to the
	appropriate handler, based on the status field of the idx. */
static int socks5_on_read(void *client_data, int idx, char *data, int len)
{
	proxy_info_t *info = client_data;

	/* putlog(LOG_MISC, "*", "socks5 read, status = %d", info->status); */
	switch (info->status) {
		case 1:
			socks5_auth_method(data, len, info);
			break;
		case 2:
			socks5_auth_reply(data, len, info);
			break;
		case 3:
			socks5_connect_reply(data, len, info);
			break;
	}
	return(0);
}

/* When there's an error or eof on the server's idx, we issue an error on
	the original idx. */
static int socks5_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	proxy_info_t *info = client_data;
	if (!err) err = ECONNREFUSED;
	if (!errmsg) errmsg = "Unexpected EOF from SOCKS5 server";
	socks5_err(info, err, errmsg);
	return(0);
}

/* When our idx is deleted, we have to send an error to the original idx and
 * free our stuff. */
static int socks5_on_delete(void *client_data, int idx)
{
	proxy_info_t *info = client_data;

	/* putlog(LOG_MISC, "*", "socks5 deleted"); */
	if (info->their_idx != -1) {
		int their_idx = info->their_idx;
		sockbuf_detach_filter(their_idx, &delete_listener, NULL);
		sockbuf_on_eof(their_idx, SOCKBUF_LEVEL_INTERNAL, ECONNREFUSED, "socks5 idx deleted unexpectedly!");
	}
	free(info->host);
	if (info->username) free(info->username);
	if (info->password) free(info->password);
	free(info);
	return(0);
}

/* If their idx gets deleted, delete ours as well. */
static int socks5_on_their_delete(void *client_data, int idx)
{
	proxy_info_t *info = client_data;

	info->their_idx = -1;
	sockbuf_delete(info->our_idx);
	return(0);
}

/* Same with an eof on their idx (for timeout). */
static int socks5_on_their_eof(void *client_data, int idx, int err, const char *errmsg)
{
	proxy_info_t *info = client_data;

	info->their_idx = -1;
	sockbuf_delete(info->our_idx);
	return(0);
}

/* When we establish a connection to the server, we have to send it a list of
	authentication methods we support. Then it chooses one and sends back
	a request to use that method. Right now we support "none" (duh) and
	"user/pass". */
static int socks5_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	proxy_info_t *info = client_data;
	char both[] = "\005\002\002\000";
	char anon[] = "\005\001\000";

	/* putlog(LOG_MISC, "*", "socks5 connected!"); */
	/* Send the accepted auth methods (user/pass and/or none). */
	if (info->username && info->password) sockbuf_write(idx, both, 4);
	else sockbuf_write(idx, anon, 3);
	info->status = 1;
	return(0);
}

int socks5_reconnect(int idx, const char *host, int port)
{
	proxy_info_t *info;
	int our_idx;

	if (!proxy_config.host) return(-1);

	our_idx = egg_client(-1, proxy_config.host, proxy_config.port, NULL, 0, -1);

	if (our_idx < 0) return(-1);

	info = (proxy_info_t *)calloc(1, sizeof(*info));
	info->host = strdup(host);
	info->port = port;
	str_redup(&info->username, proxy_config.username);
	str_redup(&info->password, proxy_config.password);
	info->status = 0;

	info->our_idx = our_idx;
	if (idx >= 0) info->their_idx = idx;
	else info->their_idx = sockbuf_new();

	sockbuf_set_handler(info->our_idx, &socks5_events, info);
	sockbuf_attach_filter(info->their_idx, &delete_listener, info);

	return(info->their_idx);
}
