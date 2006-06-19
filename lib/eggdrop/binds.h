/* binds.h: header for binds.c
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
 *
 * $Id: binds.h,v 1.11 2006/06/19 15:47:33 stdarg Exp $
 */

#ifndef _EGG_BINDS_H_
#define _EGG_BINDS_H_

/* Match type flags for bind tables. */
#define MATCH_PARTIAL	1
#define MATCH_EXACT	2
#define MATCH_MASK	4
#define MATCH_CASE	8
#define MATCH_NONE	16
#define MATCH_FLAGS_AND	32
#define MATCH_FLAGS_OR	64
#define MATCH_FLAGS	96

/* Flags for binds. */
#define BIND_WANTS_CD   1 /* Does the callback want their client_data inserted as the first argument? */
#define BIND_BREAKABLE	2
#define BIND_STACKABLE	4
#define BIND_DELETED	8
#define BIND_FAKE	16
/*** Note: bind entries can specify these two flags, defined above.
#define MATCH_FLAGS_AND	32
#define MATCH_FLAGS_OR	64
***/

/* Flags for return values from bind callbacks. */
#define BIND_RET_LOG		1
#define BIND_RET_BREAK		2
#define BIND_RET_LOG_COMMAND	3

/* This lets you add a null-terminated list of binds all at once. */
typedef struct {
	const char *user_flags;
	const char *mask;
	Function callback;
} bind_list_t;

/* This can be used to identify the module or script that created this bind */
typedef struct event_owner_b {
	char *name;
	struct egg_module *module;
	char *script;
	void *client_data;
	int (*on_delete)(struct event_owner_b *event, void *client_data);
} event_owner_t;

/* This holds the information for a bind entry. */
typedef struct bind_entry_b {
	struct bind_entry_b *next, *prev;
	flags_t user_flags;
	char *mask;
	char *function_name;
	Function callback;
	void *client_data;
	int nhits;
	int flags;
	event_owner_t *owner;
} bind_entry_t;

/* This is the highest-level structure. It's like the "msg" table or the "pubm" table. */
typedef struct bind_table_b {
	struct bind_table_b *next;
	bind_entry_t *entries;
	char *name;
	char *syntax;
	int nargs;
	int match_type;
	int flags;
} bind_table_t;

void bind_killall();
int bind_check(bind_table_t *table, flags_t *user_flags, const char *match, ...);
int bind_check_hits(bind_table_t *table, flags_t *user_flags, const char *match, int *hits, ...);

bind_table_t *bind_table_add(const char *name, int nargs, const char *syntax, int match_type, int flags);
void bind_table_del(bind_table_t *table);
void kill_binds_by_module(struct egg_module *module);
bind_table_t *bind_table_lookup(const char *name);
bind_table_t *bind_table_lookup_or_fake(const char *name);
int bind_entry_add(bind_table_t *table, const char *user_flags, const char *mask, const char *function_name, int bind_flags, Function callback, void *client_data, event_owner_t *owner);
int bind_entry_del(bind_table_t *table, const char *mask, const char *function_name, Function callback);
int bind_entry_modify(bind_table_t *table, const char *mask, const char *function_name, const char *newflags, const char *newmask);
int bind_entry_overwrite(bind_table_t *table, const char *mask, const char *function_name, Function callback, void *client_data, event_owner_t *owner);
void bind_add_list(const char *table_name, bind_list_t *cmds);
void bind_add_simple(const char *table_name, const char *flags, const char *mask, Function callback);

/* XXX: why bind_rem_list and not bind_del_list? */
void bind_rem_list(const char *table_name, bind_list_t *cmds);
/* XXX: why bind_rem_simple and not bind_del_simple? */
void bind_rem_simple(const char *table_name, const char *flags, const char *mask, Function callback);
bind_table_t *bind_table_list(void);

#endif /* !_EGG_BINDS_H_ */
