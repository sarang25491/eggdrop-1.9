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
	int our_idx, their_idx;
	int sock;
	int status;
} proxy_info_t;

static int http_on_read(int idx, sockbuf_iobuf_t *data, proxy_info_t *info);
static int http_on_eof_and_err(int idx, void *err, proxy_info_t *info);
static int http_on_connect(int idx, void *ignore, proxy_info_t *info);

sockbuf_event_t http_events = {
	(Function) 5,
	(Function) "http proxy",
	http_on_read,
	NULL,
	http_on_eof_and_err,
	http_on_eof_and_err,
	http_on_connect
};

static void http_free(proxy_info_t *info)
{
	free(info->host);
	free(info->username);
	free(info->password);
	//linemode_off(info->our_idx);
	sockbuf_delete(info->our_idx);
	free(info);
}

static void http_err(proxy_info_t *info, int err)
{
	errno = err;
	sockbuf_filter(info->their_idx, SOCKBUF_ERR, -1, (void *)err);
	http_free(info);
}

static int http_on_read(int idx, sockbuf_iobuf_t *data, proxy_info_t *info)
{
	if (!strncasecmp("HTTP/1.0 200", data->data, 12)) {
		/* Yay, we're approved. */
		info->status = 1;
		return(0);
	}

	if (data->len != 0) {
		/* Just some header that we don't care about. */
		return(0);
	}

	/* It's the blank line, so the http response is done. */

	if (info->status != 1) {
		/* Boo, we weren't approved. */
		http_err(info, ECONNREFUSED);
		return(0);
	}

	/* Successful, switch it over. */
	sockbuf_set_sock(info->our_idx, -1, 0);
	sockbuf_set_sock(info->their_idx, info->sock, SOCKBUF_CLIENT);
	http_free(info);

	return(0);
}

static int http_on_eof_and_err(int idx, void *err, proxy_info_t *info)
{
	if (!err) err = (void *)ECONNREFUSED;
	http_err(info, (int) err);
	return(0);
}

static void make_password(char *dest, proxy_info_t *info)
{
	char buf[512];
	sprintf(buf, "%s %s", info->username, info->password);

	b64enc_buf(buf, strlen(buf), dest);
}

static int http_on_connect(int idx, void *ignore, proxy_info_t *info)
{
	char request[512];

	sprintf(request, "CONNECT %s:%d HTTP/1.0\r\n", info->host, info->port);
	if (strlen(info->username)) {
		char buf[512], auth[512];

		make_password(auth, info);
		sprintf(buf, "Proxy-authenticate: basic %s\r\n", auth);
		strcat(request, buf);
	}
	strcat(request, "\r\n");

	sockbuf_write(idx, request, strlen(request));

	return(0);
}

int http_connect(int idx, char *proxy_host, int proxy_port, char *username, char *password, char *dest_ip, int dest_port)
{
	int sock;
	proxy_info_t *info;

	sock = socket_create(proxy_host, proxy_port, NULL, 0, SOCKET_CLIENT|SOCKET_TCP|SOCKET_NONBLOCK);
	if (sock < 0) return(-1);

	info = (proxy_info_t *)malloc(sizeof(*info));
	info->host = strdup(dest_ip);
	info->port = dest_port;
	if (!username) username = "";
	if (!password) password = "";
	info->username = strdup(username);
	info->password = strdup(password);

	info->our_idx = sockbuf_new(sock, SOCKBUF_CLIENT);
	linemode_on(info->our_idx);

	if (idx >= 0) info->their_idx = idx;
	else info->their_idx = sockbuf_new(-1, 0);

	info->sock = sock;

	info->status = 0;

	sockbuf_set_handler(info->our_idx, http_events, info);

	return(info->their_idx);
}
