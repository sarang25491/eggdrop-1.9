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

static int http_on_read(void *client_data, int idx, char *data, int len);
static int http_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int http_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);

sockbuf_handler_t http_events = {
	"http proxy",
	http_on_connect, http_on_eof, NULL,
	http_on_read, NULL
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

static void http_err(proxy_info_t *info, int err, const char *errmsg)
{
	errno = err;
	sockbuf_on_eof(info->their_idx, SOCKBUF_LEVEL_INTERNAL, err, errmsg);
	http_free(info);
}

static int http_on_read(void *client_data, int idx, char *data, int len)
{
	proxy_info_t *info = client_data;

	if (!strncasecmp("HTTP/1.0 200", data, 12)) {
		/* Yay, we're approved. */
		info->status = 1;
		return(0);
	}

	if (len != 0) {
		/* Just some header that we don't care about. */
		return(0);
	}

	/* It's the blank line, so the http response is done. */

	if (info->status != 1) {
		/* Boo, we weren't approved. */
		http_err(info, ECONNREFUSED, "HTTP server refused to relay connection");
		return(0);
	}

	/* Successful, switch it over. */
	sockbuf_set_sock(info->our_idx, -1, 0);
	sockbuf_set_sock(info->their_idx, info->sock, 0);
	sockbuf_on_connect(info->their_idx, SOCKBUF_LEVEL_INTERNAL, info->host, info->port);
	http_free(info);

	return(0);
}

static int http_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	proxy_info_t *info = client_data;

	if (!err) err = ECONNREFUSED;
	http_err(info, err, "Unexpected EOF from HTTP server");
	return(0);
}

static void make_password(char *dest, proxy_info_t *info)
{
	char buf[512];
	snprintf(buf, sizeof(buf), "%s %s", info->username, info->password);
	buf[sizeof(buf)-1] = 0;

	b64enc_buf(buf, strlen(buf), dest);
}

static int http_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	proxy_info_t *info = client_data;
	char request[512], buf[512];

	sprintf(request, "CONNECT %s:%d HTTP/1.0\r\n", info->host, info->port);
	sprintf(buf, "Host: %s\r\n", info->host);
	strcat(request, buf);
	if (strlen(info->username)) {
		char auth[512];

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

	info->our_idx = sockbuf_new();
	sockbuf_set_sock(info->our_idx, sock, SOCKBUF_CLIENT);
	linemode_on(info->our_idx);

	if (idx >= 0) info->their_idx = idx;
	else info->their_idx = sockbuf_new();

	info->sock = sock;

	info->status = 0;

	sockbuf_set_handler(info->our_idx, &http_events, info);

	return(info->their_idx);
}
