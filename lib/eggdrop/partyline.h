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
 * $Id: partyline.h,v 1.21 2004/06/23 20:19:45 wingman Exp $
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

typedef struct partymember partymember_t;
typedef struct partychan partychan_t;
typedef struct partychan_member partychan_member_t;
typedef struct partymember_common partymember_common_t;
typedef struct partyline_event partyline_event_t;

#include <eggdrop/partychan.h>
#include <eggdrop/partymember.h>

struct partyline_event {
	/* Events that don't depend on a single chan. */
	int (*on_privmsg)(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len);
	int (*on_nick)(void *client_data, partymember_t *src, const char *oldnick, const char *newnick);
	int (*on_quit)(void *client_data, partymember_t *src, const char *text, int len);

	/* Channel events. */
	int (*on_chanmsg)(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
	int (*on_join)(void *client_data, partychan_t *chan, partymember_t *src);
	int (*on_part)(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
};

/* helper handler for partyline events to reduce work in e.g. telnetparty module. */
int partyline_idx_privmsg(int idx,  partymember_t *dest, partymember_t *src, const char *text, int len);
int partyline_idx_nick(int idx, partymember_t *src, const char *oldnick, const char *newnick);
int partyline_idx_quit(int idx, partymember_t *src, const char *text, int len);
int partyline_idx_chanmsg(int idx, partychan_t *chan, partymember_t *src, const char *text, int len);
int partyline_idx_join(int idx, partychan_t *chan, partymember_t *src);
int partyline_idx_part(int idx, partychan_t *chan, partymember_t *src, const char *text, int len);

int partyline_init(void);
int partyline_shutdown(void);
partymember_t *partymember_new(int pid, user_t *user, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data);
int partyline_delete(partymember_t *p, const char *text);
int partyline_is_command(const char *text);
int partyline_on_input(partychan_t *chan, partymember_t *p, const char *text, int len);
int partyline_on_command(partymember_t *p, const char *cmd, const char *text);
int partyline_update_info(partymember_t *p, const char *ident, const char *host);

#endif /* !_EGG_PARTYLINE_H_ */
