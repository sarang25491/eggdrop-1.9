#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <eggdrop/eggdrop.h>
#include "proxy.h"

typedef struct {
	char *http_host;
	char *host;
	int port;
	char *username;
	char *password;
	int our_idx, their_idx;
	int status;
} proxy_info_t;

static int http_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int http_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int http_on_read(void *client_data, int idx, char *data, int len);
static int http_on_delete(void *client_data, int idx);
static int http_on_their_delete(void *client_data, int idx);

static sockbuf_handler_t http_events = {
	"http proxy",
	http_on_connect, http_on_eof, NULL,
	http_on_read, NULL,
	http_on_delete
};

static sockbuf_filter_t delete_listener = {
	"http proxy",
	SOCKBUF_LEVEL_PROXY,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, http_on_their_delete
};

static void http_err(proxy_info_t *info, int err, const char *errmsg)
{
	int idx = info->their_idx;
	errno = err;
	sockbuf_delete(info->our_idx);
	sockbuf_on_eof(idx, SOCKBUF_LEVEL_INTERNAL, err, errmsg);
}

static int http_on_delete(void *client_data, int idx)
{
	proxy_info_t *info = client_data;
	sockbuf_detach_filter(info->their_idx, &delete_listener, info);
	if (info->host) free(info->host);
	if (info->username) free(info->username);
	if (info->password) free(info->password);
	free(info);
	return(0);
}

static int http_on_read(void *client_data, int idx, char *data, int len)
{
	proxy_info_t *info = client_data;
	int sock;

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
	sockbuf_detach_filter(info->their_idx, &delete_listener, info);
	sock = sockbuf_get_sock(info->our_idx);
	sockbuf_set_sock(info->our_idx, -1, 0);
	sockbuf_set_sock(info->their_idx, sock, 0);
	sockbuf_on_connect(info->their_idx, SOCKBUF_LEVEL_INTERNAL, info->host, info->port);
	info->their_idx = -1;
	sockbuf_delete(info->our_idx);

	return(0);
}

static int http_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	proxy_info_t *info = client_data;

	if (!err) err = ECONNREFUSED;
	http_err(info, err, "Unexpected EOF from HTTP server");
	return(0);
}

static char *make_password(proxy_info_t *info)
{
	char *buf, *dest;
	int len;

	buf = egg_mprintf("%s %s", info->username, info->password);
	len = strlen(buf);
	dest = malloc(len * 4 + 1);
	b64enc_buf(buf, len, dest);
	free(buf);
	return(dest);
}

/* When we connect to the httpd, send the CONNECT request and wait for the
 * response. */
static int http_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	proxy_info_t *info = client_data;

	egg_iprintf(idx, "CONNECT %s:%d HTTP/1.0\r\n", info->host, info->port);
	egg_iprintf(idx, "Host: %s\r\n", info->http_host);
	if (info->username && info->password && strlen(info->username)) {
		char *auth;

		auth = make_password(info);
		egg_iprintf(idx, "Proxy-authenticate: basic %s\r\n", auth);
		free(auth);
	}
	egg_iprintf(idx, "\r\n");

	return(0);
}

/* If they delete their idx (timeout?), delete ours as well. */
static int http_on_their_delete(void *client_data, int idx)
{
	proxy_info_t *info = client_data;
	sockbuf_delete(info->our_idx);
	return(0);
}

int http_reconnect(int idx, const char *host, int port)
{
	int our_idx;
	proxy_info_t *info;

	if (!proxy_config.host) return(-1);

	our_idx = egg_client(-1, proxy_config.host, proxy_config.port, NULL, 0, -1);
	if (our_idx < 0) return(-1);

	/* Save our destination. */
	info = (proxy_info_t *)calloc(1, sizeof(*info));
	info->host = strdup(host);
	info->port = port;
	/* Also save the proxy info, in case the config changes while we're
	 * connecting. */
	info->http_host = strdup(proxy_config.host);
	if (proxy_config.username) info->username = strdup(proxy_config.username);
	if (proxy_config.password) info->password = strdup(proxy_config.password);

	info->our_idx = our_idx;
	linemode_on(info->our_idx);

	if (idx >= 0) info->their_idx = idx;
	else info->their_idx = sockbuf_new();

	info->status = 0;

	sockbuf_set_handler(info->our_idx, &http_events, info);
	sockbuf_attach_filter(info->their_idx, &delete_listener, info);

	return(info->their_idx);
}
