/* dcc.c: dcc support
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
static const char rcsid[] = "$Id: dcc.c,v 1.20 2004/06/07 23:14:48 stdarg Exp $";
#endif

#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "server.h"

typedef struct dcc_listen {
	int serv, client;
	int timer_id, timeout;
	int port;
} dcc_listen_t;

typedef struct dcc_send {
	struct dcc_send *next;
	/* Connection information. */
	int serv, idx, port;
	char *nick, *ip;

	/* File information. */
	char *fname;
	int fd, size;

	/* Timer information (for gets). */
	int timer_id;

	/* A bunch of stats. */
	int bytes_sent, bytes_left;
	int snapshot_bytes[5];
	int snapshot_counter, last_snapshot;
	int acks, bytes_acked;
	int resumed_at;
	int request_time, connect_time;
} dcc_send_t;

static dcc_send_t *dcc_send_head = NULL;
static dcc_send_t *dcc_recv_head = NULL;

static int dcc_listen_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port);
static int dcc_listen_delete(void *client_data, int idx);
static int dcc_listen_timeout(void *client_data);
static int dcc_chat_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int dcc_chat_eof(void *client_data, int idx, int err, const char *errmsg);
static int dcc_chat_delete(void *client_data, int idx);
static int dcc_send_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int dcc_send_read(void *client_data, int idx, char *data, int len);
static int dcc_send_written(void *client_data, int idx, int len, int remaining);
static int dcc_send_eof(void *client_data, int idx, int err, const char *errmsg);
static int dcc_send_delete(void *client_data, int idx);
static int dcc_send_done(dcc_send_t *send);
static int dcc_recv_timeout(void *client_data);
static int dcc_recv_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int dcc_recv_read(void *client_data, int idx, char *data, int len);
static int dcc_recv_eof(void *client_data, int idx, int err, const char *errmsg);
static int dcc_recv_delete(void *client_data, int idx);
static void update_snapshot(dcc_send_t *send, int len);

static sockbuf_handler_t dcc_listen_handler = {
	"DCC listen",
	NULL, NULL, dcc_listen_newclient,
	NULL, NULL,
	dcc_listen_delete
};

#define DCC_FILTER_LEVEL	(SOCKBUF_LEVEL_TEXT_BUFFER+100)

static sockbuf_filter_t dcc_chat_filter = {
	"DCC chat", DCC_FILTER_LEVEL,
	dcc_chat_connect, dcc_chat_eof, NULL,
	NULL, NULL, NULL,
	NULL, dcc_chat_delete
};

static sockbuf_filter_t dcc_send_filter = {
	"DCC send", DCC_FILTER_LEVEL,
	dcc_send_connect, dcc_send_eof, NULL,
	dcc_send_read, NULL, dcc_send_written,
	NULL, dcc_send_delete
};

static sockbuf_filter_t dcc_recv_filter = {
	"DCC get", DCC_FILTER_LEVEL,
	dcc_recv_connect, dcc_recv_eof, NULL,
	dcc_recv_read, NULL, NULL,
	NULL, dcc_recv_delete
};

static int dcc_dns_callback(void *client_data, const char *host, char **ips)
{
	if (ips && ips[0]) str_redup(&current_server.myip, ips[0]);
	else str_redup(&current_server.myip, "127.0.0.1");
	socket_ip_to_uint(current_server.myip, &current_server.mylongip);
	return(0);
}

int dcc_dns_set(const char *host)
{
	egg_dns_lookup(host, 0, dcc_dns_callback, NULL);
	return(0);
}

static dcc_listen_t *dcc_listen(int timeout)
{
	dcc_listen_t *listen;
	int idx, port;
	egg_timeval_t howlong;

	idx = egg_listen(0, &port);
	if (idx < 0) return(NULL);

	listen = calloc(1, sizeof(*listen));
	listen->serv = idx;
	listen->port = port;

	sockbuf_set_handler(idx, &dcc_listen_handler, listen);

	/* See if they want the default timeout. */
	if (!timeout) timeout = server_config.dcc_timeout;

	if (timeout > 0) {
		char buf[128];

		snprintf(buf, sizeof(buf), "dcc listen port %d", port);
		howlong.sec = timeout;
		howlong.usec = 0;
		listen->timer_id = timer_create_complex(&howlong, buf, dcc_listen_timeout, listen, 0);
	}

	return(listen);
}

/* Starts a chat with the specified nick.
 * If timeout is 0, the default timeout is used.
 * If timeout is -1, there is no timeout.
 * We don't make sure the nick is valid, so if it's not, you won't know until
 * the chat times out. */
int dcc_start_chat(const char *nick, int timeout)
{
	dcc_listen_t *listen;
	int idx;

	/* Create a listening idx to accept the chat connection. */
	listen = dcc_listen(timeout);
	if (!listen) return(-1);

	/* Create an empty sockbuf that will eventually be used for the
	 * dcc chat (when they connect to us). */
	idx = sockbuf_new();
	sockbuf_attach_filter(idx, &dcc_chat_filter, (void *)listen->serv);
	listen->client = idx;

	printserv(SERVER_NORMAL, "PRIVMSG %s :%cDCC CHAT chat %u %d%c", nick, 1, current_server.mylongip, listen->port, 1);
	return(idx);
}

static int dcc_listen_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port)
{
	int sock;
	dcc_listen_t *chat = client_data;

	sock = sockbuf_get_sock(newidx);
	sockbuf_set_sock(newidx, -1, 0);
	sockbuf_delete(newidx);

	/* Give the waiting sockbuf the new socket and mark it as a client
	 * so that it fires the on_connect event. */
	sockbuf_set_sock(chat->client, sock, SOCKBUF_CLIENT);

	/* Delete the listening idx since it's not needed. */
	chat->client = -1;
	sockbuf_delete(idx);
	return(0);
}

static int dcc_listen_delete(void *client_data, int idx)
{
	dcc_listen_t *listen = client_data;

	/* If the timer is still going, kill it. */
	if (listen->timer_id != -1) {
		timer_destroy(listen->timer_id);
		listen->timer_id = -1;
	}

	/* If we're still waiting for the connection to come in, then send
	 * an error to the associated client sock. */
	if (sockbuf_isvalid(listen->client)) {
		if (listen->timeout) {
			sockbuf_on_eof(listen->client, SOCKBUF_LEVEL_INTERNAL, -1, "DCC timed out");
		}
		else {
			sockbuf_on_eof(listen->client, SOCKBUF_LEVEL_INTERNAL, -2, "DCC listening socket unexpectedly closed");
		}
	}
	free(listen);
	return(0);
}

static int dcc_listen_timeout(void *client_data)
{
	dcc_listen_t *listen = client_data;

	listen->timeout = 1;
	listen->timer_id = -1;
	sockbuf_delete(listen->serv);
	return(0);
}

static int dcc_chat_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	sockbuf_detach_filter(idx, &dcc_chat_filter, NULL);
	return sockbuf_on_connect(idx, DCC_FILTER_LEVEL, peer_ip, peer_port);
}

static int dcc_chat_eof(void *client_data, int idx, int err, const char *errmsg)
{
	sockbuf_detach_filter(idx, &dcc_chat_filter, NULL);
	return sockbuf_on_eof(idx, DCC_FILTER_LEVEL, err, errmsg);
}

static int dcc_chat_delete(void *client_data, int idx)
{
	int serv = (int) client_data;
	sockbuf_delete(serv);
	return(0);
}

int dcc_start_send(const char *nick, const char *fname, int timeout)
{
	dcc_listen_t *listen;
	dcc_send_t *send;
	int idx, size, fd;
	char *quote, *slash;

	fd = open(fname, O_RDONLY);
	if (!fd) return(-1);
	size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	listen = dcc_listen(timeout);
	if (!listen) {
		close(fd);
		return(-1);
	}

	/* Create an empty sockbuf that will eventually be used for the
	 * dcc chat (when they connect to us). */
	idx = sockbuf_new();
	send = calloc(1, sizeof(*send));
	sockbuf_attach_filter(idx, &dcc_send_filter, send);

	listen->client = idx;
	send->next = dcc_send_head;
	dcc_send_head = send;
	send->serv = listen->serv;
	send->port = listen->port;
	send->fd = fd;
	send->idx = idx;
	send->size = size;
	send->bytes_sent = 0;
	send->bytes_left = size;
	timer_get_now_sec(&send->request_time);
	send->fname = strdup(fname);
	send->nick = strdup(nick);

	slash = strrchr(fname, '/');
	if (slash) fname = slash+1;

	if (strchr(fname, ' ')) quote = "\"";
	else quote = "";

	printserv(SERVER_NORMAL, "PRIVMSG %s :%cDCC SEND %s%s%s %u %d %d%c", nick, 1, quote, fname, quote, current_server.mylongip, listen->port, size, 1);
	return(idx);
}

static int dcc_send_bytes(dcc_send_t *send)
{
	int n, out, total;
	char buf[4096];

	total = 0;
	for (;;) {
		/* Read in a chunk from the file. If we get eof or error,
		 * return -1. */
		n = read(send->fd, buf, sizeof(buf));
		if (n <= 0) return(-1);

		/* Now write the chunk to the sockbuf. If we get an error
		 * return -2, and if it starts blocking return 0. Otherwise
		 * we loop to send another chunk. */
		out = sockbuf_write(send->idx, buf, n);
		if (out < 0) return(-2);
		send->bytes_sent += out;
		send->bytes_left -= out;
		update_snapshot(send, out);
		total += out;
		if (out < n) {
			sockbuf_on_written(send->idx, DCC_FILTER_LEVEL, total, n - out);
			if (sockbuf_isvalid(send->idx)) return(0);
			else return(-2);
		}
	}
}

static int dcc_send_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	dcc_send_t *send = client_data;
	int n;

	timer_get_now_sec(&send->connect_time);
	send->serv = -1;
	send->ip = strdup(peer_ip);
	sockbuf_on_connect(idx, DCC_FILTER_LEVEL, peer_ip, peer_port);
	if (sockbuf_isvalid(idx)) {
		n = dcc_send_bytes(send);
		if (n == -1) dcc_send_done(send);
	}
	return(0);
}

static int dcc_send_read(void *client_data, int idx, char *data, int len)
{
	dcc_send_t *send = client_data;
	int ack;

	memcpy(&ack, data, sizeof(int));
	ack = ntohl(ack);
	send->acks++;
	send->bytes_acked = ack;
	return(0);
}

static int dcc_send_written(void *client_data, int idx, int len, int remaining)
{
	dcc_send_t *send = client_data;
	int n;

	/* Update stats on the send. */
	send->bytes_sent += len;
	send->bytes_left -= len;
	update_snapshot(send, len);

	/* Pass the event up to whoever's listening. */

	sockbuf_on_written(idx, DCC_FILTER_LEVEL, len, remaining);
	if (!sockbuf_isvalid(idx)) return(0);

	/* Send as much data as we can until we block. dcc_send_bytes()
	 * returns -1 when the file has eof(). */
	n = dcc_send_bytes(send);
	if (n == -1 && send->bytes_left <= 0 && send->bytes_acked >= send->bytes_sent) {
		dcc_send_done(send);
	}

	return(0);
}

static int dcc_send_eof(void *client_data, int idx, int err, const char *errmsg)
{
	sockbuf_on_eof(idx, DCC_FILTER_LEVEL, err, errmsg);
	sockbuf_delete(idx);
	return(0);
}

static int dcc_send_delete(void *client_data, int idx)
{
	dcc_send_t *send = client_data;
	dcc_send_t *prev, *ptr;

	if (sockbuf_isvalid(send->serv)) {
		sockbuf_delete(send->serv);
	}
	if (send->fd != -1) close(send->fd);
	if (send->fname) free(send->fname);
	if (send->nick) free(send->nick);
	if (send->ip) free(send->ip);
	prev = NULL;
	for (ptr = dcc_send_head; ptr; ptr = ptr->next) {
		if (ptr == send) break;
		prev = ptr;
	}
	if (prev) prev->next = send->next;
	else dcc_send_head = send->next;
	free(send);
	return(0);
}

static int dcc_send_done(dcc_send_t *send)
{
	int idx;

	idx = send->idx;
	close(send->fd);
	send->fd = -1;
	sockbuf_on_eof(idx, DCC_FILTER_LEVEL, 0, "Success");

	/* If their handler didn't close the idx, do it here. */
	if (sockbuf_isvalid(idx)) sockbuf_delete(idx);
	return(0);
}

int dcc_send_info(int idx, int field, void *valueptr)
{
	dcc_send_t *send;
	int i, n, now;

	if (!valueptr) return(-1);
	if (sockbuf_get_filter_data(idx, &dcc_send_filter, &send) < 0) {
		if (sockbuf_get_filter_data(idx, &dcc_recv_filter, &send) < 0) return(-1);
	}

	timer_get_now_sec(&now);
	switch (field) {
		case DCC_SEND_SENT:
			*(int *)valueptr = send->bytes_sent;
			break;
		case DCC_SEND_LEFT:
			*(int *)valueptr = send->bytes_left;
			break;
		case DCC_SEND_CPS_TOTAL:
			if (!send->connect_time) n = 0;
			else if (send->connect_time >= now) n = send->bytes_sent;
			else n = (int) ((float) send->bytes_sent / ((float) now - (float) send->connect_time));

			*(int *)valueptr = n;
			break;
		case DCC_SEND_CPS_SNAPSHOT:
			update_snapshot(send, 0);
			/* When we calculate the total, we do not include
			 * the current second (snapshot_counter) because more
			 * data might be sent this second and our report won't
			 * be accurate. */
			n = 0;
			for (i = 0; i < 5; i++) {
				if (i != send->snapshot_counter) n += send->snapshot_bytes[i];
			}
			if (now - send->connect_time >= 5) n = (int) ((float) n / 4.0);
			else if (now - send->connect_time > 1) n = (int) ((float) n / (float) (now - send->connect_time - 1));
			else n = 0;
			*(int *)valueptr = n;
			break;
		case DCC_SEND_ACKS:
			*(int *)valueptr = send->acks;
			break;
		case DCC_SEND_BYTES_ACKED:
			*(int *)valueptr = send->bytes_acked;
			break;
		case DCC_SEND_REQUEST_TIME:
			*(int *)valueptr = send->request_time;
			break;
		case DCC_SEND_CONNECT_TIME:
			*(int *)valueptr = send->connect_time;
			break;
		default:
			return(-1);
	}
	return(0);
}

int dcc_accept_send(char *nick, char *localfname, char *fname, int size, int resume, char *ip, int port, int timeout)
{
	dcc_send_t *send;
	char *quote;
	egg_timeval_t howlong;

	send = calloc(1, sizeof(*send));
	send->next = dcc_recv_head;
	dcc_recv_head = send;

	/* See if they want the default timeout. */
	if (!timeout) timeout = server_config.dcc_timeout;
	if (timeout > 0) {
		char buf[128];

		snprintf(buf, sizeof(buf), "dcc recv %s/%d", ip, port);
		howlong.sec = timeout;
		howlong.usec = 0;
		send->timer_id = timer_create_complex(&howlong, buf, dcc_recv_timeout, send, 0);
	}
	else send->connect_time = -1;

	send->port = port;
	send->fname = strdup(localfname);
	send->nick = strdup(nick);
	send->ip = strdup(ip);
	send->size = size;
	send->bytes_sent = 0;
	send->bytes_left = size;
	timer_get_now_sec(&send->request_time);

	if (resume < 0) {
		send->fd = open(localfname, O_RDWR | O_CREAT, 0640);
		resume = lseek(send->fd, 0, SEEK_END);
		close(send->fd);
		if (resume < 0) resume = 0;
	}

	if (resume >= size) resume = 0;

	if (resume) {
		putlog(LOG_MISC, "*", "sending resume request");
		send->idx = sockbuf_new();
		send->fd = open(localfname, O_RDWR | O_CREAT, 0640);
		if (resume < 0) resume = lseek(send->fd, 0, SEEK_END);
		else resume = lseek(send->fd, resume, SEEK_SET);
		if (strchr(fname, ' ')) quote = "\"";
		else quote = "";
		printserv(SERVER_NORMAL, "PRIVMSG %s :%cDCC RESUME %s%s%s %d %d%c", nick, 1, quote, fname, quote, port, resume, 1);
	}
	else {
		send->idx = egg_connect(ip, port, -1);
		send->fd = open(localfname, O_RDWR | O_CREAT | O_TRUNC, 0640);
	}

	sockbuf_attach_filter(send->idx, &dcc_recv_filter, send);

	return(send->idx);
}

static int dcc_recv_timeout(void *client_data)
{
	dcc_send_t *send = client_data;

	send->timer_id = -1;
	sockbuf_on_eof(send->idx, SOCKBUF_LEVEL_INTERNAL, -1, "DCC timed out");
	return(0);
}

static int dcc_recv_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	dcc_send_t *send = client_data;

	/* send->connect_time holds our connect timer if there was one */
	if (send->timer_id >= 0) timer_destroy(send->timer_id);
	timer_get_now_sec(&send->connect_time);
	sockbuf_on_connect(idx, DCC_FILTER_LEVEL, peer_ip, peer_port);
	return(0);
}

static int dcc_recv_read(void *client_data, int idx, char *data, int len)
{
	dcc_send_t *send = client_data;
	int nlen; /* len in network byte order */

	send->bytes_sent += len;
	send->bytes_left -= len;
	update_snapshot(send, len);
	write(send->fd, data, len);
	nlen = htonl(send->bytes_sent);
	sockbuf_on_write(idx, DCC_FILTER_LEVEL, (char *)&nlen, 4);
	return sockbuf_on_read(idx, DCC_FILTER_LEVEL, data, len);
}

static int dcc_recv_eof(void *client_data, int idx, int err, const char *errmsg)
{
	sockbuf_on_eof(idx, DCC_FILTER_LEVEL, err, errmsg);
	sockbuf_delete(idx);
	return(0);
}

static int dcc_recv_delete(void *client_data, int idx)
{
	dcc_send_t *send = client_data;
	dcc_send_t *prev, *ptr;

	if (send->timer_id != -1) timer_destroy(send->timer_id);

	if (send->fd != -1) close(send->fd);
	if (send->fname) free(send->fname);
	if (send->nick) free(send->nick);
	if (send->ip) free(send->ip);
	prev = NULL;
	for (ptr = dcc_recv_head; ptr; ptr = ptr->next) {
		if (ptr == send) break;
		prev = ptr;
	}
	if (prev) prev->next = send->next;
	else dcc_recv_head = send->next;
	free(send);
	return(0);
}

static void update_snapshot(dcc_send_t *send, int len)
{
	int diff;
	int i;

	/* Get time diff. */
	diff = timer_get_now_sec(NULL) - send->last_snapshot;
	timer_get_now_sec(&send->last_snapshot);
	if (diff > 5) diff = 5;

	/* Reset counter for seconds that were skipped. */
	for (i = 0; i < diff; i++) {
		send->snapshot_counter++;
		if (send->snapshot_counter >= 5) send->snapshot_counter = 0;
		send->snapshot_bytes[send->snapshot_counter] = 0;
	}

	/* Add the bytes to the current second. */
	send->snapshot_bytes[send->snapshot_counter] += len;
}

static void got_chat(char *nick, char *uhost, user_t *u, char *text)
{
	char type[256], ip[256], port[32];
	int nport;

	type[0] = ip[0] = port[0] = 0;
	sscanf(text, "%255[^ ] %255[^ ] %31[^ ]", type, ip, port);
	type[255] = ip[255] = port[31] = 0;

	/* Check if the ip is 'new-style' (dotted decimal or ipv6). If not,
	 * it's the old-style long ip. */
	if (!strchr(ip, '.') && !strchr(ip, ':')) {
		unsigned int longip;
		longip = (unsigned int) atol(ip);
		sprintf(ip, "%d.%d.%d.%d", (longip >> 24) & 255, (longip >> 16) & 255, (longip >> 8) & 255, (longip) & 255);
	}

	nport = atoi(port);

	bind_check(BT_dcc_chat, u ? &u->settings[0].flags : NULL, nick, nick, uhost, u, type, ip, nport);
}

static void got_resume(char *nick, char *uhost, user_t *u, char *text)
{
	int port, pos, n;
	dcc_send_t *send;
	char *fname, *space;

	fname = text;
	space = strrchr(text, ' ');
	if (space && space != text) pos = atoi(space+1);
	else return;
	*space = 0;
	space--;
	space = strrchr(text, ' ');
	if (space && space != text) port = atoi(space+1);
	else return;
	*space = 0;

	for (send = dcc_send_head; send; send = send->next) {
		if (send->port == port && !strcasecmp(send->nick, nick)) {
			n = lseek(send->fd, pos, SEEK_SET);
			send->bytes_left -= n;
			send->resumed_at = n;
			printserv(SERVER_NORMAL, "PRIVMSG %s :%cDCC ACCEPT %s %d %d%c", nick, 1, fname, port, n, 1);
			break;
		}
	}
}

static void got_accept(char *nick, char *uhost, user_t *u, char *text)
{
	int port, pos, n;
	dcc_send_t *send;
	char *fname, *space;

	fname = text;
	space = strrchr(text, ' ');
	if (space && space != text) pos = atoi(space+1);
	else return;
	*space = 0;
	space--;
	space = strrchr(text, ' ');
	if (space && space != text) port = atoi(space+1);
	else return;
	*space = 0;

	for (send = dcc_recv_head; send; send = send->next) {
		if (send->port == port && !strcasecmp(send->nick, nick)) {
			n = lseek(send->fd, pos, SEEK_SET);
			send->bytes_left -= n;
			send->resumed_at = n;
			egg_reconnect(send->idx, send->ip, port, -1);
			break;
		}
	}
}

static void got_send(char *nick, char *uhost, user_t *u, char *text)
{
	char *space, *fname, ip[256];
	int port, size, n;

	fname = text;
	space = strrchr(text, ' ');
	if (!space || space == text) return;
	size = atoi(space+1);
	*space-- = 0;
	space = strrchr(text, ' ');
	if (!space || space == text) return;
	port = atoi(space+1);
	*space-- = 0;
	space = strchr(text, ' ');
	if (!space || space == text) return;
	strlcpy(ip, space + 1, sizeof ip);
	*space = 0;

	if (*fname == '"') {
		fname++;
		n = strlen(fname);
		if (n && fname[n-1] == '"') fname[n-1] = 0;
		else return;
	}
	/* Check if the ip is new-style (dotted decimal or ipv6). If not,
	 * it's the old-style long ip. */
	if (!strchr(ip, '.') && !strchr(ip, ':')) {
		unsigned int longip;
		longip = (unsigned int) atol(ip);
		sprintf(ip, "%d.%d.%d.%d", (longip >> 24) & 255, (longip >> 16) & 255, (longip >> 8) & 255, (longip) & 255);
	}

	bind_check(BT_dcc_recv, u ? &u->settings[0].flags : NULL, nick, nick, uhost, u, fname, ip, port, size);
}

/* PRIVMSG ((target) :^ADCC CHAT ((type) ((longip) ((port)^A */
static int got_dcc(char *nick, char *uhost, user_t *u, char *dest, char *cmd, char *text)
{
	if (!strncasecmp(text, "chat ", 5)) {
		got_chat(nick, uhost, u, text+5);
	}
	else if (!strncasecmp(text, "send ", 5)) {
		got_send(nick, uhost, u, text+5);
	}
	else if (!strncasecmp(text, "resume ", 7)) {
		got_resume(nick, uhost, u, text+7);
	}
	else if (!strncasecmp(text, "accept ", 7)) {
		got_accept(nick, uhost, u, text+7);
	}
	return(0);
}

bind_list_t ctcp_dcc_binds[] = {
	{NULL, "DCC", (Function) got_dcc},
	{0}
};
