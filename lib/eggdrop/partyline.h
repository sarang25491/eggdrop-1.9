/* partyline.h: header for partyline.c
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
 *
 * $Id: partyline.h,v 1.18 2004/06/20 13:33:48 wingman Exp $
 */

#ifndef _EGG_PARTYLINE_H_
#define _EGG_PARTYLINE_H_

/* Flags for partyline. */
#define PARTY_DELETED	1
#define PARTY_SELECTED	2

/* Bind table names for partyline events */
#define BTN_PARTYLINE_JOIN	"partyjoin"
#define BTN_PARTYLINE_PART	"partypart"
#define BTN_PARTYLINE_PUBLIC	"partypub"
#define BTN_PARTYLINE_CMD	"party"
#define BTN_PARTYLINE_OUT	"party_out"

struct partyline_event;
struct partychan;

typedef struct partymember {
	struct partymember *next, *prev;
	int pid;
	char *nick, *ident, *host;
	user_t *user;
	int flags;

	struct partychan **channels;
	int nchannels;

	struct partyline_event *handler;
	void *client_data;
} partymember_t;

typedef struct partychan_member {
	partymember_t *p;
	int flags;
} partychan_member_t;

typedef struct partychan {
	struct partychan *next, *prev;
	int cid;
	char *name;
	int flags;

	partychan_member_t *members;
	int nmembers;
} partychan_t;

typedef struct partymember_common {
	struct partymember_common *next;
	partymember_t **members;
	int len;
	int max;
} partymember_common_t;

typedef struct partyline_event {
	/* Events that don't depend on a single chan. */
	int (*on_privmsg)(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len);
	int (*on_nick)(void *client_data, partymember_t *src, const char *oldnick, const char *newnick);
	int (*on_quit)(void *client_data, partymember_t *src, const char *text, int len);

	/* Channel events. */
	int (*on_chanmsg)(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
	int (*on_join)(void *client_data, partychan_t *chan, partymember_t *src);
	int (*on_part)(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
} partyline_event_t;

/* helper handler for partyline events to reduce work in e.g. telnetparty module. */
int partyline_idx_privmsg(int idx,  partymember_t *dest, partymember_t *src, const char *text, int len);
int partyline_idx_nick(int idx, partymember_t *src, const char *oldnick, const char *newnick);
int partyline_idx_quit(int idx, partymember_t *src, const char *text, int len);
int partyline_idx_chanmsg(int idx, partychan_t *chan, partymember_t *src, const char *text, int len);
int partyline_idx_join(int idx, partychan_t *chan, partymember_t *src);
int partyline_idx_part(int idx, partychan_t *chan, partymember_t *src, const char *text, int len);

int partyline_init();
partymember_t *partymember_new(int pid, user_t *user, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data);
int partyline_delete(partymember_t *p, const char *text);
int partyline_is_command(const char *text);
int partyline_on_input(partychan_t *chan, partymember_t *p, const char *text, int len);
int partyline_on_command(partymember_t *p, const char *cmd, const char *text);
int partyline_update_info(partymember_t *p, const char *ident, const char *host);

/* Channel functions. */
partychan_t *partychan_new(int cid, const char *name);
partychan_t *partychan_lookup_cid(int cid);
partychan_t *partychan_lookup_name(const char *name);
partychan_t *partychan_get_default(partymember_t *p);
int partychan_ison_name(const char *chan, partymember_t *p);
int partychan_ison(partychan_t *chan, partymember_t *p);
int partychan_join_name(const char *chan, partymember_t *p);
int partychan_join_cid(int cid, partymember_t *p);
int partychan_join(partychan_t *chan, partymember_t *p);
int partychan_part_name(const char *chan, partymember_t *p, const char *text);
int partychan_part_cid(int cid, partymember_t *p, const char *text);
int partychan_part(partychan_t *chan, partymember_t *p, const char *text);
int partychan_msg_name(const char *name, partymember_t *src, const char *text, int len);
int partychan_msg_cid(int cid, partymember_t *src, const char *text, int len);
int partychan_msg(partychan_t *chan, partymember_t *src, const char *text, int len);
partymember_common_t *partychan_get_common(partymember_t *p);
int partychan_free_common(partymember_common_t *common);

partymember_t *partymember_lookup_pid(int pid);
partymember_t *partymember_lookup_nick(const char *nick);
partymember_t *partymember_new(int pid, user_t *user, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data);
int partymember_delete(partymember_t *p, const char *text);
int partymember_update_info(partymember_t *p, const char *ident, const char *host);
int partymember_who(int **pids, int *len);
int partymember_write_pid(int pid, const char *text, int len);
int partymember_write(partymember_t *p, const char *text, int len);
int partymember_msg(partymember_t *p, partymember_t *src, const char *text, int len);
int partymember_printf_pid(int pid, const char *fmt, ...);
int partymember_printf(partymember_t *p, const char *fmt, ...);

#endif /* !_EGG_PARTYLINE_H_ */
