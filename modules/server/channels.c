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
static const char rcsid[] = "$Id: channels.c,v 1.31 2004/08/27 19:16:43 darko Exp $";
 #endif

#include "server.h"

#define UHOST_CACHE_SIZE 47

#define CHAN_INACTIVE		0x1
#define CHAN_CYCLE		0x2

channel_t *channel_head = NULL;
hash_table_t *uhost_cache_ht = NULL;

int nchannels = 0;

static bind_list_t channel_raw_binds[];

static const char *channel_builtin_bools[32];
static const char *channel_builtin_ints[];
static const char *channel_builtin_coupplets[];
static const char *channel_builtin_strings[];

static channel_t global_chan;

/* Prototypes. */
static int uhost_cache_delete(const void *key, void *data, void *param);
static void clear_masklist(channel_mask_list_t *l);
static void clear_masklists_online_only(channel_t *chanptr);
static int got_list_item(void *client_data, char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[]);
static int got_list_end(void *client_data, char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[]);
void uhost_cache_decref(const char *nick);
void server_channel_destroy();
static int chanfile_load(const char *filename);
int channame_member_has_flag(const char *nick, const char *channame, unsigned char c);
static int chanptr_member_has_flag(const char *nick, channel_t *chan, unsigned char c);
static int channels_minutely();

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

	memset(&global_chan, 0, sizeof global_chan);
	global_chan.builtin_bools = 1;
	chanfile_load(server_config.chanfile);

	bind_add_simple(BTN_EVENT, NULL, "minutely", channels_minutely);
}

static inline void free_member(channel_member_t *m)
{
	free(m->nick);
	free(m->uhost);
	free(m);
}

/* Free nicklist, banmasks, etc.. */
static void free_channel_online_stuff(channel_t *chan)
{
	channel_member_t *m, *next_mem;

	for (m = chan->member_head; m; m = next_mem) {
		next_mem = m->next;
		uhost_cache_decref(m->nick);
		free_member(m);
	}
	chan->member_head = NULL;
	chan->nmembers = 0;
	chan->bot = NULL;

	free(chan->topic);
	chan->topic = NULL;

	free(chan->topic_nick);
	chan->topic_nick = NULL;

	chan->topic_time = 0;

	memset(&chan->mode, 0, sizeof(chan->mode));

	chan->limit = 0;

	free(chan->key);
	chan->key = NULL;

	clear_masklists_online_only(chan);

	free(chan->args);
	chan->args = NULL;
	chan->nargs = 0;
}

/* Free online related settings for all cahnnels */
static void free_all_online_stuff()
{
	channel_t *chan;

	/* Clear out channel list. */
	for (chan = channel_head; chan; chan = chan->next)
		free_channel_online_stuff(chan);

	/* And the uhost cache. */
	hash_table_walk(uhost_cache_ht, uhost_cache_delete, NULL);
	hash_table_delete(uhost_cache_ht);
	uhost_cache_ht = hash_table_create(NULL, NULL, UHOST_CACHE_SIZE, HASH_TABLE_STRINGS);
}

/* Completely destroy everything about the channel */
int destroy_channel_record(const char *chan_name)
{
	channel_t *chan, *prev;
	coupplet_t *cplt, *tmpcplt;
	chanstring_t *cstr, *tmpcstr;
	int i;

	if (chan_name) {
		channel_lookup(chan_name, 0, &chan, &prev);
		if (!chan)
			return -1;

		if (chan->status & CHANNEL_JOINED)
			printserv(SERVER_NORMAL, "PART %s\r\n", chan_name);

		if (prev)
			prev->next = chan->next;
		else
			channel_head = chan->next;
		nchannels--;

		free(chan->name);
		free_channel_online_stuff(chan);
	}
	else
		chan = &global_chan;

	for (i = 0; i < chan->nlists; i++) {
		clear_masklist(chan->lists[i]);
		free(chan->lists[i]);
	}
	free(chan->lists);

	for (cplt = chan->builtin_ints; cplt; cplt = tmpcplt) {
		tmpcplt = cplt->next;
		free(cplt->name);
		free(cplt);
	}

	for (cplt = chan->builtin_coupplets; cplt; cplt = tmpcplt) {
		tmpcplt = cplt->next;
		free(cplt->name);
		free(cplt);
	}

	for (cstr = chan->builtin_strings; cstr; cstr = tmpcstr) {
		tmpcstr = cstr->next;
		free(cstr->name);
		free(cstr->data);
		free(cstr);
	}

	if (chan != &global_chan)
		free(chan);
	else /* Just in case.. */
		memset(chan, 0, sizeof *chan);
	return 0;
}

/* Completely destroy all channel records */
static void destroy_all_channel_records()
{
	free_all_online_stuff();
	hash_table_delete(uhost_cache_ht);
	while (channel_head)
		destroy_channel_record(channel_head->name);
}

void server_channel_destroy()
{
	chanfile_save(server_config.chanfile);
	destroy_all_channel_records();
	destroy_channel_record(NULL); /* free any global channel stuff */
	bind_rem_simple(BTN_EVENT, NULL, "minutely", channels_minutely);
}

/* Find or create channel when passed 0 or 1 */
int channel_lookup(const char *chan_name, int create, channel_t **chanptr, channel_t **prevptr)
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
			return -create; /* Report error if we were supposed to
					   create new channel */
		}
		prev = chan;
	}
	if (!create) return -1;

	nchannels++;
	chan = calloc(1, sizeof(*chan));
	chan->name = strdup(chan_name);

	if (current_server.type1modes) {
		chan->nlists = strlen(current_server.type1modes);
		chan->lists = calloc(chan->nlists, sizeof(*chan->lists));
		for (i = 0; i < chan->nlists; i++) {
			chan->lists[i] = calloc(1, sizeof(*chan->lists[i]));
			chan->lists[i]->type = current_server.type1modes[i];
		}
	}

	/* FIXME - I suspect this code is buggy in the same way channel lists code was
			few weeks ago. It will recive attention later. */
	if (current_server.type2modes) {
		chan->nargs = strlen(current_server.type2modes);
		chan->args = calloc(chan->nargs, sizeof(*chan->args));
		for (i = 0; i < chan->nargs; i++) {
			chan->args[i].type = current_server.type2modes[i];
		}
	}

	chan->builtin_bools = global_chan.builtin_bools;
/* FIXME - set other members to their respective default values (if any) */
	chan->next = channel_head;
	channel_head = chan;
	*chanptr = chan;
	return 1;
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

/* Pass NULL as chan_name if you already have pointer to channel */
static channel_member_t *channel_add_member(const char *chan_name, channel_t *chan, const char *nick, const char *uhost)
{
	channel_member_t *m;

	if (chan_name)
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

	channel_lookup(chan_name, 0, &chan, NULL);

	if (chan && !(chan->builtin_bools & CHAN_INACTIVE)) {
		channel_member_t *m = channel_add_member(NULL, chan, nick, uhost);
		if (match_my_nick(nick)) {
			int i;
			chan->bot = m;
			chan->status |= (CHANNEL_WHOLIST | CHANNEL_JOINED);
			printserv(SERVER_NORMAL, "WHO %s\r\n", chan_name);
			printserv(SERVER_NORMAL, "MODE %s\r\n", chan_name);
			for (i = 0; i < chan->nlists; i++)
				chan->lists[i]->loading = 1;
			printserv(SERVER_NORMAL, "MODE %s %s\r\n", chan_name, current_server.type1modes);
		}
		else {
/* FIXME - Traverse ban list in search for closest ban */
		}
		return;
	}

	if (match_my_nick(nick)) {
		putlog(LOG_MISC, "*", _("Ooops, joined %s but didn't want to!"), chan_name);
		printserv(SERVER_NORMAL, "PART %s\r\n", chan_name);
	}
}

void channel_on_leave(const char *chan_name, const char *nick, const char *uhost, user_t *u)
{
	channel_t *chan;
	channel_member_t *prev = NULL, *m;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan)
		return;

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

	if (match_my_nick(nick))
		free_channel_online_stuff(chan);
}

void channel_on_quit(const char *nick, const char *uhost, user_t *u)
{
	channel_t *chan;

	if (match_my_nick(nick))
		free_all_online_stuff();
	else
		for (chan = channel_head; chan; chan = chan->next)
			channel_on_leave(chan->name, nick, uhost, u);
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
	channel_t *chan = NULL;
	chan_name = args[1];
	if (channel_lookup(chan_name, 0, &chan, NULL) == -1)
		return 0;
	nick = args[5];
	flags = args[6];
	uhost = egg_mprintf("%s@%s", args[2], args[3]);
	m = channel_add_member(NULL, chan, nick, uhost);
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
	channel_t *chan = NULL;

	chan_name = args[2];
	if (channel_lookup(chan_name, 0, &chan, NULL) == -1)
		return 0;

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
		m = channel_add_member(NULL, chan, nick, NULL);
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
		if (chan->lists[i]->type == type)
			return(chan->lists[i]);
	}
	return(NULL);
}

/* FIXME - int time should be long expire */
/* Creates or updates an entry in the mask list, as a result of channel mode change.
   It is *NOT* allowed to create new list type because, when connected to server,
   we already know all valid types or use defaults. */
void channel_irc_add_mask(channel_t *chan, char type, const char *mask, const char *set_by, int time)
{
	channel_mask_list_t *l;
	channel_mask_t *m;

	l = channel_get_mask_list(chan, type);

	if (!l)
		return;

	for (m = l->head; m; m = m->next)
		if (!(current_server.strcmp)(m->mask, mask))
			break;

	if (!m) {
		m = calloc(1, sizeof(*m));
		m->mask = strdup(mask);
		m->next = l->head;
		l->head = m;
		l->len++;
	}
	if (set_by) m->set_by = strdup(set_by);
	m->time = time;
	m->last_used = time;
}

/* FIXME - int expire should be long expire (EGGTIMEVALT) */
/* Creates or updates an entry in the mask list, as a result of console command, script, other.. */
/* It *WILL* create a new list type if need be */
int channel_notirc_add_mask(channel_t *chan, char type, const char *mask, const char *creator,
				const char *comment, int expire, int lastused, int sticky, int console)
{
	channel_mask_list_t *l;
	channel_mask_t *m;

	if (!chan) /* Global mask */
		chan = &global_chan;

	l = channel_get_mask_list(chan, type);
	if (!l) {
		void *tmpp = realloc(chan->lists, (chan->nlists+1) * sizeof(*chan->lists));
		if (!tmpp)
			return 1;
		chan->lists = tmpp;
		l = calloc(1, sizeof *l);
		chan->lists[chan->nlists++] = l;
		l->type = type;
	}

	for (m = l->head; m; m = m->next)
		/* Here we do not use wild matching, so to allow for masks to be created
		   even if wider covering mask already exists. Idea is to be able to set
		   only the narrowest mask, once the need arises. */
		if (current_server.strcmp) {
			if (!(current_server.strcmp)(m->mask, mask))
				break;
		}
		else {
			if (!strcasecmp(m->mask, mask))
				break;
		}

	if (m) {
		if (m->creator)
			return 2;
	}
	else {
		m = calloc(1, sizeof(*m));
		m->mask = strdup(mask);
	}

	m->creator = strdup(creator);
	m->comment = strdup(comment?comment:"");
	m->expire = expire;
	m->last_used = lastused;
	m->sticky = sticky;
	m->next = l->head;
	l->head = m;
	l->len++;
/* FIXME - check for enforcebans and kick if needed. Or maybe we should kick anyway?
		Or maybe only kick if it was console .+ban?
		Perhaps best, yet, is to create a separate function to traverse member list
		and deal with this in appropriate way. We'll need it for other things too.
		For now, let it just dump mode to channel */
	if (chan == &global_chan)
		for (chan = channel_head; chan; chan = chan->next) {
			if (BOT_CAN_SET_MODES(chan)) {
				printserv(SERVER_QUICK, "MODE %s +%c %s\r\n", chan->name, type, mask);
			if (type == 'b' && console)
				; /* Er, nothing for now */
		}
	}
	else {
		if (BOT_CAN_SET_MODES(chan)) {
			printserv(SERVER_QUICK, "MODE %s +%c %s\r\n", chan->name, type, mask);
			if (type == 'b' && console)
				;
		}
	}

	return 0;
}

/* This is not code i feel proud of. It should be written in the cleaner manner but let it be for now */
int channel_del_mask(channel_t *chan, char type, const char *mask, int remove)
{
	channel_mask_list_t *l;
	channel_mask_t *m, *prev;

	if (!chan)
		chan = &global_chan;

	l = channel_get_mask_list(chan, type);
	if (!l) return -1;

	prev = NULL;
	for (m = l->head; m; m = m->next) {
		if (!(current_server.strcmp)(m->mask, mask)) break;
		prev = m;
	}
	if (!m) return -2;

	if (chan == &global_chan) {
		if (prev) prev->next = m->next;
		else l->head = m->next;
		l->len--;
		free(m->mask);
		free(m->creator);
		free(m->comment);
		free(m);
		for (chan = channel_head; chan; chan = chan->next) {
			if (BOT_CAN_SET_MODES(chan))
				printserv(SERVER_QUICK, "MODE %s -%c %s\r\n", chan->name, type, mask);
		}
	}
	else if (remove) { /* We were called from p-line or script/module */
		if (BOT_CAN_SET_MODES(chan) && m->set_by)
			printserv(SERVER_QUICK, "MODE %s -%c %s\r\n", chan->name, type, mask);
		free(m->creator);
		free(m->comment);
		if (!m->set_by) {
			if (prev) prev->next = m->next;
			else l->head = m->next;
			l->len--;
			free(m->mask);
			free(m);
		}
		else { /* We're not deleting the node so clean up */
			m->creator = NULL;
			m->comment = NULL;
		}
	}
	else { /* Function was called as a result of recived IRC message MODE */
		free(m->set_by);
		if (!m->creator) {
			if (prev) prev->next = m->next;
			else l->head = m->next;
			l->len--;
			free(m->mask);
			free(m); /* Rest should've been freed already */
		}
		else {
			m->set_by = NULL;
			m->time = 0;
		}
	}

	return 0;
}

static void clear_masklists_online_only(channel_t *chanptr)
{
	channel_mask_t *m, *prev;
	int i;

	if (!chanptr)
		return;

	for (i = 0; i < chanptr->nlists; i++) {
		prev = NULL;
		for (m = chanptr->lists[i]->head; m; prev = m) {
			if (!m->creator) {
				if (m == chanptr->lists[i]->head)
					chanptr->lists[i]->head = m->next;
				else
					prev = m->next;
				free(m->set_by);
				free(m);
				chanptr->lists[i]->len--;
			}
			else
				m = m->next;
		}
	}
}

static void clear_masklist(channel_mask_list_t *l)
{
	channel_mask_t *m, *next;

	for (m = l->head; m; m = next) {
		next = m->next;
		free(m->mask);
		free(m->creator);
		free(m->set_by);
		free(m->comment);
		free(m);
	}
	l->head = NULL;
}

void channel_clear_masks(channel_t *chan, char type)
{
	channel_mask_list_t *l;

	l = channel_get_mask_list(chan, type);
	if (l) clear_masklist(l);
}

/* Returns a number of channel_mask_t nodes stored in *cm */
int channel_list_masks(channel_mask_t ***cm, char type, channel_t *chanptr, const char *mask)
{
	channel_mask_list_t *l;
	channel_mask_t *m;
	int nmasks = 0;

	if (chanptr == NULL)
		chanptr = &global_chan;

	l = channel_get_mask_list(chanptr, type);
	if (!l)
		return 0;

	for (m = l->head; m; m = m->next)
		if (wild_match(mask, m->mask)) {
			void *tmpp = realloc(*cm, (nmasks+1)*(sizeof cm));
			if (!tmpp)
				return nmasks;
			*cm = tmpp;

			(*cm)[nmasks++] = m;
		}

	return nmasks;
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
	channel_irc_add_mask(chan, type, ban, (nargs > 3) ? args[3] : NULL, (nargs > 4) ? atoi(args[4]) : -1);
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

	channel_lookup(chan_name, 0, &chan, NULL);
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
			if (changestr[0] == '+') channel_irc_add_mask(chan, *change, arg, from_nick, timer_get_now_sec(NULL));
			else channel_del_mask(chan, *change, arg, 0);
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

static int channel_set_bool(channel_t *chanptr, const char *setting)
{
	int i;
	int fakenchannels;
	channel_t *fakechanptr;

	for (i = 0; i < 32; i++)
		if (!strcasecmp(&setting[1], channel_builtin_bools[i]))
		   break;
	if (i == 32)
		return 0;

	if (!chanptr) { /* Set this for all channels? */
		fakenchannels = nchannels;
		fakechanptr = channel_head;
	}
	else {
		fakenchannels = 1;
		fakechanptr = chanptr;
	}

	while (fakenchannels--) {
		if (*setting == '+')
			fakechanptr->builtin_bools |= (1 << i);
		else
			fakechanptr->builtin_bools &= ~(1 << i);

		if (i == 0) /* +/-inactive */
			printserv(SERVER_NORMAL, "%s %s %s\r\n", *setting=='+'?"PART":"JOIN",
				fakechanptr->name, fakechanptr->key?fakechanptr->key:"");

		fakechanptr = fakechanptr->next;
	}

	return 1;
}

static int channel_set_coupplet(coupplet_t *cplt, const char *value)
{
	unsigned long left, right;
	char *endptr = NULL;
	char *r, *l;

	l = strdup(value);
	if ((r = strchr(l, ':')) == NULL) {
		free(l);
		return 0;
	}
	*r++ = '\0';

	left = strtoul(l, &endptr, 10);

	if (!*l || *endptr) {
		free(l);
		return 0;
	}

	right = strtoul (r, &endptr, 10);

	if (!*r || *endptr) {
		free(l);
		return 0;
	}

	cplt->left = left;
	cplt->right = right;
	free(l);
	return 1;
}

static int channel_set_int(coupplet_t *cplt, const char *value)
{
	char *endptr = NULL;
	unsigned long left;

	left = strtoul(value, &endptr, 10);

	if (*value && !*endptr) {
		cplt->left = left;
		return 1;
	}

	return 0;
}

int channel_set(channel_t *chanptr, const char *setting, const char *value)
{
	coupplet_t *cplt;
	chanstring_t *cstr = NULL;
	int i;

	/* Is it one of the flags? (boolean, on|off) */
	if (*setting == '+' || *setting == '-')
		return channel_set_bool(chanptr, setting);

	/* It isn't, so let's check if it's a numeric setting of some sort. An int? */
	for (cplt = chanptr->builtin_ints; cplt; cplt = cplt->next)
		if (strcasecmp(cplt->name, setting) == 0)
			return channel_set_int(cplt, value);

	/* Or an int that is being set for the first time? */
	for (i = 0; channel_builtin_ints[i]; i++)
		if (strcasecmp(channel_builtin_ints[i], setting) == 0) {
			/* Ok, it is - add record for it */
			cplt = malloc(sizeof *cplt);
			if (!channel_set_int(cplt, value)) {
				free(cplt);
				return 0;
			}
			cplt->name = strdup(channel_builtin_ints[i]);
			if (chanptr->builtin_ints)
				cplt->next = chanptr->builtin_ints;
			else
				cplt->next = NULL;
			chanptr->builtin_ints = cplt;
			return 1;
		}

	/* Maybe it's a coupplet: <setting> X:Y */
	for (cplt = chanptr->builtin_coupplets; cplt; cplt = cplt->next)
		if (strcasecmp(cplt->name, setting) == 0)
			return channel_set_coupplet(cplt, value);

	/* No luck for now. Is it a valid coupplet at all? */
	for (i = 0; channel_builtin_coupplets[i]; i++)
		if (strcasecmp(channel_builtin_coupplets[i], setting) == 0) {
			/* Ah, it is - add record for it */
			cplt = malloc(sizeof *cplt);
			if (!channel_set_coupplet(cplt, value)) {
				free(cplt);
				return 0;
			}
			cplt->name = strdup(channel_builtin_coupplets[i]);
			if (chanptr->builtin_coupplets)
				cplt->next = chanptr->builtin_coupplets;
			else
				cplt->next = NULL;
			chanptr->builtin_coupplets = cplt;
			return 1;
		}

	/* Well, maybe it's one of the 'string' settings? */
	for (cstr = chanptr->builtin_strings; cstr; cstr = cstr->next)
		if (strcasecmp(cstr->name, setting) == 0) {
			free(cstr->data);
			cstr->data = strdup(value);
			return 1;
		}

	/* Or one of the 'string' settings that just wasn't used till now? */
	for (i = 0; channel_builtin_strings[i]; i++)
		if (strcasecmp(channel_builtin_strings[i], setting) == 0) {
			/* It was - add record for it */
			cstr = malloc(sizeof *cstr);
			cstr->data = strdup(value);
			cstr->name = strdup(channel_builtin_strings[i]);
			if (chanptr->builtin_strings)
				cstr->next = chanptr->builtin_strings;
			else
				cstr->next = NULL;
			chanptr->builtin_strings = cstr;
			return 1;
		}

	/* Lastly, it can be one of user defined settings */
/* FIXME - will be done in near future, once more important things are ready */

	return 0;
}

int channel_info(partymember_t *p, const char *channame)
{
	static char hack_gettext[64]; /* Seems like gettext() messes %lu foramt */
	channel_t *chan;
	int i;
	const int hgl = sizeof(hack_gettext) - 1;

	if (channel_lookup(channame, 0, &chan, NULL) == -1) {
		partymember_printf(p, _("Error: Invalid channel '%s'"), channame);
		return BIND_RET_BREAK;
	}

	partymember_printf(p, _("Information for channel '%s'"), channame);

	if (chan->builtin_bools) {
		partymember_printf(p, _("  Flags:"));
		for (i = 0; i < 32; i++)
			if (chan->builtin_bools & (1 << i))
				partymember_printf(p, _("    +%s"), channel_builtin_bools[i]);
	}

	if (chan->builtin_ints) {
		coupplet_t *cplt;
		partymember_printf(p, _("  Settings:"));
		for (cplt = chan->builtin_ints; cplt; cplt = cplt->next) {
			snprintf(hack_gettext, hgl, "%lu", cplt->left);
			hack_gettext[hgl] = '\0';
			partymember_printf(p, _("    %s:\t%s"), cplt->name, hack_gettext);
		}
	}
	if (chan->builtin_coupplets) {
		coupplet_t *cplt;
		if (!chan->builtin_ints)
			partymember_printf(p, _("  Settings:"));
		for (cplt = chan->builtin_coupplets; cplt; cplt = cplt->next) {
			snprintf(hack_gettext, hgl, "%lu:%lu", cplt->left, cplt->right);
			hack_gettext[hgl] = '\0';
			partymember_printf(p, _("    %s:\t%s"), cplt->name, hack_gettext);
		}
	}

	if (chan->builtin_strings) {
		chanstring_t *cstr;
		if (!chan->builtin_coupplets && !chan->builtin_ints)
			partymember_printf(p, _("  Settings:"));
		for (cstr = chan->builtin_strings; cstr; cstr = cstr->next)
			partymember_printf(p, _("    %s:\t%s"), cstr->name, cstr->data);
	}

	return BIND_RET_LOG;
}

static int chanfile_save_channel(xml_node_t *root, channel_t *chan)
{
	xml_node_t *chan_node;
	coupplet_t *cplt;
	chanstring_t *cstr;
	static char gettext_hack[64];
	const int hgl = sizeof(gettext_hack) - 1;

	int i, j;

	chan_node = xml_node_new();

	if (chan->name) {
		chan_node->name = strdup("channel");
		xml_node_set_str(chan->name, chan_node, "name", 0, 0);
	}
	else
		chan_node->name = strdup("global");

	for (i = j = 0; i < 32; i++) {
		if (*channel_builtin_bools == '\0')
			continue;
		if (chan->builtin_bools & (1 << i))
			xml_node_set_str(channel_builtin_bools[i], chan_node, "flag", j++, 0);
	}

	i = 0;
	for (cplt = chan->builtin_ints; cplt ; cplt = cplt->next) {
		xml_node_set_str(cplt->name, chan_node, "setting", i, "name", 0, 0);
		snprintf(gettext_hack, sizeof gettext_hack, "%lu", cplt->left);
		gettext_hack[hgl] = '\0';
		xml_node_set_str(gettext_hack, chan_node, "setting", i++, "data", 0, 0);
	}

	for (cplt = chan->builtin_coupplets; cplt ; cplt = cplt->next) {
		xml_node_set_str(cplt->name, chan_node, "setting", i, "name", 0, 0);
		snprintf(gettext_hack, sizeof gettext_hack, "%lu:%lu", cplt->left, cplt->right);
		gettext_hack[hgl] = '\0';
		xml_node_set_str(gettext_hack, chan_node, "setting", i++, "data", 0, 0);
	}

	for (cstr = chan->builtin_strings; cstr ; cstr = cstr->next) {
		xml_node_set_str(cstr->name, chan_node, "setting", i, "name", 0, 0);
		xml_node_set_str(cstr->data, chan_node, "setting", i++, "data", 0, 0);
	}

	j = 0;
	for (i = 0; i < chan->nlists; i++) {
		channel_mask_t *m;
		char type[2] = " ";
		type[0] = chan->lists[i]->type;
		for (m = chan->lists[i]->head; m; m = m->next) {
			xml_node_set_str(type, chan_node, "chanmask", j, "type", 0, 0);
			xml_node_set_str(m->mask, chan_node, "chanmask", j, "mask", 0, 0);
			xml_node_set_str(m->creator, chan_node, "chanmask", j, "creator", 0, 0);
			xml_node_set_str(m->comment, chan_node, "chanmask", j, "comment", 0, 0);
			/* FIXME - This should be long integer */
			xml_node_set_int(m->expire, chan_node, "chanmask", j, "expire", 0, 0);
			xml_node_set_int(m->last_used, chan_node, "chanmask", j, "lastused", 0, 0);
			xml_node_set_int(m->sticky, chan_node, "chanmask", j++, "sticky", 0, 0);
		}
	}

	xml_node_append(root, chan_node);
/* FIXME - Do we need to free() here? Appears not.
	free(chan_node); */
	return 0;
}

int chanfile_save(const char *fname)
{
	xml_node_t *root;
	channel_t *chan;
	int i;

	root = xml_node_new();
	root->name = strdup("channels");

	chanfile_save_channel(root, &global_chan);

	for (chan = channel_head; chan; chan = chan->next)
		chanfile_save_channel(root, chan);

	if (!fname)
		fname = server_config.chanfile;

	i = xml_save_file(fname, root, XML_INDENT);
	xml_node_delete(root);

	if (i == 0)
		putlog(LOG_MISC, "*", _("Saving channels file '%s'"), fname);
	return i;
}

static int chanfile_load(const char *fname)
{
	int i;
	xml_node_t *doc, *root, *chan_node, *setting_node;
	channel_t *chan = NULL;
	char *tmpptr, *tmp2ptr;
	/* FIXME - storage size of cursecs is wrong */
	int cursecs;

	if (xml_load_file(fname?fname:"channels.xml", &doc, XML_TRIM_TEXT) != 0) {
		putlog(LOG_MISC, "*", _("Failed to load channel file '%s': %s"), fname, xml_last_error());
		return -1;
	}

	putlog(LOG_MISC, "*", _("Loading channels file '%s'"), fname?fname:"channels.xml");

	root = xml_root_element(doc);

	for (i = 0; i < root->nchildren; i++) {
		int j;
		chan_node = root->children[i];
		if (!strcasecmp(chan_node->name, "channel")) {
			xml_node_get_str(&tmpptr, chan_node, "name", 0, 0);
/* FIXME - Maybe we should allow for already existing channels,
		in which case we should just update the existing data */
			if (!tmpptr || channel_lookup(tmpptr, 1, &chan, NULL) == -1)
				continue;
			chan->name = strdup(tmpptr);
		}
		else if (!strcasecmp(chan_node->name, "global")) {
			chan = &global_chan;
			chan->name = NULL;
		}
		else
			continue;

		j = 0;
		while (xml_node_get_str(&tmpptr, chan_node, "flag", j++, 0) != -1) {
			int k;
			if (j > 31)
				continue;
			for (k = 0; k < 32; k++)
				if (!strcasecmp(tmpptr, channel_builtin_bools[k])) {
					chan->builtin_bools |= (1 << k);
					break;
				}
		}

		j = 0;
		while ((setting_node = xml_node_lookup(chan_node, 0, "setting", j++, 0))) {
			if (xml_node_get_str(&tmpptr, setting_node, "name", 0, 0) == -1)
				continue;
			if (xml_node_get_str(&tmp2ptr, setting_node, "data", 0, 0) == -1)
				continue;
			channel_set(chan, tmpptr, tmp2ptr);
		}

		j = 0;
		timer_get_now_sec(&cursecs);
		while ((setting_node = xml_node_lookup(chan_node, 0, "chanmask", j++, 0))) {
			char *type, *mask, *creator, *comment;
			/* FIXME - 'expire' should be long */
			int expire, lastused, sticky;
			if ((xml_node_get_int(&expire, setting_node, "expire", 0, 0) == -1) ||
			    (cursecs > expire))
				continue;
			if (xml_node_get_str(&type, setting_node, "type", 0, 0) == -1)
				continue;
			if (xml_node_get_str(&mask, setting_node, "mask", 0, 0) == -1)
				continue;
			if (xml_node_get_str(&creator, setting_node, "creator", 0, 0) == -1)
				continue;
			if (xml_node_get_str(&comment, setting_node, "comment", 0, 0) == -1)
				continue;
			if (xml_node_get_int(&lastused, setting_node, "lastused", 0, 0) == -1)
				continue;
			if (xml_node_get_int(&sticky, setting_node, "sticky", 0, 0) == -1)
				continue;
			channel_notirc_add_mask(chan, *type, mask, creator, comment, expire, lastused, sticky, 0);
		}
	}

	xml_node_delete(doc);
	return 0;
}

/* Updates info about various channel aspects, once we recive such info */
void update_channel_structures()
{
	channel_t *chan;
	int i;

	for (chan = channel_head; chan; chan = chan->next) {

		if (current_server.type1modes && chan->nlists == 0) {
			chan->nlists = strlen(current_server.type1modes);
			chan->lists = calloc(chan->nlists, sizeof(*chan->lists));
			for (i = 0; i < chan->nlists; i++) {
				chan->lists[i] = calloc(1, sizeof *chan->lists[i]);
				chan->lists[i]->type = current_server.type1modes[i];
			}
		}

		if (current_server.type2modes && chan->nargs == 0) {
			chan->nargs = strlen(current_server.type2modes);
			chan->args = calloc(chan->nargs, sizeof(*chan->args));
			for (i = 0; i < chan->nargs; i++) {
				chan->args[i].type = current_server.type2modes[i];
			}
		}
	}
}

/* Do the stuff like need-* or cycle.. */
static int channels_minutely()
{
	channel_mask_t *m;
	channel_t *chan;
	/* FIXME - storage size of cursecs is wrong */
	int cursecs = 0;
	int i;

	timer_get_now_sec(&cursecs);

	/* Expire global masks */
	for (i = 0; i < global_chan.nlists; i++)
		for (m = (&global_chan)->lists[i]->head; m; m = m->next)
			if (m->expire && cursecs > m->expire) {
				char *tmpmask = strdup(m->mask);
				putlog(LOG_MISC, "*", _("Expiring global +%c list item '%s'"),
							(&global_chan)->lists[i]->type, tmpmask);
				channel_del_mask(NULL, (&global_chan)->lists[i]->type, tmpmask, 1);
				for (chan = channel_head; chan; chan = chan->next)
					channel_del_mask(chan, (&global_chan)->lists[i]->type, tmpmask, 1);
				free(tmpmask);
			}

	if (!current_server.registered)
		return 0;

	/* Stuff to do only if we're connected */
	for (chan = channel_head; chan; chan = chan->next) {
		if (chan->status & CHANNEL_JOINED && /* If we actaully are on channel */
		   !(chan->builtin_bools & CHAN_INACTIVE)) { /* and channel is not inactive */
			/* +cycle */
			if (chan->builtin_bools & CHAN_CYCLE &&/* Channel is +cycle */
			   chan->nmembers == 1 && /* and we are alone */
			   !BOT_CAN_SET_MODES(chan) /* and we are not op/halfop */
			   ) {
				putlog(LOG_MISC, "*", _("Cycling channel '%s' for ops."), chan->name);
				printserv(SERVER_NORMAL, "PART %s\r\n", chan->name);
				printserv(SERVER_NORMAL, "JOIN %s %s\r\n", chan->name,
						chan->key?chan->key:"");
			}
			/* expire channel masks */
			for (i = 0; i < chan->nlists; i++)
				for (m = chan->lists[i]->head; m; m = m->next)
					if (m->expire && cursecs > m->expire) {
						putlog(LOG_MISC, "*", _("Expiring %s +%c list item '%s'"),
								chan->name, chan->lists[i]->type, m->mask);
						channel_del_mask(chan, chan->lists[i]->type, m->mask, 1);
					}
		}
	}
	return 0;
}

/* Check if user has a given flag on channel pointed to by channel_t pointer */
static int chanptr_member_has_flag(const char *nick, channel_t *chan, unsigned char c)
{
	channel_member_t *m;

	if (!chan || !(chan->status & CHANNEL_JOINED))
		return 0;

	for (m = chan->member_head; m; m = m->next)
		if (!(current_server.strcmp)(m->nick, nick))
			return flag_match_single_char(&m->mode, c);

	return 0;
}

/* Check if user has a given flag on channel with given name */
int channame_member_has_flag(const char *nick, const char *channame, unsigned char c)
{
	channel_t *chan = NULL;

	if (channel_lookup(channame, 0, &chan, NULL) == -1)
		return 0;
	else
		return chanptr_member_has_flag(nick, chan, c);
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

/* Do not change the order of these.
   Do not remove setting, simply change it to empty string ""
   Do not add, there can only be 32 such settings */

static const char *channel_builtin_bools[32] = { /* DDD */
	"inactive", "cycle", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", ""
	};

/* TBIL = To Be Implemented Later */

static const char *channel_builtin_ints[] = {
	"idle-kick", /* TBIL */
	"ban-time", /* TBIL */
	"invite-time", /* TBIL */
	"exempt-time", /* TBIL */
	NULL
};

static const char *channel_builtin_coupplets[] = {
	"automode-delay", /* TBIL */
	NULL
};

static const char *channel_builtin_strings[] = {
	"need-op", /* TBIL */
	"need-unban", /* TBIL */
	"need-key", /* TBIL */
	"need-invite", /* TBIL */
	NULL
};
