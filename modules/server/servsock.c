/* servsock.c: server socket functions
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
static const char rcsid[] = "$Id: servsock.c,v 1.24 2004/10/01 16:13:31 stdarg Exp $";
#endif

#include "server.h"

/* From server.c. */
extern int cycle_delay;

static void disconnect_server();
static int server_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int server_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int server_on_read(void *client_data, int idx, char *text, int len);
static int server_on_delete(void *client_data, int idx);

static sockbuf_handler_t server_handler = {
	"server",
	server_on_connect, server_on_eof, NULL,
	server_on_read, NULL,
	server_on_delete
};

/**********************************************************************
 * Connection management
 */

void connect_to_next_server() {
	server_t *serv;

	serv = server_get_next();
	if (!serv) {
		putlog(LOG_MISC, "*", _("Error connecting to next server: no servers are defined!"));
		return;
	}

	str_redup(&current_server.server_host, serv->host);
	str_redup(&current_server.server_self, serv->host);
	str_redup(&current_server.pass, serv->pass);
	current_server.strcmp = strcasecmp;
	if (!server_config.fake005) server_config.fake005 = strdup(":fakeserver 005 fakenick MODES=3 MAXCHANNELS=10 MAXBANS=100 NICKLEN=9 TOPICLEN=307 KICKLEN=307 CHANTYPES=#& PREFIX=(ov)@+ NETWORK=fakenetwork CASEMAPPING=rfc1459 CHANMODES=b,k,l,imnprst :are available on this server");

	if (serv->port) current_server.port = serv->port;
	else current_server.port = server_config.default_port;

	putlog(LOG_SERV, "*", _("Connecting to %s (%d)."), current_server.server_host, current_server.port);
	current_server.idx = egg_connect(current_server.server_host, current_server.port, server_config.connect_timeout);
	if (current_server.idx < 0) {
		putlog(LOG_SERV, "*", _("Error connecting to server."));
	}
	sockbuf_set_handler(current_server.idx, &server_handler, NULL);
}

/* Close the current server connection. */
void kill_server(const char *reason)
{
	if (reason && (current_server.idx > -1)) printserv(SERVER_NOQUEUE, "QUIT :%s\r\n", reason);
	disconnect_server();
}

/* Clean up stuff when we want to disconnect or are disconnected. */
static void disconnect_server()
{
	int connected = current_server.connected;
	int i, idx = current_server.idx;

	cycle_delay = server_config.cycle_delay;

	channel_reset();
	if (current_server.server_host) free(current_server.server_host);
	if (current_server.server_self) free(current_server.server_self);
	if (current_server.nick) free(current_server.nick);
	if (current_server.user) free(current_server.user);
	if (current_server.host) free(current_server.host);
	if (current_server.real_name) free(current_server.real_name);
	if (current_server.pass) free(current_server.pass);
	for (i = 0; i < current_server.nsupport; i++) {
		free(current_server.support[i].name);
		free(current_server.support[i].value);
	}
	if (current_server.support) free(current_server.support);
	memset(&current_server, 0, sizeof(current_server));
	current_server.idx = -1;

	if (idx != -1) sockbuf_delete(idx);
	if (connected) eggdrop_event("disconnect-server");
}


/****************************************************************
 * Sockbuf handlers
 */

/* When we get an eof or an error in the connection. */
static int server_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	putlog(LOG_SERV, "*", "Disconnected from %s", current_server.server_self);
	if (err && errmsg) putlog(LOG_SERV, "*", "Network error %d: %s", err, errmsg);
	disconnect_server();
	return(0);
}

static int server_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	/* If ip_lookup is 0, then we get our ip address from the socket we used
	 * to connect to the server. If it's 1, it's handled in got001(). */
	if (server_config.ip_lookup == 0) {
		int sock;
		char *ip;

		sock = sockbuf_get_sock(idx);
		socket_get_name(sock, &ip, NULL);
		dcc_dns_set(ip);
		free(ip);
	}

	linemode_on(current_server.idx);

	current_server.connected = 1;
	nick_list_on_connect();
	putlog(LOG_SERV, "*", _("Connected to %s, logging in."), current_server.server_host);
	eggdrop_event("connect-server");
	if (!current_server.connected) return(0);

	printserv(SERVER_NOQUEUE, "USER %s localhost %s :%s\r\n", server_config.user, current_server.server_host, server_config.realname);
	if (current_server.pass) printserv(SERVER_NOQUEUE, "PASS %s\r\n", current_server.pass);
	try_next_nick();
	return(0);
}

static int server_on_read(void *client_data, int idx, char *text, int len)
{
	if (!len) return(0);
	server_parse_input(text);
	return(0);
}

static int server_on_delete(void *client_data, int idx)
{
	return(0);
}
