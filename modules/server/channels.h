#ifndef _CHANNELS_H_
#define _CHANNELS_H_

/* Status bits for channels. */
#define CHANNEL_WHOLIST	1
#define CHANNEL_BANLIST	2

typedef struct {
	char *nick;
	char *uhost;
	int ref_count;
} uhost_cache_entry_t;

typedef struct channel_member {
	struct channel_member *next;

	char *nick;
	char *uhost;
	int join_time;
	int mode;
} channel_member_t;

typedef struct channel_mask {
	struct channel_mask *next;
	char *mask;
	char *set_by;
	int time;
} channel_mask_t;

typedef struct channel {
	struct channel *next;

	char *name;

	char *topic, *topic_nick;	/* Topic and who set it. */
	int topic_time;	/* When it was set. */

	int mode;	/* Mode bits. */
	int limit;	/* Channel limit. */
	char *key;	/* Channel key. */

	channel_mask_t *ban_head;	/* Ban list. */
	int nbans;

	int status;

	channel_member_t *member_head;	/* Member list. */
	int nmembers;
} channel_t;

extern channel_t *channel_head;
extern int nchannels;

extern hash_table_t *uhost_cache_ht;

extern void server_channel_init();
extern void server_channel_destroy();

extern void channel_lookup(const char *chan_name, int create, channel_t **chanptr, channel_t **prevptr);
extern char *uhost_cache_lookup(const char *nick);
extern void uhost_cache_fillin(const char *nick, const char *uhost, int addref);

/* Events that hook into input.c. */
extern void channel_on_nick(const char *old_nick, const char *new_nick);
extern void channel_on_quit(const char *nick, const char *uhost, user_t *u);
extern void channel_on_leave(const char *chan_name, const char *nick, const char *uhost, user_t *u);
extern void channel_on_join(const char *chan_name, const char *nick, const char *uhost);
extern void channel_add_member(const char *chan_name, const char *nick, const char *uhost);

/* Functions for others (scripts/modules) to access channel data. */
extern void channel_list(char ***chans, int *nchans);

#endif
