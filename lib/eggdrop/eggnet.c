#if HAVE_CONFIG_H
	#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "memutil.h"
#include "my_socket.h"
#include "sockbuf.h"
#include "eggnet.h"
#include "eggdns.h"
#include "eggtimer.h"

#define EGGNET_LEVEL	(SOCKBUF_LEVEL_INTERNAL+1)

typedef struct connect_info {
	int dns_id;
	int timer_id;
	int idx;
	int port;
} connect_info_t;

static int connect_host_resolved(void *client_data, const char *host, const char *ip);
static int egg_connect_timeout(void *client_data);
static int egg_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int egg_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int egg_on_delete(void *client_data, int idx);
static int detach(void *client_data, int idx);

static sockbuf_filter_t eggnet_connect_filter = {
	"connect", EGGNET_LEVEL,
	egg_on_connect, egg_on_eof, NULL,
	NULL, NULL, NULL,
	NULL, egg_on_delete
};

int egg_net_init()
{
	egg_dns_init();
}

/* Print to an idx, using printf notation. */
int egg_iprintf(int idx, const char *format, ...)
{
	char *ptr, buf[1024];
	int len;
	va_list args;

	va_start(args, format);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, format, args);
	va_end(args);
	sockbuf_write(idx, ptr, len);
	if (ptr != buf) free(ptr);
	return(len);
}

/* Create a listening socket on a port. */
int egg_server(const char *vip, int port, int *real_port)
{
	int idx, sock, ntries;

	ntries = 0;
	do {
		sock = socket_create(NULL, 0, vip, port+ntries, SOCKET_SERVER|SOCKET_TCP|SOCKET_NONBLOCK);
		ntries++;
	} while (sock < 0 && ntries < 20 && port);

	if (sock < 0) return(sock);

	if (real_port) socket_get_name(sock, NULL, real_port);

	idx = sockbuf_new();
	sockbuf_set_sock(idx, sock, SOCKBUF_SERVER);

	return(idx);
}

/* Create a client socket and attach it to an idx. This does not handle
 * proxies or firewalls, just direct connections. 'ip' and 'port' are the
 * destination, and 'vip' 'vport' are the source address (vhost). */
int egg_client(const char *ip, int port, const char *vip, int vport)
{
	int idx, sock;

	sock = socket_create(ip, port, vip, vport, SOCKET_CLIENT|SOCKET_TCP|SOCKET_NONBLOCK);
	if (sock < 0) return(-1);
	idx = sockbuf_new();
	sockbuf_set_sock(idx, sock, SOCKBUF_CLIENT);
	return(idx);
}

/* Open a listen port. If proxies/firewalls/vhosts are configured, it will
 * automatically use them. Returns an idx for the new socket. */
int egg_listen(int port, int *real_port)
{
	int idx;

	/* Proxy/firewall/vhost code goes here. */
	idx = egg_server(NULL, port, real_port);
	return(idx);
}

/* Connect to a given host/port. If proxies/firewalls/vhosts are configured, it
 * will automatically use them. Returns an idx for your connection. */
int egg_reconnect(int idx, const char *host, int port, int timeout)
{
	connect_info_t *connect_info;
	egg_timeval_t howlong;

	/* Resolve the hostname. */
	connect_info = calloc(1, sizeof(*connect_info));
	connect_info->port = port;
	connect_info->idx = idx;
	sockbuf_attach_filter(connect_info->idx, &eggnet_connect_filter, connect_info);
	connect_info->timer_id = -1;
	connect_info->dns_id = egg_dns_lookup(host, DNS_IPV4, connect_host_resolved, connect_info);
	if (timeout > 0) {
		char buf[128];

		snprintf(buf, sizeof(buf), "idx %d to %s/%d", idx, host, port);
		howlong.sec = timeout;
		howlong.usec = 0;
		connect_info->timer_id = timer_create_complex(&howlong, buf, egg_connect_timeout, connect_info, 0);
	}
	return(connect_info->idx);
}

int egg_connect(const char *host, int port, int timeout)
{
	int idx;

	idx = sockbuf_new();
	egg_reconnect(idx, host, port, timeout);
	return(idx);
}

static int connect_host_resolved(void *client_data, const char *host, const char *ip)
{
	connect_info_t *connect_info = client_data;
	int sock, idx;

	connect_info->dns_id = -1;

	if (!ip) {
		idx = connect_info->idx;
		detach(client_data, idx);
		sockbuf_on_eof(idx, EGGNET_LEVEL, -1, "Could not resolve host");
		return(0);
	}

	/* Proxy/firewall/vhost code goes here. */

	sock = socket_create(ip, connect_info->port, NULL, 0, SOCKET_CLIENT|SOCKET_TCP|SOCKET_NONBLOCK);
	if (sock < 0) {
		idx = connect_info->idx;
		detach(client_data, idx);
		sockbuf_on_eof(idx, EGGNET_LEVEL, -1, "Could not create socket");
		return(0);
	}
	sockbuf_set_sock(connect_info->idx, sock, SOCKBUF_CLIENT);
	return(0);
}

static int egg_connect_timeout(void *client_data)
{
	connect_info_t *connect_info = client_data;
	int idx, dns_id;

	idx = connect_info->idx;
	dns_id = connect_info->dns_id;
	connect_info->timer_id = -1;
	if (dns_id != -1) {
		/* dns_cancel will call connect_host_resolved for us. */
		egg_dns_cancel(dns_id, 1);
	}
	else {
		detach(client_data, idx);
		sockbuf_on_eof(idx, EGGNET_LEVEL, -1, "Connect timed out");
	}
	return(0);
}

static int egg_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	detach(client_data, idx);
	sockbuf_on_connect(idx, EGGNET_LEVEL, peer_ip, peer_port);
	return(0);
}

static int egg_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	detach(client_data, idx);
	return sockbuf_on_eof(idx, EGGNET_LEVEL, err, errmsg);
}

static int egg_on_delete(void *client_data, int idx)
{
	connect_info_t *connect_info = client_data;

	if (connect_info->dns_id != -1) egg_dns_cancel(connect_info->dns_id, 0);
	if (connect_info->timer_id != -1) timer_destroy(connect_info->timer_id);
	free(connect_info);
	return(0);
}

static int detach(void *client_data, int idx)
{
	connect_info_t *connect_info = client_data;

	if (connect_info->timer_id != -1) timer_destroy(connect_info->timer_id);
	sockbuf_detach_filter(idx, &eggnet_connect_filter, NULL);
	free(connect_info);
	return(0);
}
