#ifndef _CHANNELS_H_
#define _CHANNELS_H_

typedef struct channel_member {
	struct channel_member *next;
	char *nick;
	char *modes;
	int imode;
} channel_member_t;

typedef struct {
	char *name;

	/* Topic. */
	char *topic, *topic_nick;
	int topic_time;

	char *modes;
	int imode;
	int limit;
	char *key;
	channel_member_t *members;
	int nmembers;
} channel_t;

extern void channels_join_all();

#endif
