/* eggnet.c: network functions
 *
 * Copyright (C) 2002, 2003, 2004 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef lint
static const char rcsid[] = "$Id: eggnet.c,v 1.9 2003/12/18 23:10:41 stdarg Exp $";
#endif

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <eggdrop/common.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <eggdrop/eggdrop.h>

#define EGGNET_LEVEL (SOCKBUF_LEVEL_INTERNAL+1)

typedef struct connect_info {
	int dns_id;
	int timer_id;
	int idx;
	int port;
} connect_info_t;

static int connect_host_resolved(void *client_data, const char *host, char **ips);
static int egg_connect_timeout(void *client_data);
static int egg_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int egg_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int egg_on_delete(void *client_data, int idx);
static connect_info_t *attach(int idx, const char *host, int port, int timeout);
static int detach(void *client_data, int idx);

static sockbuf_filter_t eggnet_connect_filter = {
	"connect", EGGNET_LEVEL,
	egg_on_connect, egg_on_eof, NULL,
	NULL, NULL, NULL,
	NULL, egg_on_delete
};

static char *default_proxy_name = NULL;
static int default_timeout = 30;

int egg_net_init()
{
	return egg_dns_init();
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
 * proxies or firewalls, just direct connections. 'host' and 'port' are the
 * destination, and 'vip' 'vport' are the source address (vhost). */
int egg_client(int idx, const char *host, int port, const char *vip, int vport, int timeout)
{
	connect_info_t *connect_info;

	/* If they don't have their own idx (-1), create one. */
	if (idx < 0) idx = sockbuf_new();

	/* Resolve the hostname. */
	connect_info = attach(idx, host, port, timeout);
	connect_info->dns_id = egg_dns_lookup(host, -1, connect_host_resolved, connect_info);
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
	egg_proxy_t *proxy;

	if (!default_proxy_name) return egg_client(idx, host, port, NULL, 0, timeout);

	/* If there is a proxy configured, send this request to the right
	 * handler. */
	proxy = egg_proxy_lookup(default_proxy_name);
	if (!proxy) {
		putlog(LOG_MISC, "*", "Proxy '%s' selected but not loaded!", default_proxy_name);
		putlog(LOG_MISC, "*", "Trying to connect without proxy.");
		return egg_client(idx, host, port, NULL, 0, timeout);
	}

	/* Ok, so we will connect through the selected proxy. But we still have
	 * to set up the timeout handler (to avoid duplication of code in the
	 * proxy handlers) if they want one. */
	if (timeout > 0) attach(idx, host, port, timeout);
	proxy->reconnect(idx, host, port);
	return(0);
}

int egg_connect(const char *host, int port, int timeout)
{
	int idx;

	idx = sockbuf_new();
	egg_reconnect(idx, host, port, timeout);
	return(idx);
}

/* Called when dns is done resolving from egg_client().
 * Now we have an ip address to connect to. */
static int connect_host_resolved(void *client_data, const char *host, char **ips)
{
	connect_info_t *connect_info = client_data;
	int sock, idx;

	connect_info->dns_id = -1;

	if (!ips || !ips[0]) {
		idx = connect_info->idx;
		detach(client_data, idx);
		sockbuf_on_eof(idx, EGGNET_LEVEL, -1, "Could not resolve host");
		return(0);
	}

	sock = socket_create(ips[0], connect_info->port, NULL, 0, SOCKET_CLIENT|SOCKET_TCP|SOCKET_NONBLOCK);
	if (sock < 0) {
		idx = connect_info->idx;
		detach(client_data, idx);
		sockbuf_on_eof(idx, EGGNET_LEVEL, -1, "Could not create socket");
		return(0);
	}

	/* Put this socket into the original sockbuf as a client. This allows
	 * its connect event to trigger. We don't detach() from it yet, since
	 * we provide the timeout services. */
	sockbuf_set_sock(connect_info->idx, sock, SOCKBUF_CLIENT);
	return(0);
}

/* If a connection times out, due to dns timeout or connect timeout. */
static int egg_connect_timeout(void *client_data)
{
	connect_info_t *connect_info = client_data;
	int idx, dns_id;

	idx = connect_info->idx;
	dns_id = connect_info->dns_id;
	connect_info->timer_id = -1;
	if (dns_id != -1) {
		/* dns_cancel will call connect_host_resolved for us, which
		 * will filter up a "dns failed" error. */
		egg_dns_cancel(dns_id, 1);
	}
	else {
		detach(client_data, idx);
		sockbuf_on_eof(idx, EGGNET_LEVEL, -1, "Connect timed out");
	}
	return(0);
}

/* When the idx connects, we're done.. just detach() and pass it along. */
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

static connect_info_t *attach(int idx, const char *host, int port, int timeout)
{
	connect_info_t *connect_info = calloc(1, sizeof(*connect_info));

	connect_info->port = port;
	connect_info->idx = idx;
	connect_info->dns_id = -1;
	connect_info->timer_id = -1;
	sockbuf_attach_filter(connect_info->idx, &eggnet_connect_filter, connect_info);
	if (timeout == 0) timeout = default_timeout;
	if (timeout > 0) {
		char buf[128];
		egg_timeval_t howlong;

		snprintf(buf, sizeof(buf), "idx %d to %s/%d", idx, host, port);
		howlong.sec = timeout;
		howlong.usec = 0;
		connect_info->timer_id = timer_create_complex(&howlong, buf, egg_connect_timeout, connect_info, 0);
	}
	return(connect_info);
}

static int detach(void *client_data, int idx)
{
	connect_info_t *connect_info = client_data;

	if (connect_info->timer_id != -1) timer_destroy(connect_info->timer_id);
	sockbuf_detach_filter(idx, &eggnet_connect_filter, NULL);
	free(connect_info);
	return(0);
}




/* Proxy stuff. */

static egg_proxy_t **proxies = NULL;
static int nproxies = 0;

int egg_proxy_add(egg_proxy_t *proxy)
{
	proxies = realloc(proxies, (nproxies+1) * sizeof(*proxies));
	proxies[nproxies++] = proxy;
	return(0);
}

int egg_proxy_del(egg_proxy_t *proxy)
{
	int i;

	for (i = 0; i < nproxies; i++) {
		if (proxies[i] == proxy) break;
	}
	if (i == nproxies) return(-1);
	memmove(proxies+i, proxies+i+1, (nproxies-i-1) * sizeof(*proxies));
	nproxies--;
	return(0);
}

egg_proxy_t *egg_proxy_lookup(const char *name)
{
	int i;

	for (i = 0; i < nproxies; i++) {
		if (proxies[i]->name && !strcasecmp(proxies[i]->name, name)) return(proxies[i]);
	}
	return(NULL);
}

int egg_proxy_set_default(const char *name)
{
	str_redup(&default_proxy_name, name);
	return(0);
}

const char *egg_proxy_get_default()
{
	return(default_proxy_name);
}
