/* sockbuf.c: sockbuf functions
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
static const char rcsid[] = "$Id: sockbuf.c,v 1.15 2004/06/25 17:44:04 darko Exp $";
#endif

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#ifdef HAVE_POLL
#  ifdef HAVE_SYS_POLL_H
#    include <sys/poll.h>
#  else
#    include <poll.h>
#  endif
#else
#  include "lib/compat/poll.h"
#endif

#include <errno.h>
#include <eggdrop/eggdrop.h>

typedef struct {
	int sock;	/* Underlying socket descriptor */
	int flags;	/* Keep track of blocked status, client/server */

	char *peer_ip;	/* Connection data. */
	int peer_port;
	char *my_ip;
	int my_port;

	/* Some stats. */
	sockbuf_stats_t *stats;

	char *data;	/* Output buffer. */
	int len;	/* Length of buffer. */

	sockbuf_filter_t **filters;	/* Line-mode, gzip, ssl... */
	void **filter_client_data;	/* Client data for filters */
	int nfilters;	/* Number of filters */

	sockbuf_handler_t *handler;	/* User's event handlers */
	void *client_data;	/* User's client data */
} sockbuf_t;

static sockbuf_t *sockbufs = NULL;
static int nsockbufs = 0;
static int ndeleted_sockbufs = 0;

/* 'idx_array' and 'pollfds' are parallel arrays. */
static int *idx_array = NULL;
static struct pollfd *pollfds = NULL;
static int npollfds = 0;

/* Listeners attach to the end of the pollfds array so that we can listen
	for events on sockets that don't have sockbufs. */
static int nlisteners = 0;

/* An idle event handler that does nothing. */
static sockbuf_handler_t sockbuf_idler = {
	"idle",
	NULL, NULL, NULL,
	NULL, NULL
};

static void sockbuf_got_eof(int idx, int err);

/* Stats helpers. */
static void stats_in(sockbuf_stats_t *stats, int len);
static void stats_out(sockbuf_stats_t *stats, int len);
static void skip_stats(sockbuf_stats_t *stats, int curtime);
static void update_stats(sockbuf_stats_t *stats);

int sockbuf_init(void)
{
	return (0);	
}

int sockbuf_shutdown(void)
{
	int i;

	for (i = npollfds - 1; i >= 0; i--) {
        	sockbuf_t *sbuf = &sockbufs[idx_array[i]];

		putlog(LOG_DEBUG, "*", "Socket %i %s:%i shouldn't be opened at this stage, closing.", idx_array[i],
			(sbuf->peer_ip) ? sbuf->peer_ip : sbuf->my_ip,
			(sbuf->peer_ip) ? sbuf->peer_port : sbuf->my_port);
			
		sockbuf_delete(idx_array[i]);
	}

	if (idx_array) {
		free(idx_array);
		idx_array = NULL;
	}

	if (pollfds) {
		free(pollfds);
		pollfds = NULL;
	}
	npollfds = 0;

	if (sockbufs) {
		free(sockbufs);
		sockbufs = NULL;
	}
	nsockbufs = 0;

	return (0);
}

/* Mark a sockbuf as blocked and put it on the POLLOUT list. */
static void sockbuf_block(int idx)
{
	int i;
	sockbufs[idx].flags |= SOCKBUF_BLOCK;
	for (i = 0; i < npollfds; i++) {
		if (idx_array[i] == idx) {
			pollfds[i].events |= POLLOUT;
			break;
		}
	}
}

/* Mark a sockbuf as unblocked and remove it from the POLLOUT list. */
static void sockbuf_unblock(int idx)
{
	int i;
	sockbufs[idx].flags &= (~SOCKBUF_BLOCK);
	for (i = 0; i < npollfds; i++) {
		if (idx_array[i] == idx) {
			pollfds[i].events &= (~POLLOUT);
			break;
		}
	}
}

/* Try to write data to the underlying socket. If we don't write it all,
	save the data in the output buffer and start monitoring for POLLOUT. */
static int sockbuf_real_write(int idx, const char *data, int len)
{
	int nbytes = 0;
	sockbuf_t *sbuf = &sockbufs[idx];

	/* If it's not blocked already, write as much as we can. */
	if (!(sbuf->flags & SOCKBUF_BLOCK)) {		
		nbytes = write (sbuf->sock, data, len);
		if (nbytes < 0) {
			if (errno != EAGAIN) {
				sockbuf_got_eof(idx, errno);
				return(nbytes);
			}
			nbytes = 0;
		}

		if (nbytes > 0) stats_out(sbuf->stats, nbytes);
		if (nbytes == len) return(nbytes);
		sockbuf_block(idx);
		data += nbytes;
		len -= nbytes;
	}

	/* Add the remaining data to the buffer. */
	sbuf->data = realloc(sbuf->data, sbuf->len + len);
	memcpy(sbuf->data + sbuf->len, data, len);
	sbuf->len += len;
	return(nbytes);
}

/* Eof occurs on a socket. */
int sockbuf_on_eof(int idx, int level, int err, const char *errmsg)
{
	int i;
	sockbuf_t *sbuf = &sockbufs[idx];

	for (i = 0; i < sbuf->nfilters; i++) {
		if (sbuf->filters[i]->on_eof && sbuf->filters[i]->level > level) {
			return sbuf->filters[i]->on_eof(sbuf->filter_client_data[i], idx, err, errmsg);
		}
	}

	/* If we didn't branch to a filter, try the user handler. */
	if (sbuf->handler->on_eof) {
		sbuf->handler->on_eof(sbuf->client_data, idx, err, errmsg);
	}
	return(0);
}

/* This is called when a client sock connects successfully. */
int sockbuf_on_connect(int idx, int level, const char *peer_ip, int peer_port)
{
	int i;
	sockbuf_t *sbuf = &sockbufs[idx];

	for (i = 0; i < sbuf->nfilters; i++) {
		if (sbuf->filters[i]->on_connect && sbuf->filters[i]->level > level) {
			return sbuf->filters[i]->on_connect(sbuf->filter_client_data[i], idx, peer_ip, peer_port);
		}
	}

	timer_get_now(&sbuf->stats->connected_at);
	if (peer_ip) str_redup(&sbuf->peer_ip, peer_ip);
	sbuf->peer_port = peer_port;
	if (sbuf->handler->on_connect) {
		sbuf->handler->on_connect(sbuf->client_data, idx, peer_ip, peer_port);
	}
	return(0);
}

/* When an incoming connection is accepted. */
int sockbuf_on_newclient(int idx, int level, int newidx, const char *peer_ip, int peer_port)
{
	int i;
	sockbuf_t *sbuf = &sockbufs[idx];

	for (i = 0; i < sbuf->nfilters; i++) {
		if (sbuf->filters[i]->on_connect && sbuf->filters[i]->level > level) {
			return sbuf->filters[i]->on_newclient(sbuf->filter_client_data[i], idx, newidx, peer_ip, peer_port);
		}
	}

	if (sbuf->handler->on_newclient) {
		sbuf->handler->on_newclient(sbuf->client_data, idx, newidx, peer_ip, peer_port);
	}
	return(0);
}

/* We read some data from the sock. */
int sockbuf_on_read(int idx, int level, char *data, int len)
{
	int i;
	sockbuf_t *sbuf = &sockbufs[idx];

	for (i = 0; i < sbuf->nfilters; i++) {
		if (sbuf->filters[i]->on_read && sbuf->filters[i]->level > level) {
			return sbuf->filters[i]->on_read(sbuf->filter_client_data[i], idx, data, len);
		}
	}

	sbuf->stats->bytes_in += len;
	if (sbuf->handler->on_read ){
		sbuf->handler->on_read(sbuf->client_data, idx, data, len);
	}
	return(0);
}

/* We're writing some data to the sock. */
int sockbuf_on_write(int idx, int level, const char *data, int len)
{
	int i;
	sockbuf_t *sbuf = &sockbufs[idx];

	for (i = sbuf->nfilters-1; i >= 0; i--) {
		if (sbuf->filters[i]->on_write && sbuf->filters[i]->level < level) {
			return sbuf->filters[i]->on_write(sbuf->filter_client_data[i], idx, data, len);
		}
	}
	/* There's no user handler for on_write (they wrote it). */
	return sockbuf_real_write(idx, data, len);
}

/* We wrote some data to the sock. */
int sockbuf_on_written(int idx, int level, int len, int remaining)
{
	int i;
	sockbuf_t *sbuf = &sockbufs[idx];

	for (i = 0; i < sbuf->nfilters; i++) {
		if (sbuf->filters[i]->on_written && sbuf->filters[i]->level > level) {
			return sbuf->filters[i]->on_written(sbuf->filter_client_data[i], idx, len, remaining);
		}
	}

	if (sbuf->handler->on_written) {
		sbuf->handler->on_written(sbuf->client_data, idx, len, remaining);
	}
	return(0);
}

/* When eof or an error is detected. */
static void sockbuf_got_eof(int idx, int err)
{
	char *errmsg;
	sockbuf_t *sbuf = &sockbufs[idx];

	/* If there's no error given, check for a socket-level error. */
	if (!err) err = socket_get_error(sbuf->sock);

	/* Get the associated error message. */
	errmsg = strerror(err);

	sockbuf_close(idx);
	sockbuf_on_eof(idx, SOCKBUF_LEVEL_INTERNAL, err, errmsg);
}

/* When a client sock is writable, that means it's connected. Unless there's
	a socket level error, anyway. So see if there's an error, then get
	the peer we're connected to, then call the on_connect event. */
static void sockbuf_got_writable_client(int idx)
{
	int err, peer_port;
	char *peer_ip;
	sockbuf_t *sbuf = &sockbufs[idx];

	err = socket_get_error(sbuf->sock);
	if (err) {
		sockbuf_got_eof(idx, err);
		return;
	}

	sbuf->flags &= ~SOCKBUF_CLIENT;
	if (!sbuf->len) sockbuf_unblock(idx);
	socket_get_peer_name(sbuf->sock, &peer_ip, &peer_port);

	sockbuf_on_connect(idx, SOCKBUF_LEVEL_INTERNAL, peer_ip, peer_port);
	if (peer_ip) free(peer_ip);
}

/* When a server sock is readable, that means there's a connection waiting
	to be accepted. So we'll accept the sock, get the peer name, and
	call the on_newclient event. */
static void sockbuf_got_readable_server(int idx)
{
	int newsock, newidx, peer_port;
	char *peer_ip = NULL;
	sockbuf_t *sbuf = &sockbufs[idx];

	newsock = socket_accept(sbuf->sock, &peer_ip, &peer_port);
	if (newsock < 0) {
		if (peer_ip) free(peer_ip);
		return;
	}
	socket_set_nonblock(newsock, 1);

	newidx = sockbuf_new();
	timer_get_now(&sockbufs[newidx].stats->connected_at);
	sockbuf_set_sock(newidx, newsock, 0);
	sockbuf_on_newclient(idx, SOCKBUF_LEVEL_INTERNAL, newidx, peer_ip, peer_port);
}

/* This is called when the POLLOUT condition is true for already-connected
	socks. We write as much data as we can and call the on_written
	event. */
static void sockbuf_got_writable(int idx)
{
	int nbytes;
	sockbuf_t *sbuf = &sockbufs[idx];

	/* Try to write any buffered data. */
	errno = 0;
	nbytes = write(sbuf->sock, sbuf->data, sbuf->len);
	if (nbytes > 0) {
		stats_out(sbuf->stats, nbytes);
		sbuf->len -= nbytes;
		sbuf->stats->raw_bytes_left = sbuf->len;
		if (!sbuf->len) sockbuf_unblock(idx);
		else memmove(sbuf->data, sbuf->data+nbytes, sbuf->len);
		sockbuf_on_written(idx, SOCKBUF_LEVEL_INTERNAL, nbytes, sbuf->len);
	}
	else if (nbytes < 0) {
		/* If there's an error writing to a socket that's marked as
			writable, then there's probably a socket-level error. */
		sockbuf_got_eof(idx, errno);
	}
}

/* When a sock is readable we read some from it and pass it to the on_read
	handlers. We don't want to read more than once here, because fast
	sockets on slow computers can get stuck in the read loop. */
static void sockbuf_got_readable(int idx)
{
	sockbuf_t *sbuf = &sockbufs[idx];
	char buf[4097];
	int nbytes;

	errno = 0;
	nbytes = read(sbuf->sock, buf, sizeof(buf)-1);
	if (nbytes > 0) {
		stats_in(sbuf->stats, nbytes);
		buf[nbytes] = 0;
		sockbuf_on_read(idx, SOCKBUF_LEVEL_INTERNAL, buf, nbytes);
	}
	else {
		sockbuf_got_eof(idx, errno);
	}
}

int sockbuf_new()
{
	sockbuf_t *sbuf;
	int idx;

	for (idx = 0; idx < nsockbufs; idx++) {
		if (sockbufs[idx].flags & SOCKBUF_AVAIL) break;
	}
	if (idx == nsockbufs) {
		int i;

		sockbufs = realloc(sockbufs, (nsockbufs+5) * sizeof(*sockbufs));
		memset(sockbufs+nsockbufs, 0, 5 * sizeof(*sockbufs));
		for (i = 0; i < 5; i++) {
			sockbufs[nsockbufs+i].sock = -1;
			sockbufs[nsockbufs+i].flags = SOCKBUF_AVAIL;
		}
		nsockbufs += 5;
	}

	sbuf = &sockbufs[idx];
	memset(sbuf, 0, sizeof(*sbuf));
	sbuf->flags = SOCKBUF_BLOCK;
	sbuf->sock = -1;
	sbuf->handler = &sockbuf_idler;
	sbuf->stats = calloc(1, sizeof(*sbuf->stats));

	return(idx);
}

int sockbuf_get_sock(int idx)
{
	if (!sockbuf_isvalid(idx)) return(-1);
	return(sockbufs[idx].sock);
}

int sockbuf_set_sock(int idx, int sock, int flags)
{
	int i;

	if (!sockbuf_isvalid(idx)) return(-1);

	sockbufs[idx].sock = sock;
	sockbufs[idx].flags &= ~(SOCKBUF_CLIENT|SOCKBUF_SERVER|SOCKBUF_BLOCK|SOCKBUF_NOREAD);
	sockbufs[idx].flags |= flags;

	/* pollfds   = [socks][socks][socks][listeners][listeners][end] */
	/* idx_array = [ idx ][ idx ][ idx ][end]*/
	/* So when we grow pollfds, we shift the listeners at the end. */

	/* Find the entry in the pollfds array. */
	for (i = 0; i < npollfds; i++) {
		if (idx_array[i] == idx) break;
	}

	if (sock == -1) {
		if (i == npollfds) return(1);

		/* If they set the sock to -1, then we remove the entry. */
		memmove(idx_array+i, idx_array+i+1, sizeof(int) * (npollfds-i-1));
		memmove(pollfds+i, pollfds+i+1, sizeof(*pollfds) * (nlisteners + npollfds-i-1));
		npollfds--;
		return(0);
	}

	/* Add it to the end if it's not found. */
	if (i == npollfds) {
		/* Add the new idx to the idx_array. */
		idx_array = realloc(idx_array, sizeof(int) * (i+1));
		idx_array[i] = idx;

		/* Add corresponding pollfd to pollfds. */
		pollfds = realloc(pollfds, sizeof(*pollfds) * (i+nlisteners+1));
		memmove(pollfds+i+1, pollfds+i, sizeof(*pollfds) * nlisteners);

		npollfds++;
	}

	pollfds[i].fd = sock;
	pollfds[i].events = 0;
	if (flags & (SOCKBUF_BLOCK|SOCKBUF_CLIENT)) pollfds[i].events |= POLLOUT;
	if (!(flags & SOCKBUF_NOREAD)) pollfds[i].events |= POLLIN;

	return(idx);
}

int sockbuf_get_peer(int idx, const char **peer_ip, int *peer_port)
{
	if (!sockbuf_isvalid(idx)) return(-1);
	if (peer_ip) *peer_ip = sockbufs[idx].peer_ip;
	if (peer_port) *peer_port = sockbufs[idx].peer_port;
	return(0);
}

int sockbuf_get_self(int idx, const char **my_ip, int *my_port)
{
	if (!sockbuf_isvalid(idx)) return(-1);
	if (my_ip) *my_ip = sockbufs[idx].my_ip;
	if (my_port) *my_port = sockbufs[idx].my_port;
	return(0);
}

int sockbuf_get_stats(int idx, sockbuf_stats_t **stats)
{
	if (!sockbuf_isvalid(idx)) return(-1);
	if (stats) {
		*stats = sockbufs[idx].stats;
		update_stats(*stats);
	}
	return(0);
}

int sockbuf_noread(int idx)
{
	int i;

	if (!sockbuf_isvalid(idx)) return(-1);

	/* Find the entry in the pollfds array. */
	for (i = 0; i < npollfds; i++) {
		if (idx_array[i] == idx) break;
	}

	if (i == npollfds) return(-1);

	pollfds[i].events &= (~POLLIN);
	return(0);
}

int sockbuf_read(int idx)
{
	int i;

	if (!sockbuf_isvalid(idx)) return(-1);

	/* Find the entry in the pollfds array. */
	for (i = 0; i < npollfds; i++) {
		if (idx_array[i] == idx) break;
	}

	if (i == npollfds) return(-1);

	pollfds[i].events |= POLLIN;
	return(0);
}

int sockbuf_isvalid(int idx)
{
	if (idx >= 0 && idx < nsockbufs && !(sockbufs[idx].flags & (SOCKBUF_AVAIL | SOCKBUF_DELETED))) return(1);
	return(0);
}

int sockbuf_close(int idx)
{
	sockbuf_t *sbuf;

	if (!sockbuf_isvalid(idx)) return(-1);
	sbuf = &sockbufs[idx];
	if (sbuf->sock >= 0) {
		socket_close(sbuf->sock);
		sockbuf_set_sock(idx, -1, 0);
	}
	return(0);
}

int sockbuf_delete(int idx)
{
	sockbuf_t *sbuf;
	int i;

	if (!sockbuf_isvalid(idx)) return(-1);
	sbuf = &sockbufs[idx];

	sbuf->flags |= SOCKBUF_DELETED;
	/* Call the on_delete handler for all filters. */
	for (i = 0; i < sbuf->nfilters; i++) {
		if (sbuf->filters[i]->on_delete) {
			sbuf->filters[i]->on_delete(sbuf->filter_client_data[i], idx);
		}
	}

	if (sbuf->handler->on_delete) sbuf->handler->on_delete(sbuf->client_data, idx);

	/* Close the file descriptor. */
	if (sbuf->sock >= 0) socket_close(sbuf->sock);

	/* Free the peer ip. */
	if (sbuf->peer_ip) free(sbuf->peer_ip);

	/* Free its output buffer. */
	if (sbuf->data) free(sbuf->data);

	/* Free the stats struct. */
	if (sbuf->stats) free(sbuf->stats);

	/* Free filters */
	if (sbuf->filters) free(sbuf->filters);

	/* Free filter client data */
	if (sbuf->filter_client_data) free(sbuf->filter_client_data);

	/* Mark it as deleted. */
	memset(sbuf, 0, sizeof(*sbuf));
	sbuf->sock = -1;
	sbuf->flags = SOCKBUF_DELETED;
	sbuf->handler = &sockbuf_idler;
	ndeleted_sockbufs++;

	/* Find it in the pollfds/idx_array and delete it. */
	for (i = 0; i < npollfds; i++) if (idx_array[i] == idx) break;
	if (i == npollfds) return(0);

	memmove(pollfds+i, pollfds+i+1, sizeof(*pollfds) * (npollfds+nlisteners-i-1));
	memmove(idx_array+i, idx_array+i+1, sizeof(int) * (npollfds-i-1));
	npollfds--;

	return(0);
}

int sockbuf_write(int idx, const char *data, int len)
{
	if (!sockbuf_isvalid(idx)) return(-1);
	if (len < 0) len = strlen(data);
	sockbufs[idx].stats->bytes_out += len;
	return sockbuf_on_write(idx, SOCKBUF_LEVEL_WRITE_INTERNAL, data, len);
}

int sockbuf_set_handler(int idx, sockbuf_handler_t *handler, void *client_data)
{
	if (!sockbuf_isvalid(idx)) return(-1);
	sockbufs[idx].handler = handler;
	sockbufs[idx].client_data = client_data;

	return(0);
}

/* Listeners are sockets that you want to be included in the event loop, but
	do not have sockbufs associated with them. This is useful for stuff
	like Tcl scripts who want to use async Tcl channels. All you have to
	do is attach the channel's file descriptor with this function and it
	will be monitored for activity (but not acted upon).
*/
int sockbuf_attach_listener(int fd)
{
	pollfds = realloc(pollfds, sizeof(*pollfds) * (npollfds + nlisteners + 1));
	pollfds[npollfds+nlisteners].fd = fd;
	pollfds[npollfds+nlisteners].events = POLLIN;
	pollfds[npollfds+nlisteners].revents = 0;
	nlisteners++;
	return(0);
}

int sockbuf_detach_listener(int fd)
{
	int i;

	/* Search for it so we can clear its event field. */
	for (i = 0; i < nlisteners; i++) {
		if (pollfds[npollfds+i].fd == fd) break;
	}
	if (i < nlisteners) {
		memmove(pollfds+npollfds+i, pollfds+npollfds+i+1, sizeof(*pollfds) * (nlisteners-i-1));
		nlisteners--;
	}
	return(0);
}

/* A filter is something you can write to intercept events that happen on/to
	a sockbuf. When something happens, like data arrives on the socket,
	we pass the event to the earliest filter in the chain. It chooses to
	halt the event or continue it (maybe modifying it too). Some events,
	like writing to the sockbuf (sockbuf_write) have to get called
	backwards.
*/
int sockbuf_attach_filter(int idx, sockbuf_filter_t *filter, void *client_data)
{
	sockbuf_t *sbuf;
	int i;

	if (!sockbuf_isvalid(idx)) return(-1);
	sbuf = &sockbufs[idx];

	sbuf->filters = realloc(sbuf->filters, sizeof(filter) * (sbuf->nfilters+1));

	sbuf->filter_client_data = realloc(sbuf->filter_client_data, sizeof(void *) * (sbuf->nfilters+1));

	/* Filters are ordered according to levels. The lower the level, the
		earlier the filter comes. This allows filters to be stacked
		in different orders but still function intelligently (e.g.
		compression should always be above encryption).
	*/
	for (i = 0; i < sbuf->nfilters; i++) {
		if (filter->level < sbuf->filters[i]->level) break;
	}

	/* Move up the higher-level filters. */
	memmove(sbuf->filters+i+1, sbuf->filters+i, sizeof(filter) * (sbuf->nfilters-i));
	memmove(sbuf->filter_client_data+i+1, sbuf->filter_client_data+i, sizeof(void *) * (sbuf->nfilters-i));

	/* Put this filter in the empty spot. */
	sbuf->filters[i] = filter;
	sbuf->filter_client_data[i] = client_data;

	sbuf->nfilters++;
	return(0);
}

int sockbuf_get_filter_data(int idx, sockbuf_filter_t *filter, void *client_data_ptr)
{
	int i;
	sockbuf_t *sbuf;

	if (!sockbuf_isvalid(idx)) return(-1);
	sbuf = &sockbufs[idx];
	for (i = 0; i < sbuf->nfilters; i++) {
		if (sbuf->filters[i] == filter) {
			*(void **)client_data_ptr = sbuf->filter_client_data[i];
			return(0);
		}
	}
	return(-1);
}

/* Detach the specified filter, and return the filter's client data in the
	client_data pointer (it should be a pointer to a pointer). */
int sockbuf_detach_filter(int idx, sockbuf_filter_t *filter, void *client_data_ptr)
{
	int i;
	sockbuf_t *sbuf;

	if (!sockbuf_isvalid(idx)) return(-1);
	sbuf = &sockbufs[idx];

	for (i = 0; i < sbuf->nfilters; i++) if (sbuf->filters[i] == filter) break;
	if (i == sbuf->nfilters) {
		if (client_data_ptr) client_data_ptr = NULL;
		return(0);
	}

	if (client_data_ptr) *(void **)client_data_ptr = sbuf->filter_client_data[i];
	memmove(sbuf->filter_client_data+i, sbuf->filter_client_data+i+1, sizeof(void *) * (sbuf->nfilters-i-1));
	memmove(sbuf->filters+i, sbuf->filters+i+1, sizeof(void *) * (sbuf->nfilters-i-1));
	sbuf->nfilters--;
	return(0);
}

/* This bit waits for something to happen on one of the sockets, with an
	optional timeout (pass -1 to wait forever). Then, all of the sockbufs
	are processed, callbacks made, etc, before control returns to the
	caller.
*/
int sockbuf_update_all(int timeout)
{
	int i, n, flags, revents, idx;
	static int depth = 0;

	/* Increment the depth counter when we enter the proc. */
	depth++;

	n = poll(pollfds, npollfds, timeout);
	if (n < 0) n = npollfds;

	/* If a sockbuf gets deleted during its event handler, the pollfds array
		gets shifted down and we will miss the events of the next
		socket. That's ok, because we'll pick up those events next
		time.
	*/
	for (i = 0; n && i < npollfds; i++) {
		/* Common case: no activity. */
		revents = pollfds[i].revents;
		if (!revents) continue;

		idx = idx_array[i];
		flags = sockbufs[idx].flags;
		if (revents & POLLOUT) {
			if (flags & SOCKBUF_CLIENT) sockbuf_got_writable_client(idx);
			else sockbuf_got_writable(idx);
		}
		if (revents & POLLIN) {
			if (flags & SOCKBUF_SERVER) sockbuf_got_readable_server(idx);
			else sockbuf_got_readable(idx);
		}
		if (revents & (POLLHUP|POLLNVAL|POLLERR)) sockbuf_got_eof(idx, 0);
		n--;
	}

	/* Now that we're done manipulating stuff, back out of the depth. */
	depth--;

	/* If this is the topmost level, check for deleted sockbufs. */
	if (ndeleted_sockbufs && !depth) {
		for (i = 0; ndeleted_sockbufs && i < nsockbufs; i++) {
			if (sockbufs[i].flags & SOCKBUF_DELETED) {
				sockbufs[i].flags = SOCKBUF_AVAIL;
				ndeleted_sockbufs--;
			}
		}
		/* If ndeleted_sockbufs isn't 0, then we somehow lost track of
			an idx. That can't happen, but we might as well be
			safe. */
		ndeleted_sockbufs = 0;
	}

	return(0);
}

/* Helper functions to update stats. */
static void stats_out(sockbuf_stats_t *stats, int len)
{
	timer_get_now(&stats->last_output_at);
	skip_stats(stats, stats->last_output_at.sec);
	stats->raw_bytes_out += len;
	stats->snapshot_out_bytes[stats->snapshot_counter] += len;
}

static void stats_in(sockbuf_stats_t *stats, int len)
{
	timer_get_now(&stats->last_input_at);
	skip_stats(stats, stats->last_input_at.sec);
	stats->raw_bytes_in += len;
	stats->snapshot_in_bytes[stats->snapshot_counter] += len;
}

static void skip_stats(sockbuf_stats_t *stats, int curtime)
{
	int diff, i;

	diff = curtime - stats->last_snapshot;
	stats->last_snapshot = curtime;
	if (diff > 5) diff = 5;

	/* Reset counters for seconds that were skipped. */
	for (i = 0; i < diff; i++) {
		stats->snapshot_counter++;
		if (stats->snapshot_counter >= 5) stats->snapshot_counter = 0;
		stats->snapshot_out_bytes[stats->snapshot_counter] = 0;
		stats->snapshot_in_bytes[stats->snapshot_counter] = 0;
	}
}

/* Should be called to update the cps stats. */
static void update_stats(sockbuf_stats_t *stats)
{
	int curtime = timer_get_now_sec(NULL);
	int nsecs;
	int snap_in = 0, snap_out = 0;
	int i;

	/* Compute total input cps. */
	nsecs = curtime - stats->connected_at.sec + 1;

	stats->total_in_cps = stats->raw_bytes_in / nsecs;
	stats->total_out_cps = stats->raw_bytes_out / nsecs;

	/* Compute cps. */

	/* Zero out any skipped seconds. */
	skip_stats(stats, curtime);

	/* When we calculate the totals, we do not include the current second
	 * because more data might be sent during this second and our report
	 * will be pretty inaccurate. */
	for (i = 0; i < 5; i++) {
		if (i != stats->snapshot_counter) {
			snap_in += stats->snapshot_in_bytes[i];
			snap_out += stats->snapshot_out_bytes[i];
		}
	}
	if (nsecs > 4) nsecs = 4;
	else if (nsecs > 1) nsecs--;
	else nsecs = 1;
	stats->snapshot_in_cps = snap_in / nsecs;
	stats->snapshot_out_cps = snap_out / nsecs;
}
