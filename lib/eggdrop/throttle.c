/* throttle.c: socket throttling
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
static const char rcsid[] = "$Id: throttle.c,v 1.3 2003/12/17 07:39:14 wcc Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <eggdrop/eggdrop.h>
#include "sockbuf.h"
#include "eggtimer.h"

#define THROTTLE_LEVEL SOCKBUF_LEVEL_THROTTLE

typedef struct throttle {
	struct throttle *next, *prev;
	char *buf;	/* buffered output */
	int len;	/* buffer length */
	int idx;	/* idx we're throttling */
	int bytes_in, bytes_out;	/* i/o this second */
	int speed_in, speed_out;	/* throttle limits */
} throttle_t;

static throttle_t *throttle_head = NULL;
static throttle_t *throttle_next = NULL;
static int throttle_timer = -1;

static void throttle_remove(throttle_t *t);

static int throttle_on_read(void *client_data, int idx, char *data, int len)
{
	throttle_t *t = client_data;

	t->bytes_in += len;
	if (t->speed_in > 0 && t->bytes_in > t->speed_in) {
		sockbuf_noread(idx);
	}
	return sockbuf_on_read(idx, THROTTLE_LEVEL, data, len);
}

static int throttle_on_write(void *client_data, int idx, const char *data, int len)
{
	throttle_t *t = client_data;

	if (t->speed_out <= 0) return sockbuf_on_write(idx, THROTTLE_LEVEL, data, len);

	t->buf = realloc(t->buf, t->len + len);
	memcpy(t->buf + t->len, data, len);
	t->len += len;

	return(0);
}

static int throttle_on_written(void *client_data, int idx, int len, int remaining)
{
	throttle_t *t = client_data;

	return sockbuf_on_written(idx, THROTTLE_LEVEL, len, remaining + t->len);
}

static int throttle_on_delete(void *client_data, int idx)
{
	throttle_off(idx);
	return(0);
}

static sockbuf_filter_t throttle_filter = {
	"throttle",
	THROTTLE_LEVEL,
	NULL, NULL, NULL,
	throttle_on_read, throttle_on_write, throttle_on_written,
	NULL, throttle_on_delete
};

static throttle_t *throttle_lookup(int idx)
{
	throttle_t *t;

	for (t = throttle_head; t; t = t->next) {
		if (t->idx == idx) break;
	}
	return(t);
}

static void throttle_add(throttle_t *t)
{
	t->next = throttle_head;
	t->prev = NULL;
	if (throttle_head) throttle_head->prev = t;
	throttle_head = t;
}

static void throttle_remove(throttle_t *t)
{
	/* If we get removed during throttle_secondly() we have to update
	 * the next pointer to a valid one. */
	if (throttle_next == t) {
		throttle_next = t->next;
	}

	if (t->prev) t->prev->next = t->next;
	else throttle_head = t->next;

	if (t->next) t->next->prev = t->prev;
}

/* Every second, we reset the i/o counters for throttled sockbufs. */
static int throttle_secondly(void *ignore)
{
	throttle_t *t;
	int avail, r, out;

	for (t = throttle_head; t; t = throttle_next) {
		throttle_next = t->next;
		t->bytes_in -= t->speed_in;
		if (t->bytes_in < 0) t->bytes_in = 0;
		else if (t->bytes_in < t->speed_in) sockbuf_read(t->idx);

		out = 0;
		while (t->len && out < t->speed_out) {
			/* How many bytes can we send? */
			avail = t->speed_out - out;
			if (avail > t->len) avail = t->len;

			r = sockbuf_on_write(t->idx, THROTTLE_LEVEL, t->buf, avail);
			if (r < 0) break;

			memmove(t->buf, t->buf+avail, t->len-avail);
			t->len -= avail;
			out += avail;

			if (r > 0) sockbuf_on_written(t->idx, THROTTLE_LEVEL, r, t->len + avail - r);

		}
	}
	return(0);
}

int throttle_on(int idx)
{
	throttle_t *t;
	egg_timeval_t howlong;

	t = calloc(1, sizeof(*t));
	t->idx = idx;
	throttle_add(t);
	sockbuf_attach_filter(idx, &throttle_filter, t);

	if (throttle_timer < 0) {
		howlong.sec = 1;
		howlong.usec = 0;
		throttle_timer = timer_create_repeater(&howlong, "bandwidth throttler", throttle_secondly);
	}
	return(0);
}

int throttle_off(int idx)
{
	throttle_t *t;

	t = throttle_lookup(idx);
	if (!t) return(-1);

	throttle_remove(t);
	if (sockbuf_isvalid(idx)) {
		sockbuf_detach_filter(idx, &throttle_filter, NULL);
		if (t->len) sockbuf_on_write(idx, THROTTLE_LEVEL, t->buf, t->len);
	}

	if (t->buf) free(t->buf);
	free(t);

	if (!throttle_head) {
		timer_destroy(throttle_timer);
		throttle_timer = -1;
	}
	return(0);
}

int throttle_set(int idx, int dir, int speed)
{
	throttle_t *t;
	int r;

	t = throttle_lookup(idx);
	if (!t) return(-1);
	if (dir) {
		t->speed_out = speed;
		if (speed <= 0 && t->len) {
			r = sockbuf_on_write(idx, THROTTLE_LEVEL, t->buf, t->len);
			if (r > 0) sockbuf_on_written(idx, THROTTLE_LEVEL, r, t->len - r);
			t->len = 0;
		}
	}
	else t->speed_in = speed;
	return(0);
}
