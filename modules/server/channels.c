/* channels.c: channel support
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
static const char rcsid[] = "$Id: channels.c,v 1.23 2004/06/29 21:28:17 stdarg Exp $";
#endif

#include "server.h"

#define UHOST_CACHE_SIZE 47

channel_t *channel_head = NULL;
hash_table_t *uhost_cache_ht = NULL;

int nchannels = 0;

static bind_list_t channel_raw_binds[];

/* Prototypes. */
static int uhost_cache_delete(const void *key, void *data, void *param);
static void clear_masklist(channel_mask_list_t *l);
static int got_list_item(void *client_data, char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[]);
static int got_list_end(void *client_data, char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[]);
void uhost_cache_decref(const char *nick);

void server_channel_init()
{
	bind_table_t *table;

	channel_head = NULL;
	nchannels = 0;
	uhost_cache_ht = hash_table_create(NULL, NULL, UHOST_CACHE_SIZE, HASH_TABLE_STRINGS);
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

static void free_member(channel_member_t *m)
{
	if (m->nick) free(m->nick);
	if (m->uhost) free(m->uhost);
	free(m);
}

static void free_channel(channel_t *chan)
{
	channel_member_t *m, *next_mem;
	int i;

	if (chan->name) free(chan->name);
	for (m = chan->member_head; m; m = next_mem) {
		next_mem = m->next;
		uhost_cache_decref(m->nick);
		free_member(m);
	}
	if (chan->topic) free(chan->topic);
	if (chan->topic_nick) free(chan->topic_nick);
	if (chan->key) free(chan->key);
	for (i = 0; i < chan->nlists; i++) {
		clear_masklist(chan->lists+i);
	}
	if (chan->lists) free(chan->lists);
	free(chan);
}

void channel_reset()
{
	channel_t *chan, *next_chan;

	/* Clear out channel list. */
	for (chan = channel_head; chan; chan = next_chan) {
		next_chan = chan->next;
		free_channel(chan);
	}
	channel_head = NULL;
	nchannels = 0;

	/* And the uhost cache. */
	hash_table_walk(uhost_cache_ht, uhost_cache_delete, NULL);
	hash_table_delete(uhost_cache_ht);
	uhost_cache_ht = hash_table_create(NULL, NULL, UHOST_CACHE_SIZE, HASH_TABLE_STRINGS);
}

void server_channel_destroy()
{
	/* Free everything. */
	channel_reset();
	hash_table_delete(uhost_cache_ht);
}

void channel_lookup(const char *chan_name, int create, channel_t **chanptr, channel_t **prevptr)
{
	channel_t *chan, *prev;
	int i;

	*chanptr = NULL;
	if (prevptr) *prevptr = NULL;

	prev = NULL;
	for (chan = channel_head; chan; chan = chan->next) {
		if (!strcasecmp(chan->name, chan_name)) {
			*chanptr = chan;
			if (prevptr) *prevptr = prev;
			return;
		}
		prev = chan;
	}
	if (!create) return;

	nchannels++;
	chan = calloc(1, sizeof(*chan));
	chan->name = strdup(chan_name);
	chan->nlists = strlen(current_server.type1modes);
	chan->lists = calloc(chan->nlists, sizeof(*chan->lists));
	for (i = 0; i < chan->nlists; i++) {
		chan->lists[i].type = current_server.type1modes[i];
	}
	chan->nargs = strlen(current_server.type2modes);
	chan->args = calloc(chan->nargs, sizeof(*chan->args));
	for (i = 0; i < chan->nargs; i++) {
		chan->args[i].type = current_server.type2modes[i];
	}
	chan->next = channel_head;
	channel_head = chan;
	*chanptr = chan;
}

static void make_lowercase(char *nick)
{
	while (*nick) {
		*nick = tolower(*nick);
		nick++;
	}
}

static int uhost_cache_delete(const void *key, void *dataptr, void *param)
{
	uhost_cache_entry_t *cache = *(void **)dataptr;

	if (cache->nick) free(cache->nick);
	if (cache->uhost) free(cache->uhost);
	free(cache);
	return(0);
}

static uhost_cache_entry_t *cache_lookup(const char *nick)
{
	char buf[64], *lnick;
	uhost_cache_entry_t *cache = NULL;

	lnick = egg_msprintf(buf, sizeof(buf), NULL, "%s", nick);
	make_lowercase(lnick);
	hash_table_find(uhost_cache_ht, lnick, &cache);
	if (lnick != buf) free(lnick);
	return(cache);
}

char *uhost_cache_lookup(const char *nick)
{
	uhost_cache_entry_t *cache;

	cache = cache_lookup(nick);
	if (cache) return(cache->uhost);
	return(NULL);
}

void uhost_cache_addref(const char *nick, const char *uhost)
{
	char buf[64], *lnick;
	uhost_cache_entry_t *cache = NULL;

	lnick = egg_msprintf(buf, sizeof(buf), NULL, "%s", nick);
	make_lowercase(lnick);
	hash_table_find(uhost_cache_ht, lnick, &cache);
	if (!cache) {
		cache = calloc(1, sizeof(*cache));
		cache->nick = strdup(nick);
		make_lowercase(cache->nick);
		if (uhost) cache->uhost = strdup(uhost);
		hash_table_insert(uhost_cache_ht, cache->nick, cache);
	}
	cache->ref_count++;
	if (lnick != buf) free(lnick);
}

void uhost_cache_decref(const char *nick)
{
	uhost_cache_entry_t *cache;

	/* We don't decrement ourselves.. we always know our own host. */
	if (match_my_nick(nick)) return;

	cache = cache_lookup(nick);
	if (!cache) return;

	cache->ref_count--;
	if (cache->ref_count <= 0) {
		hash_table_remove(uhost_cache_ht, cache->nick, NULL);
		uhost_cache_delete(NULL, cache, NULL);
	}
}

channel_member_t *channel_add_member(const char *chan_name, const char *nick, const char *uhost)
{
	channel_t *chan;
	channel_member_t *m;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan) return(NULL);

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

void channel_on_join(const char *chan_name, const char *nick, const char *uhost)
{
	channel_t *chan;
	int create;
	int i;

	if (match_my_nick(nick)) create = 1;
	else create = 0;

	channel_lookup(chan_name, create, &chan, NULL);
	if (!chan) return;

	if (create) {
		chan->status |= CHANNEL_WHOLIST;
		printserv(SERVER_NORMAL, "WHO %s\r\n", chan_name);
		printserv(SERVER_NORMAL, "MODE %s\r\n", chan_name);
		for (i = 0; i < chan->nlists; i++) chan->lists[i].loading = 1;
		printserv(SERVER_NORMAL, "MODE %s %s\r\n", chan_name, current_server.type1modes);
	}

	channel_add_member(chan_name, nick, uhost);
}

static void real_channel_leave(channel_t *chan, const char *nick, const char *uhost, user_t *u)
{
	char *chan_name;

	if (match_my_nick(nick)) {
		chan_name = strdup(chan->name);
		free_channel(chan);
		nchannels--;
	}
	else {
		channel_member_t *prev, *m;

		chan_name = chan->name;
		uhost_cache_decref(nick);
		prev = NULL;
		for (m = chan->member_head; m; m = m->next) {
			if (!(current_server.strcmp)(m->nick, nick)) break;
			prev = m;
		}
		if (!m) return;

		if (prev) prev->next = m->next;
		else chan->member_head = m->next;
		free_member(m);
		chan->nmembers--;
	}

	bind_check(BT_leave, u ? &u->settings[0].flags : NULL, chan_name, nick, uhost, u, chan_name);

	if (match_my_nick(nick)) free(chan_name);
}

void channel_on_leave(const char *chan_name, const char *nick, const char *uhost, user_t *u)
{
	channel_t *chan, *prev;

	channel_lookup(chan_name, 0, &chan, &prev);
	if (!chan) return;
	if (match_my_nick(nick)) {
		if (prev) prev->next = chan->next;
		else channel_head = chan->next;
		nchannels--;
	}
	real_channel_leave(chan, nick, uhost, u);
}

void channel_on_quit(const char *nick, const char *uhost, user_t *u)
{
	channel_t *chan;

	if (match_my_nick(nick)) {
		while (channel_head) real_channel_leave(channel_head, nick, uhost, u);
		channel_head = NULL;
		nchannels = 0;
	}
	else {
		for (chan = channel_head; chan; chan = chan->next) {
			real_channel_leave(chan, nick, uhost, u);
		}
	}
}

void channel_on_nick(const char *old_nick, const char *new_nick)
{
	channel_t *chan;
	channel_member_t *m;
	char buf[64], *lnick;
	uhost_cache_entry_t *cache = NULL;

	lnick = egg_msprintf(buf, sizeof(buf), NULL, "%s", old_nick);
	make_lowercase(lnick);
	hash_table_remove(uhost_cache_ht, lnick, &cache);
	if (lnick != buf) free(lnick);

	if (cache) {
		str_redup(&cache->nick, new_nick);
		make_lowercase(cache->nick);
		hash_table_insert(uhost_cache_ht, cache->nick, cache);
	}

	for (chan = channel_head; chan; chan = chan->next) {
		for (m = chan->member_head; m; m = m->next) {
			if (!(current_server.strcmp)(m->nick, old_nick)) {
				str_redup(&m->nick, new_nick);
				break;
			}
		}
	}
}

int channel_list(const char ***chans)
{
	channel_t *chan;
	int i;

	*chans = calloc(nchannels+1, sizeof(*chans));
	i = 0;
	for (chan = channel_head; chan; chan = chan->next) {
		(*chans)[i] = chan->name;
		i++;
	}
	(*chans)[i] = NULL;
	return(i);
}

int channel_list_members(const char *chan, const char ***members)
{
	channel_t *chanptr;
	channel_member_t *m;
	int i;

	channel_lookup(chan, 0, &chanptr, NULL);
	if (!chanptr) {
		*members = NULL;
		return(-1);
	}

	*members = calloc(chanptr->nmembers+1, sizeof(*members));
	i = 0;
	for (m = chanptr->member_head; m; m = m->next) {
		(*members)[i] = m->nick;
		i++;
	}
	return(i);
}

/* :server 352 <ournick> <chan> <user> <host> <server> <nick> <H|G>[*][@|+] :<hops> <name> */
static int got352(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name, *nick, *uhost, *flags, *ptr, changestr[3];
	int i;
	channel_member_t *m;

	chan_name = args[1];
	nick = args[5];
	flags = args[6];
	uhost = egg_mprintf("%s@%s", args[2], args[3]);
	m = channel_add_member(chan_name, nick, uhost);
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

	channel_lookup(chan_name, 0, &chan, NULL);
	if (chan) chan->status &= ~CHANNEL_WHOLIST;
	return(0);
}

/* :server 353 <ournick> = <chan> :<mode><nick1> <mode><nick2> ... */
static int got353(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name, *nick, *nicks, *flags, *ptr, *prefixptr, changestr[3];
	int i;
	channel_member_t *m;

	chan_name = args[2];
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
		m = channel_add_member(chan_name, nick, NULL);
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

	channel_lookup(chan_name, 0, &chan, NULL);
	if (chan) chan->status &= ~CHANNEL_NAMESLIST;
	return(0);
}

/* :origin TOPIC <chan> :topic */
static int gottopic(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[0];
	char *topic = args[1];
	channel_t *chan = NULL;

	channel_lookup(chan_name, 0, &chan, NULL);
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

	channel_lookup(chan_name, 0, &chan, NULL);
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

	channel_lookup(chan_name, 0, &chan, NULL);
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

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan) return(0);

	str_redup(&chan->topic_nick, nick);
	chan->topic_time = atoi(time);
	return(0);
}

/************************************
 * Mask list handling
 */

channel_mask_list_t *channel_get_mask_list(channel_t *chan, char type)
{
	int i;

	for (i = 0; i < chan->nlists; i++) {
		if (chan->lists[i].type == type) return(chan->lists+i);
	}
	return(NULL);
}

void channel_add_mask(channel_t *chan, char type, const char *mask, const char *set_by, int time)
{
	channel_mask_list_t *l;
	channel_mask_t *m;

	l = channel_get_mask_list(chan, type);
	if (!l) return;

	m = calloc(1, sizeof(*m));
	m->mask = strdup(mask);
	if (set_by) m->set_by = strdup(set_by);
	m->time = time;
	m->next = l->head;
	l->head = m;
}

static void free_mask(channel_mask_t *m)
{
	if (m->mask) free(m->mask);
	if (m->set_by) free(m->set_by);
	free(m);
}

void channel_del_mask(channel_t *chan, char type, const char *mask)
{
	channel_mask_list_t *l;
	channel_mask_t *m, *prev;

	l = channel_get_mask_list(chan, type);
	if (!l) return;

	prev = NULL;
	for (m = l->head; m; m = m->next) {
		if (!(current_server.strcmp)(m->mask, mask)) break;
		prev = m;
	}
	if (!m) return;
	if (prev) prev->next = m->next;
	else l->head = m->next;
	free_mask(m);
}

static void clear_masklist(channel_mask_list_t *l)
{
	channel_mask_t *m, *next;

	for (m = l->head; m; m = next) {
		next = m->next;
		free_mask(m);
	}
	l->head = NULL;
}

void channel_clear_masks(channel_t *chan, char type)
{
	channel_mask_list_t *l;

	l = channel_get_mask_list(chan, type);
	if (l) clear_masklist(l);
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

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan) return(0);
	l = channel_get_mask_list(chan, type);
	if (!l || !(l->loading)) return(0);
	channel_add_mask(chan, type, ban, (nargs > 3) ? args[3] : NULL, (nargs > 4) ? atoi(args[4]) : -1);
	return(0);
}

/* :server 368 <ournick> <chan> :End of channel ban list */
static int got_list_end(void *client_data, char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[1];
	char type = (char) (int) client_data;
	channel_t *chan;
	channel_mask_list_t *l;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan) return(0);
	l = channel_get_mask_list(chan, type);
	if (l) l->loading = 0;
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

/* nick is NULL to get channel mode */
int channel_mode(const char *chan_name, const char *nick, char *buf)
{
	channel_t *chan;
	channel_member_t *m;
	flags_t *flags = NULL;

	buf[0] = 0;
	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan) return(-1);
	if (!nick) flags = &chan->mode;
	else {
		for (m = chan->member_head; m; m = m->next) {
			if (!(current_server.strcmp)(nick, m->nick)) {
				flags = &m->mode;
				break;
			}
		}
		if (!flags) return(-2);
	}
	flag_to_str(flags, buf);
	return(0);
}

int channel_mode_arg(const char *chan_name, int type, const char **value)
{
	int i;
	channel_t *chan;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan) return(-1);
	for (i = 0; i < chan->nargs; i++) {
		if (chan->args[i].type == type) {
			*value = chan->args[i].value;
			return(0);
		}
	}
	return(-2);
}

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

	channel_lookup(dest, 0, &chan, NULL);
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
			for (m = chan->member_head; m; m = m->next) {
				if (!(current_server.strcmp)(m->nick, arg)) {
					flag_merge_str(&m->mode, changestr);
					break;
				}
			}
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
