/* channels.h: header for channels.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: channels.h,v 1.21 2004/10/04 15:48:30 stdarg Exp $
 */

#ifndef _EGG_MOD_SERVER_CHANNELS_H_
#define _EGG_MOD_SERVER_CHANNELS_H_

/* Status bits for channels. */
#define CHANNEL_WHOLIST		0x1
#define CHANNEL_BANLIST		0x2
#define CHANNEL_NAMESLIST	0x4
#define CHANNEL_JOINED		0x8

/* Flag bits for channels. */
#define	CHANNEL_STATIC		0x1
#define CHANNEL_INACTIVE	0x2

#define BOT_ISOP(chanptr) (((chanptr)->status & CHANNEL_JOINED) && flag_match_single_char(&((chanptr)->bot->mode), 'o'))
#define BOT_ISHALFOP(chanptr) (((chanptr)->status & CHANNEL_JOINED) && flag_match_single_char(&((chanptr)->bot->mode), 'h'))
#define BOT_CAN_SET_MODES(chanptr) (((chanptr)->status & CHANNEL_JOINED) && (flag_match_single_char(&((chanptr)->bot->mode), 'o') \
									|| flag_match_single_char(&((chanptr)->bot->mode), 'h')))

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
	flags_t mode;
} channel_member_t;

typedef struct channel_mask {
	struct channel_mask *next;
	char *mask;
	char *set_by;
/* FIXME - these should be long, not int. (EGGTIMEVALT) */
	int time;
	int last_used;
} channel_mask_t;

typedef struct channel_mask_list {
	struct channel_mask *head;
	int len;
	int loading;
	char type;
} channel_mask_list_t;

typedef struct channel_mode_arg {
	int type;
	char *value;
} channel_mode_arg_t;

typedef struct channel {
	struct channel *next, *prev;

	char *name;

	char *topic, *topic_nick;	/* Topic and who set it. */
	int topic_time;			/* When it was set. */

	flags_t mode;			/* Mode bits. */
	int limit;			/* Channel limit. */
	char *key;			/* Channel key. */

	channel_mask_list_t **lists;	/* Mask lists (bans, invites, etc). */
	int nlists;

	channel_mode_arg_t *args;	/* Stored channel modes. */
	int nargs;

	channel_member_t *member_head;	/* Member list. */
	int nmembers;

	channel_member_t *bot;		/* All you need to know about me :-) */

	xml_node_t *settings;	/* Extended settings for scripts/modules/us. */
	int status;		/* Status of channel. */
	int flags;		/* Internal flags for channel. */
} channel_t;

extern channel_t *channel_head;
extern int nchannels;

extern hash_table_t *uhost_cache_ht;

/* channels.c */
extern void channel_init();
extern void channel_reset();
extern void channel_destroy();
extern void channel_free(channel_t *chan);
extern channel_t *channel_probe(const char *chan_name, int create);
extern channel_t *channel_add(const char *name);
extern int channel_remove(const char *name);
extern int channel_load(const char *fname);
extern int channel_save(const char *fname);
extern int channel_set(channel_t *chan, const char *value, ...);
extern int channel_get(channel_t *chan, char **strptr, ...);
extern int channel_get_int(channel_t *chan, int *intptr, ...);
extern xml_node_t *channel_get_node(channel_t *chan, ...);
extern int channel_mode(const char *chan_name, const char *nick, char *buf);
extern int channel_mode_arg(const char *chan_name, int type, const char **value);
extern channel_mask_list_t *channel_get_mask_list(channel_t *chan, char type);
extern void channel_add_mask(channel_t *chan, char type, const char *mask, const char *set_by, int time);
extern void channel_del_mask(channel_t *chan, char type, const char *mask);

/* channel_events.c */
extern void channel_events_init();
extern void channel_events_destroy();
extern void channel_free_online(channel_t *chan);
extern void channel_on_quit(const char *nick, const char *uhost, user_t *u);
extern void channel_on_connect();

/* uhost_cache.c */
extern void uhost_cache_init();
extern void uhost_cache_reset();
extern void uhost_cache_destroy();
extern char *uhost_cache_lookup(const char *nick);
extern void uhost_cache_addref(const char *nick, const char *uhost);
extern void uhost_cache_decref(const char *nick);
void uhost_cache_swap(const char *old_nick, const char *new_nick);

#endif /* !_EGG_MOD_SERVER_CHANNELS_H_ */
