/* channel_events.c: handle channel events
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
static const char rcsid[] = "$Id: channel_events.c,v 1.3 2005/03/03 18:45:26 stdarg Exp $";
 #endif

#include "server.h"

static bind_list_t channel_raw_binds[];

/* Prototypes. */
static void clear_masklist(channel_mask_list_t *l);
static int got_list_item(void *client_data, char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[]);
static int got_list_end(void *client_data, char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[]);
static void clear_masklists(channel_t *chan);

void channel_events_init()
{
	bind_table_t *table;

	bind_add_list("raw", channel_raw_binds);
	table = bind_table_lookup_or_fake("raw");
	if (!table) return;

	/* FIXME Putting these here for now... but they should probably be
	 * configurable or moved to an addon module/script. */
	bind_entry_add(table, NULL, "367", "banlistitem", BIND_WANTS_CD, got_list_item, (void *)'b');
	bind_entry_add(table, NULL, "346", "invitelistitem", BIND_WANTS_CD, got_list_item, (void *)'I');
	bind_entry_add(table, NULL, "348", "exceptlistitem", BIND_WANTS_CD, got_list_item, (void *)'e');
	bind_entry_add(table, NULL, "368", "banlistend", BIND_WANTS_CD, got_list_end, (void *)'b');
	bind_entry_add(table, NULL, "347", "invitelistend", BIND_WANTS_CD, got_list_end, (void *)'I');
	bind_entry_add(table, NULL, "349", "exceptlistend", BIND_WANTS_CD, got_list_end, (void *)'e');
}

static inline void free_member(channel_member_t *m)
{
	if (m->nick) free(m->nick);
	if (m->uhost) free(m->uhost);
	free(m);
}

static void clear_masklists(channel_t *chan)
{
	int i;

	for (i = 0; i < chan->nlists; i++) {
		clear_masklist(chan->lists[i]);
	}
}

static void clear_masklist(channel_mask_list_t *l)
{
	channel_mask_t *m, *next;

	for (m = l->head; m; m = next) {
		next = m->next;
		if (m->mask) free(m->mask);
		if (m->set_by) free(m->set_by);
		free(m);
	}
	memset(l, 0, sizeof(l));
}

/* Free online data like nicklist, banmasks, etc.. */
void channel_free_online(channel_t *chan)
{
	channel_member_t *m, *next_mem;
	int i;

	for (m = chan->member_head; m; m = next_mem) {
		next_mem = m->next;
		uhost_cache_decref(m->nick);
		free_member(m);
	}

	if (chan->topic) free(chan->topic);
	if (chan->topic_nick) free(chan->topic_nick);
	if (chan->key) free(chan->key);

	clear_masklists(chan);

	for (i = 0; i < chan->nargs; i++) {
		if (chan->args[i].value) free(chan->args[i].value);
	}
	if (chan->args) free(chan->args);

	chan->topic = chan->topic_nick = chan->key = NULL;
	chan->lists = NULL;
	chan->args = NULL;
	chan->member_head = NULL;
	chan->status = chan->nlists = chan->nargs = chan->nmembers = 0;
	chan->limit = -1;
}

void channel_events_destroy()
{
	bind_rem_list("raw", channel_raw_binds);
}

static channel_member_t *channel_add_member(channel_t *chan, const char *nick, const char *uhost)
{
	channel_member_t *m;

	/* See if this member is already added. */
	for (m = chan->member_head; m; m = m->next) {
		if (!(current_server.strcmp)(m->nick, nick)) break;
	}

	/* Do we need to create a new member? */
	if (!m) {
		m = calloc(1, sizeof(*m));
		m->nick = strdup(nick);
		timer_get_now_sec(&m->join_time);
		m->next = chan->member_head;
		chan->member_head = m;
		chan->nmembers++;
	}

	/* Do we need to fill in the uhost? */
	if (uhost && !m->uhost) {
		m->uhost = strdup(uhost);
	}

	uhost_cache_addref(nick, uhost);

	return(m);
}

channel_member_t *channel_lookup_member(channel_t *chan, const char *nick)
{
	channel_member_t *m;

	for (m = chan->member_head; m; m = m->next) {
		if (!(current_server.strcmp)(m->nick, nick)) break;
	}
	return(m);
}

/* We just finished connecting to the server, so join our channels. */
void channel_on_connect()
{
	channel_t *chan;
	char *key;
	int inactive;

	for (chan = channel_head; chan; chan = chan->next) {
		channel_get_int(chan, &inactive, "inactive", 0, NULL);
		if (inactive) continue;
		channel_get(chan, &key, "key", 0, NULL);
		if (key && strlen(key)) printserv(SERVER_NORMAL, "JOIN %s %s", chan->name, key);
		else printserv(SERVER_NORMAL, "JOIN %s", chan->name);
	}
}

void channel_on_join(const char *chan_name, const char *nick, const char *uhost)
{
	channel_t *chan;
	channel_member_t *m;

	/* Lookup channel or create. */
	chan = channel_probe(chan_name, 1);
	if (!chan) return;

	/* Add a new member. */
	m = channel_add_member(chan, nick, uhost);

	/* Is it me? If so, request channel lists. */
	if (match_my_nick(nick)) {
		int i;
		chan->bot = m;
		chan->status |= (CHANNEL_WHOLIST | CHANNEL_JOINED);
		printserv(SERVER_NORMAL, "WHO %s\r\n", chan_name);
		printserv(SERVER_NORMAL, "MODE %s\r\n", chan_name);
		for (i = 0; i < chan->nlists; i++) {
			chan->lists[i]->loading = 1;
		}
		printserv(SERVER_NORMAL, "MODE %s %s\r\n", chan_name, current_server.type1modes);
	}
}

void channel_on_leave(const char *chan_name, const char *nick, const char *uhost, user_t *u)
{
	channel_t *chan;
	channel_member_t *prev = NULL, *m;

	chan = channel_probe(chan_name, 0);
	if (!chan) return;

	uhost_cache_decref(nick);
	for (m = chan->member_head; m; m = m->next) {
		if (!(current_server.strcmp)(m->nick, nick)) break;
		prev = m;
	}
	if (!m) return;

	if (prev) prev->next = m->next;
	else chan->member_head = m->next;
	free_member(m);
	chan->nmembers--;

	bind_check(BT_leave, u ? &u->settings[0].flags : NULL, chan_name, nick, uhost, u, chan_name);

	/* Are we the ones leaving? */
	if (match_my_nick(nick)) {
		if (chan->flags & CHANNEL_STATIC) channel_free_online(chan);
		else channel_free(chan);
	}
}

void channel_on_quit(const char *nick, const char *uhost, user_t *u)
{
	channel_t *chan;

	for (chan = channel_head; chan; chan = chan->next) {
		channel_on_leave(chan->name, nick, uhost, u);
	}
}

void channel_on_nick(const char *old_nick, const char *new_nick)
{
	channel_t *chan;
	channel_member_t *m;

	uhost_cache_swap(old_nick, new_nick);

	for (chan = channel_head; chan; chan = chan->next) {
		for (m = chan->member_head; m; m = m->next) {
			if (!(current_server.strcmp)(m->nick, old_nick)) {
				str_redup(&m->nick, new_nick);
				break;
			}
		}
	}
}

/* :server 352 <ournick> <chan> <user> <host> <server> <nick> <H|G>[*][@|+] :<hops> <name> */
static int got352(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name, *nick, *uhost, *flags, *ptr, changestr[3];
	int i;
	channel_member_t *m;
	channel_t *chan = NULL;
	chan_name = args[1];
	chan = channel_probe(chan_name, 0);
	if (!chan) return(0);
	nick = args[5];
	flags = args[6];
	uhost = egg_mprintf("%s@%s", args[2], args[3]);
	m = channel_add_member(chan, nick, uhost);
	changestr[0] = '+';
	changestr[2] = 0;
	flags++;
	while (*flags) {
		ptr = strchr(current_server.whoprefix, *flags);
		if (ptr) {
			i = ptr - current_server.whoprefix;
			changestr[1] = current_server.modeprefix[i];
			flag_merge_str(&m->mode, changestr);
		}
		flags++;
	}
	free(uhost);
	return(0);
}

/* :server 315 <ournick> <name> :End of /WHO list */
static int got315(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[1];
	channel_t *chan = NULL;

	channel_probe(chan_name, 0);
	if (chan) chan->status &= ~CHANNEL_WHOLIST;
	return(0);
}

/* :server 353 <ournick> = <chan> :<mode><nick1> <mode><nick2> ... */
static int got353(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name, *nick, *nicks, *flags, *ptr, *prefixptr, changestr[3];
	int i;
	channel_member_t *m;
	channel_t *chan = NULL;

	chan_name = args[2];
	chan = channel_probe(chan_name, 0);
	if (!chan) return(0);

	nicks = strdup(args[3]);
	ptr = nicks;
	changestr[0] = '+';
	changestr[2] = 0;
	while (ptr && *ptr) {
		while (isspace(*ptr)) ptr++;
		flags = ptr;
		while (strchr(current_server.whoprefix, *ptr)) ptr++;
		nick = ptr;
		while (*ptr && !isspace(*ptr)) ptr++;
		if (*ptr) *ptr = 0;
		else ptr = NULL;
		m = channel_add_member(chan, nick, NULL);
		*nick = 0;
		while (*flags) {
			prefixptr = strchr(current_server.whoprefix, *flags);
			if (prefixptr) {
				i = prefixptr - current_server.whoprefix;
				changestr[1] = current_server.modeprefix[i];
				flag_merge_str(&m->mode, changestr);
			}
			flags++;
		}
		if (ptr) ptr++;
	}
	free(nicks);
	return(0);
}

/* :server 366 <ournick> <name> :End of /NAMES list */
static int got366(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[1];
	channel_t *chan = NULL;

	chan = channel_probe(chan_name, 0);
	if (chan) chan->status &= ~CHANNEL_NAMESLIST;
	return(0);
}

/* :origin TOPIC <chan> :topic */
static int gottopic(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[0];
	char *topic = args[1];
	channel_t *chan = NULL;

	chan = channel_probe(chan_name, 0);
	if (!chan) return(0);

	str_redup(&chan->topic, topic);
	if (chan->topic_nick) free(chan->topic_nick);
	if (from_uhost) chan->topic_nick = egg_mprintf("%s!%s", from_nick, from_uhost);
	else chan->topic_nick = strdup(from_nick);
	timer_get_now_sec(&chan->topic_time);

	return(0);
}

/* :server 331 <ournick> <chan> :No topic is set */
static int got331(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[1];
	channel_t *chan = NULL;

	chan = channel_probe(chan_name, 0);
	if (!chan) return(0);

	str_redup(&chan->topic, "");
	return(0);
}

/* :server 332 <ournick> <chan> :<topic> */
static int got332(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[1];
	char *topic = args[2];
	channel_t *chan = NULL;

	chan = channel_probe(chan_name, 0);
	if (!chan) return(0);

	str_redup(&chan->topic, topic);
	return(0);
}

/* :server 333 <ournick> <chan> <nick> <time> */
static int got333(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[1];
	char *nick = args[2];
	char *time = args[3];
	channel_t *chan = NULL;

	chan = channel_probe(chan_name, 0);
	if (!chan) return(0);

	str_redup(&chan->topic_nick, nick);
	chan->topic_time = atoi(time);
	return(0);
}

/* :server 367 <ournick> <chan> <ban> [<nick> <time>]
 * :server 346 <ournick> <chan> <invite> [<nick> <time>]
 * :server 348 <ournick> <chan> <except> [<nick> <time>] */
static int got_list_item(void *client_data, char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[1];
	char *ban = args[2];
	char type = (char) (int) client_data;
	channel_t *chan;
	channel_mask_list_t *l;

	chan = channel_probe(chan_name, 0);
	if (!chan) return(0);
	l = channel_get_mask_list(chan, type);
	if (!l || !(l->loading)) return(0);
	channel_add_mask(chan, type, ban, (nargs > 3) ? args[3] : NULL, (nargs > 4) ? atoi(args[4]) : -1);
	return(0);
}

/* :server 368 <ournick> <chan> :End of channel ban list */
/* :server 347 <ournick> <chan> :End of channel invite except list */
/* :server 349 <ournick> <chan> :End of channel ban except list */
static int got_list_end(void *client_data, char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[1];
	char type = (char) (int) client_data;
	channel_t *chan;
	channel_mask_list_t *l;

	chan = channel_probe(chan_name, 0);
	if (!chan) return(0);
	l = channel_get_mask_list(chan, type);
	if (l) l->loading = 0;
/* FIXME - See if we need to set any modes */
	return(0);
}

/**********************************************
 * Join/leave stuff.
 */

/* Got a join message. */
static int gotjoin(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan = args[0];

	channel_on_join(chan, from_nick, from_uhost);
	bind_check(BT_join, u ? &u->settings[0].flags : NULL, chan, from_nick, from_uhost, u, chan);
	return(0);
}

/* Got a part message. */
static int gotpart(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan = args[0];
	char *text = args[1];

	channel_on_leave(chan, from_nick, from_uhost, u);
	bind_check(BT_part, u ? &u->settings[0].flags : NULL, chan, from_nick, from_uhost, u, chan, text);
	return(0);
}

/* Got a quit message. */
static int gotquit(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *text = args[0];

	channel_on_quit(from_nick, from_uhost, u);
	bind_check(BT_quit, u ? &u->settings[0].flags : NULL, from_nick, from_nick, from_uhost, u, text);

	return(0);
}

/* Got a kick message. */
static int gotkick(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan = args[0];
	char *victim = args[1];
	char *text = args[2];
	char *uhost;

	if (!text) text = "";
	uhost = uhost_cache_lookup(victim);
	if (!uhost) uhost = "";

	channel_on_leave(chan, victim, uhost, u);
	bind_check(BT_kick, u ? &u->settings[0].flags : NULL, from_nick, from_nick, from_uhost, u, chan, victim, text);

	return(0);
}

/* Got nick change.  */
static int gotnick(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *newnick = args[0];

	channel_on_nick(from_nick, newnick);
	bind_check(BT_nick, u ? &u->settings[0].flags : NULL, from_nick, from_nick, from_uhost, u, newnick);

	/* Is it our nick that's changing? */
	if (match_my_nick(from_nick)) str_redup(&current_server.nick, newnick);

	return(0);
}

/*************************************************
 * Mode stuff.
 */

static void parse_chan_mode(char *from_nick, char *from_uhost, user_t *u, int nargs, char *args[], int trigger_bind)
{
	int hasarg, curarg, modify_member, modify_channel, modify_list;
	int i;
	channel_member_t *m;
	char changestr[3];
	char *dest;
	char *change;
	channel_t *chan;
	const char *arg;

	if (nargs < 2) return;

	dest = args[0];
	change = args[1];

	chan = channel_probe(dest, 0);
	if (!chan) return;

	changestr[0] = '+';
	changestr[2] = 0;

	curarg = 2;
	while (*change) {
		/* Direction? */
		if (*change == '+' || *change == '-') {
			changestr[0] = *change;
			change++;
			continue;
		}

		/* Figure out what kind of change it is and if it takes an
		 * argument. */
		modify_member = modify_channel = modify_list = 0;
		if (strchr(current_server.modeprefix, *change)) {
			hasarg = 1;
			modify_member = 1;
		}
		else if (strchr(current_server.type1modes, *change)) {
			hasarg = 1;
			modify_list = 1;
		}
		else if (strchr(current_server.type2modes, *change)) {
			hasarg = 1;
		}
		else if (changestr[0] == '+' && strchr(current_server.type3modes, *change)) {
			hasarg = 1;
		}
		else {
			modify_channel = 1;
			hasarg = 0;
		}

		if (hasarg) {
			if (curarg >= nargs) arg = "";
			else arg = args[curarg++];
		}
		else arg = NULL;

		changestr[1] = *change;

		if (modify_member) {
			/* Find the person it modifies and apply. */
			m = channel_lookup_member(chan, arg);
		}
		else if (modify_channel) {
			/* Simple flag change for channel. */
			flag_merge_str(&chan->mode, changestr);
		}
		else if (modify_list) {
			/* Add/remove an address from a list. */
			if (changestr[0] == '+') channel_add_mask(chan, *change, arg, from_nick, timer_get_now_sec(NULL));
			else channel_del_mask(chan, *change, arg);
		}
		else if (changestr[0] == '+') {
			/* + flag change that requires special handling. */
			switch (*change) {
				case 'k':
					str_redup(&chan->key, arg);
					break;
				case 'l':
					chan->limit = atoi(arg);
					break;
			}
			for (i = 0; i < chan->nargs; i++) {
				if (chan->args[i].type == *change) {
					str_redup(&chan->args[i].value, arg);
					break;
				}
			}
		}
		else {
			/* - flag change that requires special handling. */
			switch (*change) {
				case 'k':
					if (chan->key) free(chan->key);
					chan->key = NULL;
					break;
				case 'l':
					chan->limit = 0;
					break;
			}
			for (i = 0; i < chan->nargs; i++) {
				if (chan->args[i].type == *change) {
					str_redup(&chan->args[i].value, NULL);
					break;
				}
			}
		}

		/* Now trigger the mode bind. */
		if (trigger_bind) bind_check(BT_mode, u ? &u->settings[0].flags : NULL, changestr, from_nick, from_uhost, u, dest, changestr, arg);
		change++;
	}
}

/* Got an absolute channel mode. */
static int got324(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	parse_chan_mode(from_nick, from_uhost, u, nargs-1, args+1, 0);
	return(0);
}

/* Got a mode change. */
static int gotmode(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *dest = args[0];
	char *change = args[1];
	char changestr[3];

	changestr[0] = '+';
	changestr[2] = 0;

	/* Is it a user mode? */
	if (!strchr(current_server.chantypes, *dest)) {
		while (*change) {
			/* Direction? */
			if (*change == '+' || *change == '-') {
				changestr[0] = *change;
				change++;
				continue;
			}

			changestr[1] = *change;
			bind_check(BT_mode, u ? &u->settings[0].flags : NULL, changestr, from_nick, from_uhost, u, dest, changestr, NULL);
			change++;
		}
		return(0);
	}

	parse_chan_mode(from_nick, from_uhost, u, nargs, args, 1);
	return(0);
}

static bind_list_t channel_raw_binds[] = {
	/* WHO replies */
	{NULL, "352", got352}, /* WHO reply */
	{NULL, "315", got315}, /* end of WHO */

	/* NAMES replies */
	{NULL, "353", got353}, /* NAMES reply */
	{NULL, "366", got366}, /* end of NAMES */

	/* Topic info */
	{NULL, "TOPIC", gottopic},
	{NULL, "331", got331},
	{NULL, "332", got332},
	{NULL, "333", got333},

	/* Channel member stuff. */
	{NULL, "JOIN", gotjoin},
	{NULL, "PART", gotpart},
	{NULL, "QUIT", gotquit},
	{NULL, "KICK", gotkick},
	{NULL, "NICK", gotnick},

	/* Mode. */
	{NULL, "324", got324},
	{NULL, "MODE", gotmode},

	{NULL, NULL, NULL}
};
