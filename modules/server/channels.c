/* Some channel-specific stuff. */

#include <eggdrop/eggdrop.h>
#include <eggdrop/module.h>
#include <ctype.h>
#include "server.h"
#include "binds.h"
#include "channels.h"
#include "output.h"

#define UHOST_CACHE_SIZE	47

channel_t *channel_head = NULL;
int nchannels = 0;

hash_table_t *uhost_cache_ht = NULL;

static bind_list_t channel_raw_binds[];

/* Prototypes. */
static int uhost_cache_delete(const void *key, void *data, void *param);
static void clear_banlist(channel_t *chan);

void server_channel_init()
{
	channel_head = NULL;
	nchannels = 0;
	uhost_cache_ht = hash_table_create(NULL, NULL, UHOST_CACHE_SIZE, HASH_TABLE_STRINGS);
	bind_add_list("raw", channel_raw_binds);
}

void server_channel_destroy()
{
	channel_t *chan, *next_chan;
	channel_member_t *m, *next_mem;

	/* Clear out channel lists. */
	for (chan = channel_head; chan; chan = next_chan) {
		next_chan = chan->next;
		for (m = chan->member_head; m; m = next_mem) {
			next_mem = m->next;
			free(m->nick);
			free(m->uhost);
			free(m);
		}
		free(chan->name);
		if (chan->topic) free(chan->topic);
		if (chan->topic_nick) free(chan->topic_nick);
		if (chan->key) free(chan->key);
		clear_banlist(chan);
		free(chan);
	}

	/* And the uhost cache. */
	hash_table_walk(uhost_cache_ht, uhost_cache_delete, NULL);
	hash_table_destroy(uhost_cache_ht);
}

void channel_lookup(const char *chan_name, int create, channel_t **chanptr, channel_t **prevptr)
{
	channel_t *chan, *prev;

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
	chan->next = channel_head;
	channel_head = chan;
	*chanptr = chan;
	if (prevptr) *prevptr = NULL;
}

static void make_lowercase(char *nick)
{
	while (*nick) {
		*nick = tolower(*nick);
		nick++;
	}
}

static int uhost_cache_delete(const void *key, void *data, void *param)
{
	uhost_cache_entry_t *cache = data;

	free(cache->nick);
	free(cache->uhost);
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

void uhost_cache_fillin(const char *nick, const char *uhost, int addref)
{
	char buf[64], *lnick;
	uhost_cache_entry_t *cache = NULL;

	lnick = egg_msprintf(buf, sizeof(buf), NULL, "%s", nick);
	make_lowercase(lnick);
	hash_table_find(uhost_cache_ht, lnick, &cache);
	if (!cache) {
		cache = malloc(sizeof(*cache));
		cache->nick = strdup(nick);
		make_lowercase(cache->nick);
		cache->uhost = strdup(uhost);
		cache->ref_count = 1;
		hash_table_insert(uhost_cache_ht, cache->nick, cache);
	}
	else if (addref) {
		cache->ref_count++;
	}
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
		hash_table_delete(uhost_cache_ht, cache->nick, NULL);
		uhost_cache_delete(NULL, cache, NULL);
	}
}

void channel_add_member(const char *chan_name, const char *nick, const char *uhost)
{
	channel_t *chan;
	channel_member_t *m;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan) return;

	/* See if this member is already added. */
	for (m = chan->member_head; m; m = m->next) {
		if (!(current_server.strcmp)(m->nick, nick)) return;
	}

	/* Nope, so add him and put him in the cache. */
	uhost_cache_fillin(nick, uhost, 1);

	m = calloc(1, sizeof(*m));
	m->nick = strdup(nick);
	m->uhost = strdup(uhost);
	m->join_time = egg_timeval_now.sec;

	m->next = chan->member_head;
	chan->member_head = m;
}

void channel_on_join(const char *chan_name, const char *nick, const char *uhost)
{
	channel_t *chan;
	int create;

	if (match_my_nick(nick)) create = 1;
	else create = 0;

	channel_lookup(chan_name, create, &chan, NULL);
	if (!chan) return;

	if (create) {
		chan->status |= CHANNEL_WHOLIST | CHANNEL_BANLIST;
		printserv(SERVER_NORMAL, "WHO %s\r\n", chan_name);
		printserv(SERVER_NORMAL, "MODE %s +b\r\n", chan_name);
	}

	channel_add_member(chan_name, nick, uhost);
}

static void real_channel_leave(channel_t *chan, const char *nick, const char *uhost, user_t *u)
{
	channel_member_t *m;
	char *chan_name;

	if (match_my_nick(nick)) {
		channel_member_t *next;

		chan_name = strdup(chan->name);
		for (m = chan->member_head; m; m = next) {
			next = m->next;
			uhost_cache_decref(m->nick);
			free(m->nick);
			free(m->uhost);
			free(m);
		}
		if (chan->topic) free(chan->topic);
		if (chan->topic_nick) free(chan->topic_nick);
		if (chan->key) free(chan->key);
		free(chan->name);
		free(chan);
		nchannels--;
	}
	else {
		channel_member_t *prev;

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

		free(m->nick);
		free(m->uhost);
		free(m);
	}

	bind_check(BT_leave, chan_name, nick, uhost, u, chan_name);

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
	channel_t *chan, *next;

	if (match_my_nick(nick)) {
		for (chan = channel_head; chan; chan = next) {
			next = chan->next;
			real_channel_leave(chan, nick, uhost, u);
		}
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
	hash_table_delete(uhost_cache_ht, lnick, &cache);
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

void channel_list(char ***chans, int *nchans)
{
	channel_t *chan;
	int i;

	*chans = calloc(nchannels+1, sizeof(char **));
	*nchans = nchannels;
	i = 0;
	for (chan = channel_head; chan; chan = chan->next) {
		(*chans)[i] = chan->name;
		i++;
	}
}

/* :server 352 <ournick> <chan> <user> <host> <server> <nick> <H|G>[*][@|+] :<hops> <name> */
static int got352(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name, *nick, *uhost;

	chan_name = args[1];
	nick = args[5];
	uhost = egg_mprintf("%s@%s", args[2], args[3]);
	channel_add_member(chan_name, nick, uhost);
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
 * Ban handling
 */

static void add_ban(channel_t *chan, const char *mask, const char *set_by, int time)
{
	channel_mask_t *m;

	m = calloc(1, sizeof(*m));
	m->mask = strdup(mask);
	if (set_by) m->set_by = strdup(set_by);
	m->time = time;
	m->next = chan->ban_head;
	chan->ban_head = m;
}

static void clear_banlist(channel_t *chan)
{
	channel_mask_t *m, *next;

	for (m = chan->ban_head; m; m = next) {
		next = m->next;
		free(m->mask);
		if (m->set_by) free(m->set_by);
		free(m);
	}
	chan->ban_head = NULL;
}

/* :server 367 <ournick> <chan> <ban> [<nick> <time>] */
static int got367(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[1];
	char *ban = args[2];
	channel_t *chan;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan || !(chan->status & CHANNEL_BANLIST)) return(0);
	add_ban(chan, ban, (nargs > 3) ? args[3] : NULL, (nargs > 4) ? atoi(args[4]) : -1);
	return(0);
}

/* :server 368 <ournick> <chan> :End of channel ban list */
static int got368(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan_name = args[1];
	channel_t *chan;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan) return(0);
	chan->status &= ~CHANNEL_BANLIST;
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
	bind_check(BT_join, chan, from_nick, from_uhost, u, chan);
	return(0);
}

/* Got a part message. */
static int gotpart(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *chan = args[0];
	char *text = args[1];

	channel_on_leave(chan, from_nick, from_uhost, u);
	bind_check(BT_part, chan, from_nick, from_uhost, u, chan, text);
	return(0);
}

/* Got a quit message. */
static int gotquit(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *text = args[0];

	channel_on_quit(from_nick, from_uhost, u);
	bind_check(BT_quit, from_nick, from_nick, from_uhost, u, text);

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
	bind_check(BT_kick, from_nick, from_nick, from_uhost, u, chan, victim, text);

	return(0);
}

/* Got nick change.  */
static int gotnick(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *newnick = args[0];

	channel_on_nick(from_nick, newnick);
	bind_check(BT_nick, from_nick, from_nick, from_uhost, u, newnick);

	/* Is it our nick that's changing? */
	if (match_my_nick(from_nick)) str_redup(&current_server.nick, newnick);

	return(0);
}

/*************************************************
 * Mode stuff.
 */

/* Got a mode change. */
static int gotmode(char *from_nick, char *from_uhost, user_t *u, char *cmd, int nargs, char *args[])
{
	char *dest = args[0];
	char *change = args[1];
	char *arg;
	char changestr[3];
	int hasarg, curarg;
	channel_t *chan;

	/* Is it a user mode? */
	if (!strchr(current_server.chantypes, *dest)) {
		/* Not interested right now. */
		return(0);
	}

	/* Make sure it's a valid channel. */
	channel_lookup(dest, 0, &chan, NULL);
	if (!chan) return(0);

	hasarg = 0;
	curarg = 2;
	changestr[0] = '+';
	changestr[2] = 0;
	while (*change) {
		/* Direction? */
		if (*change == '+' || *change == '-') {
			changestr[0] = *change;
			change++;
			continue;
		}

		/* Figure out if it takes an argument. */
		if (strchr(current_server.modeprefix, *change) || strchr(current_server.type1modes, *change) || strchr(current_server.type2modes, *change)) {
			hasarg = 1;
		}
		else if (strchr(current_server.type3modes, *change)) {
			if (changestr[0] == '+') hasarg = 1;
			else hasarg = 0;
		}
		else {
			hasarg = 0;
		}

		if (hasarg) arg = args[curarg++];
		else arg = NULL;

		changestr[1] = *change;

		bind_check(BT_mode, changestr, from_nick, from_uhost, u, dest, changestr, arg);
		change++;
	}
	return(0);
}

static bind_list_t channel_raw_binds[] = {
	/* WHO replies */
	{"352", got352}, /* WHO reply */
	{"315", got315}, /* end of WHO */

	/* Topic info */
	{"331", got331},
	{"332", got332},
	{"333", got333},

	/* Bans. */
	{"367", got367},
	{"368", got368},

	/* Channel member stuff. */
	{"JOIN", gotjoin},
	{"PART", gotpart},
	{"QUIT", gotquit},
	{"KICK", gotkick},
	{"NICK", gotnick},

	/* Mode. */
	{"MODE", gotmode},

	{NULL, NULL}
};
