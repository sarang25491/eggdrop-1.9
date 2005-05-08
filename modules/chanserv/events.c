/* events.c -- respond to irc events
 *
 * Copyright (C) 2003, 2004 Eggheads Development Team
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
static const char rcsid[] = "$Id: events.c,v 1.3 2005/05/08 04:40:12 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "chanserv.h"

typedef struct modefight {
	struct modefight *next;
	int mode, lastdir;
	char *chan, *firstsource, *secondsource, *lastsource, *target;
	time_t expire_time;
} modefight_t;

static modefight_t *modefight_head = NULL;
static int modefight_timer_id = -1;

static int got_join(const char *nick, const char *uhost, user_t *u, const char *chan)
{
	channel_t *channel;
	int greets = 0;
	char *greet = NULL;

	/* Update stats. */
	chanserv_update_stats(CHANSERV_STAT_JOIN, chan, nick, uhost);

	if (!u) return(0);

	/* Check if we're sending greets. */
	channel = server->channel_lookup(chan);
	if (!channel) return(0);
	server->channel_get_int(channel, &greets, "greets", 0, NULL);
	if (greets <= 0) return(0);
	user_get_setting(u, chan, "greet", &greet);
	if (!greet) user_get_setting(u, NULL, "greet", &greet);
	if (greet) server->printserv(SERVER_NORMAL, "PRIVMSG %s :[%s] %s", chan, u->handle, greet);
	return(0);
}

static int got_part(const char *nick, const char *uhost, user_t *u, const char *chan, const char *text)
{
	chanserv_update_stats(CHANSERV_STAT_PART, chan, nick, uhost);
	chanserv_update_stats(CHANSERV_STAT_LEAVE, chan, nick, uhost);
	return(0);
}

static int got_pub(const char *nick, const char *uhost, user_t *u, const char *chan, const char *text)
{
	chanserv_update_stats(CHANSERV_STAT_MSG, chan, nick, uhost);
	chanserv_update_stats(CHANSERV_STAT_LINE, chan, nick, uhost);
	return(0);
}

static int got_msg(const char *nick, const char *uhost, user_t *u, const char *text)
{
	chanserv_update_stats(CHANSERV_STAT_MSG, NULL, nick, uhost);
	chanserv_update_stats(CHANSERV_STAT_LINE, NULL, nick, uhost);
	return(0);
}

static void modefight_delete(modefight_t *m)
{
	if (m->chan) free(m->chan);
	if (m->firstsource) free(m->firstsource);
	if (m->secondsource) free(m->secondsource);
	if (m->lastsource) free(m->lastsource);
	if (m->target) free(m->target);
	free(m);
}

static int modefight_cleanup()
{
	time_t now = timer_get_now_sec(NULL);
	modefight_t *m, *prev, *next;

	prev = NULL;
	for (m = modefight_head; m; m = next) {
		next = m->next;
		if (now > m->expire_time) {
			if (prev) prev->next = next;
			else modefight_head = next;
			modefight_delete(m);
		}
		else prev = m;
	}
	return(0);
}

static modefight_t *modefight_probe(const char *chan, const char *nick, const char *target, int dir, int mode, int create)
{
	modefight_t *m;
	for (m = modefight_head; m; m = m->next) {
		if (m->mode != mode || strcasecmp(chan, m->chan)) continue;
		if ((target && !m->target) || (!target && m->target)) continue;
		if ((target && m->target) && strcasecmp(target, m->target)) continue;
		return(m);
	}
	if (!create) return(NULL);
	m = calloc(1, sizeof(*m));
	m->chan = strdup(chan);
	m->firstsource = strdup(nick);
	m->lastsource = strdup(nick);
	m->lastdir = dir;
	m->mode = mode;
	if (target) m->target = strdup(target);
	m->next = modefight_head;
	modefight_head = m;
	return(m);
}

static int got_mode(const char *nick, const char *uhost, user_t *u, const char *dest, const char *change, const char *arg)
{
	modefight_t *m;
	int period;

	/* Only count channel mode changes. */
	if (server->match_botnick(dest)) return(0);

	chanserv_update_stats(CHANSERV_STAT_MODE, dest, nick, uhost);
	m = modefight_probe(dest, nick, arg, *change, *(change+1), 1);
	chanserv_lookup_config(dest, CHANSERV_STAT_MODEFIGHT, NULL, &period);
	m->expire_time = timer_get_now_sec(NULL) + period;
	if (!m->secondsource && strcasecmp(nick, m->firstsource)) m->secondsource = strdup(nick);
	if (m->lastdir != *change && strcasecmp(m->lastsource, nick)) {
		m->lastdir = *change;
		str_redup(&m->lastsource, nick);
		chanserv_update_stats(CHANSERV_STAT_MODEFIGHT, dest, nick, uhost);
	}
	return(0);
}

int events_init()
{
	egg_timeval_t howlong = {1, 0};

	bind_add_simple("join", NULL, NULL, got_join);
	bind_add_simple("part", NULL, NULL, got_part);
	bind_add_simple("pubm", NULL, NULL, got_pub);
	bind_add_simple("msgm", NULL, NULL, got_msg);
	bind_add_simple("mode", NULL, NULL, got_mode);
	timer_create_repeater(&howlong, "modefight_cleanup", modefight_cleanup);
	return(0);
}

int events_shutdown()
{
	bind_rem_simple("join", NULL, NULL, got_join);
	bind_rem_simple("part", NULL, NULL, got_part);
	bind_rem_simple("pubm", NULL, NULL, got_pub);
	bind_rem_simple("msgm", NULL, NULL, got_msg);
	bind_rem_simple("mode", NULL, NULL, got_mode);
	timer_destroy(modefight_timer_id);
	return(0);
}
