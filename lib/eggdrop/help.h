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
 * $Id: help.h,v 1.5 2004/09/29 18:03:53 stdarg Exp $
 */

#ifndef _EGG_HELP_H_
#define _EGG_HELP_H_

typedef struct {
	char *name;
	int ref;
} help_file_t;

typedef struct {
	char *name;
	char *syntax;
	char *summary;
	help_file_t *file;
} help_summary_t;

typedef struct {
	char *name;
	help_summary_t **entries;
	int nentries;
} help_section_t;

typedef struct {
	char *search;
	int cursection;
	int curentry;
} help_search_t;

int help_init();
int help_shutdown();

int help_count_sections();
int help_count_entries();

int help_load(const char *filename);
int help_unload(const char *filename);

int help_load_by_module(const char *name);
int help_unload_by_module(const char *name);

help_summary_t *help_lookup_summary(const char *name);
help_section_t *help_lookup_section(const char *name);
int help_print_entry(partymember_t *p, const char *name);

help_search_t *help_search_new(const char *searchstr);
int help_search_end(help_search_t *search);
help_summary_t *help_search_result(help_search_t *search);

#endif /* !_EGG_HELP_H_ */
