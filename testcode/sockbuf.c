#define DO_POLL

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#ifdef DO_POLL
	#include <sys/poll.h>
#else
	/* No poll()? Then we emulate it. */
	#define POLLIN	1
	#define POLLOUT	2
	#define POLLERR	4
	#define POLLNVAL	8
	#define POLLHUP	16
	struct pollfd {
		int fd;
		short events;
		short revents;
	};
#endif
#include <errno.h>

#include "sockbuf.h"

typedef struct sockbuf_b {
	int sock;	/* Underlying socket descriptor */
	int flags;	/* Keep track of blocked status, client/server */

	sockbuf_iobuf_t outbuf;	/* Output buffer. */

	sockbuf_event_b *filters;	/* Line-mode, gzip, ssl... */
	void **filter_client_data;	/* Client data for filters */
	int nfilters;	/* Number of filters */

	sockbuf_event_b on;	/* User's event handlers */
	void *client_data;	/* User's client data */
} sockbuf_t;

static sockbuf_t *sockbufs = NULL;
static int nsockbufs = 0;
static int ndeleted_sockbufs = 0;

/* 'idx_array' and 'pollfds' are parallel arrays. */
static int *idx_array = NULL;
static struct pollfd *pollfds = NULL;
static int npollfds = 0;
static int nlisteners = 0;

/* An idle event handler that does nothing. */
static sockbuf_event_t sockbuf_idler = {0, (Function) "idle"};

static int sockbuf_real_write(int idx, sockbuf_iobuf_t *iobuf);

/* Level is the level of the caller. For WRITE events, the callbacks are
	processed backwards, so the initial level should be sbuf->nfilters, so
	that when we -- it, it will point to the last filter. For all other
	events, level should start at -1, so that when we ++ it, it will be 0
	(first filter). */
int sockbuf_filter(int idx, int event, int level, void *arg)
{
	int found;
	int retval;
	Function callback;
	sockbuf_t *sbuf;

	if (!sockbuf_isvalid(idx)) return(-1);
	sbuf = &sockbufs[idx];

	/* Search for the first filter that handles this event. */
	/* SOCKBUF_WRITE needs to be processed backwards. */
	found = 0;
	if (event == SOCKBUF_WRITE) {
		for (level--; level >= 0; level--) {
			if ((int) (sbuf->filters[level][0]) <= event) continue;
			if ((callback = sbuf->filters[level][event+2])) {
				found = 1;
				break;
			}
		}
	}
	else {
		for (level++; level < sbuf->nfilters; level++) {
			if ((int) (sbuf->filters[level][0]) <= event) continue;
			if ((callback = sbuf->filters[level][event+2])) {
				found = 1;
				break;
			}
		}
	}

	if (found) {
		retval = callback(idx, event, level, arg, sbuf->filter_client_data[level]);
	}
	else if ((int) sbuf->on[0] > event) {
		callback = sockbufs[idx].on[event+2];
		if (callback) retval = callback(idx, arg, sbuf->client_data);
	}
	else {
		/* Here is where our default handlers go. */
		switch (event) {
			case SOCKBUF_WRITE:
				retval = sockbuf_real_write(idx, arg);
				break;
		}
	}

	return(retval);
}

int sockbuf_write_filter(int idx, int level, unsigned char *data, int len)
{
	sockbuf_iobuf_t my_iobuf;

	my_iobuf.data = data;
	my_iobuf.len = len;
	my_iobuf.max = len;
	return sockbuf_filter(idx, SOCKBUF_WRITE, level, &my_iobuf);
}

/* Mark a sockbuf as blocked and put it on the POLLOUT list. */
static int sockbuf_block(sockbuf_t *sbuf)
{
	int i;
	sbuf->flags |= SOCKBUF_BLOCK;
	for (i = 0; i < npollfds; i++) {
		if (pollfds[i].fd == sbuf->sock) {
			pollfds[i].events |= POLLOUT;
			break;
		}
	}
	return(0);
}

/* Mark a sockbuf as unblocked and call its on->empty handler. */
static int sockbuf_unblock(int idx)
{
	int i;

	sockbufs[idx].flags &= (~SOCKBUF_BLOCK);
	for (i = 0; i < npollfds; i++) {
		if (idx_array[i] == idx) {
			pollfds[i].events &= (~POLLOUT);
			break;
		}
	}

	/* If it's a client socket, this means it's connected. */
	if (sockbufs[idx].flags & SOCKBUF_CLIENT) {
		sockbufs[idx].flags &= (~SOCKBUF_CLIENT);
		sockbuf_filter(idx, SOCKBUF_CONNECT, -1, NULL);
		return(0);
	}
	/* Otherwise do the EMPTY event. */
	sockbuf_filter(idx, SOCKBUF_EMPTY, -1, NULL);
	return(0);
}

/* Eof occurs on a socket. */
static int sockbuf_eof(int idx)
{
	sockbuf_filter(idx, SOCKBUF_EOF, -1, NULL);
	return(0);
}

/* Error occurs on a socket. */
static int sockbuf_err(int idx, int err)
{
	if (!err) {
		int size = sizeof(int);
		getsockopt(sockbufs[idx].sock, SOL_PACKET, SO_ERROR, &err, &size);
		if (!err) return(0);
	}

	sockbuf_filter(idx, SOCKBUF_ERR, -1, (void *)err);
	return(err);
}

/* Add a buffer to a sockbuf's output list, copying it first. */
int sockbuf_write(int idx, unsigned char *data, int len)
{
	sockbuf_iobuf_t iobuf;

	iobuf.data = data;
	iobuf.len = len;
	iobuf.max = len;

	return sockbuf_filter(idx, SOCKBUF_WRITE, sockbufs[idx].nfilters, &iobuf);
}

static int sockbuf_real_write(int idx, sockbuf_iobuf_t *iobuf)
{
	int nbytes;
	sockbuf_t *sbuf = &sockbufs[idx];
	sockbuf_iobuf_t *outbuf;
	char *data;
	int len;

	data = iobuf->data;
	len = iobuf->len;

	/* If it's not blocked already, write as much as we can. */
	if (!(sbuf->flags & SOCKBUF_BLOCK)) {
		int nbytes = write(sbuf->sock, data, len);
		if (nbytes == len) return(nbytes);
		if (nbytes < 0) {
			if (errno != EAGAIN) {
				sockbuf_err(idx, errno);
				return(nbytes);
			}
			nbytes = 0;
		}
		sockbuf_block(sbuf);
		data += nbytes;
		len -= nbytes;
	}

	/* Check if we need to grow the buffer. */
	outbuf = &sbuf->outbuf;
	if (outbuf->max < (outbuf->len + len)) {
		outbuf->max = outbuf->len + len + 512;
		outbuf->data = realloc(outbuf->data, outbuf->max);
	}

	memcpy(outbuf->data+outbuf->len, data, len);
	outbuf->len += len;
	return(nbytes);
}

/* Write as much data as we can. */
static int sockbuf_on_writable(int idx)
{
	int nbytes;
	sockbuf_t *sbuf = &sockbufs[idx];

	/* If it's a connecting socket, this means it's connected. */
	if (sbuf->flags & SOCKBUF_CLIENT) {
		if (sockbuf_err(idx, 0)) return(0);
		sockbuf_unblock(idx);
		/* If this sockbuf gets deleted in the callback, we're done. */
		if (sbuf->sock == -1) return(0);
	}

	/* Ok, try to write all the data. */
	nbytes = write(sbuf->sock, sbuf->outbuf.data, sbuf->outbuf.len);
	if (nbytes > 0) {
		sbuf->outbuf.len -= nbytes;
		if (!sbuf->outbuf.len) sockbuf_unblock(idx);
		else memmove(sbuf->outbuf.data, sbuf->outbuf.data+nbytes, sbuf->outbuf.len);
	}
	else if (nbytes < 0) sockbuf_err(idx, errno);
	return(nbytes);
}

static int sockbuf_on_readable(int idx)
{
	sockbuf_t *sbuf = &sockbufs[idx];
	sockbuf_iobuf_t iobuf;
	char buf[4096];
	int nbytes;

	/* If it's a server socket, this means there is a connection waiting. */
	if (sbuf->flags & SOCKBUF_SERVER) {
		sockbuf_filter(idx, SOCKBUF_READ, -1, (void *)sbuf->sock);
		return(0);
	}

	nbytes = read(sbuf->sock, buf, sizeof(buf));
	if (nbytes > 0) {
		iobuf.data = buf;
		iobuf.len = nbytes;
		iobuf.max = sizeof(buf);
		sockbuf_filter(idx, SOCKBUF_READ, -1, &iobuf);
	}
	else if (nbytes < 0) {
		sockbuf_err(idx, errno);
	}
	else {
		sockbuf_eof(idx);
	}

	return(0);
}

int sockbuf_new(int sock, int flags)
{
	sockbuf_t *sbuf;
	int idx;

	for (idx = 0; idx < nsockbufs; idx++) {
		if (sockbufs[idx].flags & SOCKBUF_AVAIL) break;
	}
	if (idx == nsockbufs) {
		int i;

		sockbufs = (sockbuf_t *)realloc(sockbufs, (nsockbufs+5) * sizeof(*sockbufs));
		memset(sockbufs+nsockbufs, 0, 5 * sizeof(*sockbufs));
		for (i = 0; i < 5; i++) {
			sockbufs[nsockbufs+i].sock = -1;
			sockbufs[nsockbufs+i].flags = SOCKBUF_AVAIL;
		}
		nsockbufs += 5;
	}

	sbuf = &sockbufs[idx];
	memset(sbuf, 0, sizeof(*sbuf));
	sbuf->flags = (flags & ~(SOCKBUF_AVAIL|SOCKBUF_DELETED));
	sbuf->sock = -1;
	sbuf->on = sockbuf_idler;

	if (sock >= 0) sockbuf_set_sock(idx, sock, flags);

	return(idx);
}

int sockbuf_set_sock(int idx, int sock, int flags)
{
	int i;

	if (!sockbuf_isvalid(idx)) return(-1);

	sockbufs[idx].sock = sock;
	sockbufs[idx].flags = flags;

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
		idx_array = (int *)realloc(idx_array, sizeof(int) * (i+1));
		idx_array[i] = idx;

		/* Add corresponding pollfd to pollfds. */
		pollfds = (struct pollfd *)realloc(pollfds, sizeof(*pollfds) * (i+nlisteners+1));
		memmove(pollfds+i+1, pollfds+i, sizeof(*pollfds) * nlisteners);

		npollfds++;
	}

	pollfds[i].fd = sock;
	if (flags & (SOCKBUF_NOREAD)) pollfds[i].events = 0;
	else pollfds[i].events = POLLIN;
	if (flags & (SOCKBUF_BLOCK | SOCKBUF_CLIENT)) pollfds[i].events |= POLLOUT;

	return(idx);
}

int sockbuf_isvalid(int idx)
{
	if (idx >= 0 && idx < nsockbufs && !(sockbufs[idx].flags & (SOCKBUF_AVAIL | SOCKBUF_DELETED))) return(1);
	return(0);
}

int sockbuf_delete(int idx)
{
	sockbuf_t *sbuf;
	int i;

	if (!sockbuf_isvalid(idx)) return(-1);
	sbuf = &sockbufs[idx];

	/* Close the file descriptor. */
	if (sbuf->sock >= 0) close(sbuf->sock);

	/* Free its output buffer. */
	if (sbuf->outbuf.data) free(sbuf->outbuf.data);

	/* Mark it as deleted. */
	memset(sbuf, 0, sizeof(*sbuf));
	sbuf->sock = -1;
	sbuf->flags = SOCKBUF_DELETED;
	ndeleted_sockbufs++;

	/* Find it in the pollfds/idx_array and delete it. */
	for (i = 0; i < npollfds; i++) if (idx_array[i] == idx) break;
	if (i == npollfds) return(0);

	memmove(pollfds+i, pollfds+i+1, sizeof(*pollfds) * (npollfds+nlisteners-i-1));
	memmove(idx_array+i, idx_array+i+1, sizeof(int) * (npollfds-i-1));
	npollfds--;

	return(0);
}

int sockbuf_set_handler(int idx, sockbuf_event_t handler, void *client_data)
{
	if (!sockbuf_isvalid(idx)) return(-1);
	sockbufs[idx].on = handler;
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
	pollfds = (struct pollfd *)realloc(pollfds, sizeof(*pollfds) * (npollfds + nlisteners + 1));
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
int sockbuf_attach_filter(int idx, sockbuf_event_t filter, void *client_data)
{
	sockbuf_t *sbuf;

	if (!sockbuf_isvalid(idx)) return(-1);
	sbuf = &sockbufs[idx];

	sbuf->filters = (sockbuf_event_b *)realloc(sbuf->filters, sizeof(*sbuf->filters) * (sbuf->nfilters+1));
	sbuf->filters[sbuf->nfilters] = filter;

	sbuf->filter_client_data = (void **)realloc(sbuf->filter_client_data, sizeof(void *) * (sbuf->nfilters+1));
	sbuf->filter_client_data[sbuf->nfilters] = client_data;

	sbuf->nfilters++;
	return(sbuf->nfilters);
}

int sockbuf_detach_filter(int idx, sockbuf_event_t filter, void *client_data)
{
	int i;
	sockbuf_t *sbuf;

	if (!sockbuf_isvalid(idx)) return(-1);
	sbuf = &sockbufs[idx];

	for (i = 0; i < sbuf->nfilters; i++) if (sbuf->filters[i] == filter) break;
	if (i == sbuf->nfilters) {
		if (client_data) *(void **)client_data = NULL;
		return(0);
	}

	if (client_data) *(void **)client_data = sbuf->filter_client_data[i];
	memmove(sbuf->filter_client_data+i, sbuf->filter_client_data+i+1, sizeof(void *) * sbuf->nfilters-i-1);
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
	int i, n, revents, idx;
	static int depth = 0;

	/* Increment the depth counter when we enter the proc. */
	depth++;

#ifdef DO_POLL
	n = poll(pollfds, npollfds, timeout);
	if (n < 0) n = 0;
#else
	fd_set reads, writes, excepts;
	int highest = -1;
	int events;

	FD_ZERO(&reads); FD_ZERO(&writes); FD_ZERO(&excepts);
	for (i = 0; i < npollfds; i++) {
		if (pollfds[i].fd == -1) continue;
		events = pollfds[i].events;
		n = pollfds[i].fd;
		if (events & POLLIN) FD_SET(n, &reads);
		if (events & POLLOUT) FD_SET(n, &writes);
		FD_SET(n, &excepts);
		if (n > highest) highest = n;
	}
	if (timeout > -1) {
		struct timeval tv;

		tv.tv_sec = (timeout / 1000);
		timeout -= 1000 * tv.tv_sec;
		tv.tv_usec = timeout*1000;
		events = select(highest+1, &reads, &writes, &excepts, &tv);
	}
	else events = select(highest+1, &reads, &writes, &excepts, NULL);
	if (events < 0) {
		char temp[1];
		/* There is a bad file descriptor among us... find it! */
		events = 0;
		for (i = 0; i < npollfds; i++) {
			errno = 0;
			n = write(pollfds[i].fd, temp, 0);
			if (n < 0 && errno != EINVAL) {
				pollfds[i].revents = POLLNVAL;
				events++;
			}
			else pollfds[i].revents = 0;
		}
	}
	else for (i = 0; i < npollfds; i++) {
		n = pollfds[i].fd;
		pollfds[i].revents = 0;
		if (FD_ISSET(n, &reads)) pollfds[i].revents |= POLLIN;
		if (FD_ISSET(n, &writes)) pollfds[i].revents |= POLLOUT;
		if (FD_ISSET(n, &excepts)) pollfds[i].revents |= POLLERR;
	}
	n = events;
#endif

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
		if (revents & POLLIN) sockbuf_on_readable(idx);
		if (revents & POLLOUT) sockbuf_on_writable(idx);
		if (revents & POLLHUP) sockbuf_eof(idx);
		if (revents & (POLLERR|POLLNVAL)) {
			/* Try socket-level error first, then generic error. */
			if (!sockbuf_err(idx, 0)) sockbuf_err(idx, -1);
		}
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
