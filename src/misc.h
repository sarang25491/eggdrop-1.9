/*
 * misc.h --
 *
 *	maskhost() dumplots() daysago() days() daysdur()
 *	queueing output for the bot (msg and help)
 *	help system
 *	motd display and %var substitution
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Eggheads Development Team
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
 * $Id: misc.h,v 1.10 2003/12/11 00:49:11 wcc Exp $
 */

#ifndef _EGG_MISC_H
#define _EGG_MISC_H

#include "flags.h"		/* flag_record		*/

#define HELP_DCC        1
#define HELP_TEXT       2
#define HELP_IRC        16

#ifndef MAKING_MODS

extern void _maskhost(const char *, char *, int);
extern void dumplots(int, const char *, char *);
extern void daysago(time_t, time_t, char *);
extern void days(time_t, time_t, char *);
extern void daysdur(time_t, time_t, char *);
extern void help_subst(char *, char *, struct flag_record *, int, char *);
extern void sub_lang(int, char *);
extern void show_motd(int);
extern void tellhelp(int, char *, struct flag_record *, int);
extern void tellwildhelp(int, char *, struct flag_record *);
extern void tellallhelp(int, char *, struct flag_record *);
extern void showhelp(char *, char *, struct flag_record *, int);
extern void rem_help_reference(char *file);
extern void add_help_reference(char *file);
extern void reload_help_data(void);
extern char *extracthostname(char *);
extern void show_telnet_banner(int i);
extern void make_rand_str(char *, int);
extern int oatoi(const char *);
extern void kill_bot(char *, char *);

#define maskhost(a,b) _maskhost((a),(b),1)
#define maskban(a,b)  _maskhost((a),(b),0)

#endif				/* !MAKING_MODS */

#endif				/* !_EGG_MISC_H */
