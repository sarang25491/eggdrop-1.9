/* Some channel-specific stuff. */

#include <eggdrop/eggdrop.h>
#include <eggdrop/module.h>
#include "server.h"

typedef struct {
	char *nick;
	char *uhost;
	int ref;
} nick_uhost_entry_t;

typedef struct channel_member {
	struct channel_member *next;

	char *nick;
	char *uhost;
	int join_time;
	int mode;
} channel_member_t;

typedef struct channel {
	struct channel *next;

	char *name;

	char *topic, *topic_nick;	/* Topic and who set it. */
	int topic_time;	/* When it was set. */

	int mode;	/* Mode bits. */
	int limit;	/* Channel limit. */
	char *key;	/* Channel key. */

	int nmembers;
	channel_member_t *member_head;
} channel_t;

static channel_t *channel_head = NULL;
static int nchannels;

static channel_t *channel_lookup(const char *chan_name, int create)
{
	channel_t *chan;

	for (chan = channel_head; chan; chan = chan->next) {
		if (!strcasecmp(chan->name, chan_name)) return(chan);
	}
	if (!create) return(NULL);

	chan = calloc(1, sizeof(*chan));
	chan->name = strdup(chan_name);
	return(chan);
}

void channels_join_all()
{
}

void channel_on_join(const char *chan_name, const char *nick, const char *uhost)
{
	channel_t *chan;
	channel_member_t *m;
	int create;

	if (match_my_nick(nick)) create = 1;
	else create = 0;

	chan = channel_lookup(chan_name, create);
	if (!chan) return;

	m = calloc(1, sizeof(*m));
	m->nick = strdup(nick);
	m->uhost = strdup(uhost);
	m->join_time = egg_timeval_now.sec;

	m->next = chan->member_head;
	chan->member_head = m;
}

void channel_on_leave(const char *chan_name, const char *nick, const char *uhost)
{
	channel_t *chan;
	channel_member_t *m;

	chan = channel_lookup(chan_name, 0);
	if (!chan) return;

	if (match_my_nick(nick)) {
		channel_member_t *next;

		for (m = chan->member_head; m; m = next) {
			free(m->nick);
			free(m->uhost);
			next = m->next;
			free(m);
		}
		if (chan->topic) free(chan->topic);
		if (chan->topic_nick) free(chan->topic_nick);
		if (chan->key) free(chan->key);
		free(chan);
	}
}

void channel_on_nick(const char *old_nick, const char *new_nick)
{
	channel_t *chan;
	channel_member_t *m;

	for (chan = channel_head; chan; chan = chan->next) {
		for (m = chan->member_head; m; m = m->next) {
			if (!(current_server.strcmp)(m->nick, old_nick)) {
				str_redup(&m->nick, new_nick);
				break;
			}
		}
	}
}
