/*
 * dccutil.h --
 */
/*
 * Copyright (C) 2000, 2001, 2002, 2003 Eggheads Development Team
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
/*
 * $Id: dccutil.h,v 1.4 2003/01/02 21:33:16 wcc Exp $
 */

#ifndef _EGG_DCCUTIL_H
#define _EGG_DCCUTIL_H

/* TODO: rename me to dccprintf
 *  (background: dprintf has become a system call,
 *   so all occurences of dprintf in eggdrop has been
 *   redefnied as dprintf_eggdrop)
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef HAVE_DPRINTF
# define dprintf dprintf_eggdrop
#endif

void dprintf EGG_VARARGS(int, arg1);
void chatout EGG_VARARGS(char *, arg1);
extern void (*shareout) ();
extern void (*sharein) (int, char *);
void chanout_but EGG_VARARGS(int, arg1);
void dcc_chatter(int);
void lostdcc(int);
void makepass(char *);
void tell_dcc(int);
void not_away(int);
void set_away(int, char *);
void dcc_remove_lost(void);
void flush_lines(int, struct chat_info *);
struct dcc_t *find_idx(int);
int new_dcc(struct dcc_table *, int);
void del_dcc(int);
char *add_cr(char *);
void changeover_dcc(int, struct dcc_table *, int);
void do_boot(int, char *, char *);
int detect_dcc_flood(time_t *, struct chat_info *, int);

/* Moved there since there is no botnet */
int add_note(char *to, char *from, char *msg, int idx, int echo);

#endif				/* !_EGG_DCCUTIL_H */
