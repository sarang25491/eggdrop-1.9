/*
 * tclhash.h
 *
 * $Id: tclhash.h,v 1.26 2001/12/09 03:55:58 stdarg Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001 Eggheads Development Team
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

#ifndef _EGG_TCLHASH_H
#define _EGG_TCLHASH_H

/* Flags for bind entries */
/* Does the callback want their client_data inserted as the first argument? */
#define BIND_WANTS_CD 1

/* Flags for bind tables */
#define BIND_STRICT_ATTR 0x80
#define BIND_BREAKABLE 0x100

/* Flags for return values from bind callbacks */
#define BIND_RET_LOG 1
#define BIND_RET_BREAK 2

/* This holds the final information for a function listening on a bind
   table. */
typedef struct bind_entry_b {
	struct bind_entry_b *next;
	struct flag_record user_flags;
	char *function_name;
	Function callback;
	void *client_data;
	int hits;
	int bind_flags;
} bind_entry_t;

/* This is the list of bind masks in a given table.
   For instance, in the "msg" table you might have "pass", "op",
   and "invite". */
typedef struct bind_chain_b {
	struct bind_chain_b *next;
	bind_entry_t *entries;
	char *mask;
	int flags;
} bind_chain_t;

/* This is the highest-level structure. It's like the "msg" table
   or the "pubm" table. */
typedef struct bind_table_b {
	struct bind_table_b *next;
	bind_chain_t *chains;
	char *name;
	char *syntax;
	int nargs;
	int match_type;
	int flags;
} bind_table_t;

#ifndef MAKING_MODS

void kill_bind2(void);

void check_tcl_dcc(const char *, int, const char *);
void check_tcl_chjn(const char *, const char *, int, char, int, const char *);
void check_tcl_chpt(const char *, const char *, int, int);
void check_tcl_bot(const char *, const char *, const char *);
void check_tcl_link(const char *, const char *);
void check_tcl_disc(const char *);
const char *check_tcl_filt(int, const char *);
int check_tcl_note(const char *, const char *, const char *);
void check_tcl_listen(const char *, int);
void check_tcl_time(struct tm *);
void check_tcl_nkch(const char *, const char *);
void check_tcl_away(const char *, int, const char *);
void check_tcl_event(const char *);
int check_tcl_chat(const char *, int, const char *);
void check_tcl_act(const char *, int, const char *);
void check_tcl_bcst(const char *, int, const char *);
void check_tcl_chon(char *, int);
void check_tcl_chof(char *, int);

int check_bind(bind_table_t *table, const char *match, struct flag_record *_flags, ...);

bind_table_t *add_bind_table2(const char *name, int nargs, const char *syntax, int match_type, int flags);

void del_bind_table2(bind_table_t *table);

bind_table_t *find_bind_table2(const char *name);

int add_bind_entry(bind_table_t *table, const char *flags, const char *mask, const char *function_name, int bind_flags, Function callback, void *client_data);

int del_bind_entry(bind_table_t *table, const char *flags, const char *mask, const char *function_name, void *cdata);

void add_builtins2(bind_table_t *table, cmd_t *cmds);

void rem_builtins2(bind_table_t *table, cmd_t *cmds);

#endif

#endif				/* _EGG_TCLHASH_H */
