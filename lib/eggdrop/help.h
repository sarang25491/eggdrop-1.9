/* help.h: header for help.c
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
 * $Id: help.h,v 1.1 2004/01/11 12:16:08 wcc Exp $
 */

#ifndef _EGG_HELP_H_
#define _EGG_HELP_H_

#define HELP_HASH_SIZE 50

typedef struct help {
	char *name;
	char *syntax;	/* Command syntax */
	char *flags;	/* Flags required to use command (if entry is a command) */

	char **lines;	/* Lines of help data */
	int nlines;	/* Number of help data lines */

	char **seealso;	/* See also entries */
	int nseealso;	/* Number of see also entries */
} help_t;

int help_init();
int help_load(const char *, const char *);
help_t *help_new(const char *);
help_t *help_lookup_by_name(const char *);
int help_count();
int help_print_party(partymember_t *, help_t *);

#endif /* !_EGG_HELP_H_ */
