/* garbage.c
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
 */

#ifndef lint
static const char rcsid[] = "$Id: garbage.c,v 1.4 2004/09/26 09:42:09 stdarg Exp $";
#endif

#include <stdlib.h>

#include <eggdrop/eggdrop.h>

typedef struct garbage_list {
	struct garbage_list *next;
	garbage_proc_t garbage_proc;
	void *client_data;
} garbage_list_t;

static garbage_list_t *garbage_list_head = NULL;

int garbage_add(garbage_proc_t garbage_proc, void *client_data, int flags)
{
	garbage_list_t *g;

	/* Make sure it's not a duplicate (helpful for cleanup functions). */
	if (flags & GARBAGE_ONCE) {
		for (g = garbage_list_head; g; g = g->next) {
			if (g->garbage_proc == garbage_proc && g->client_data == client_data) return(0);
		}
	}

	/* Create the entry. */
	g = malloc(sizeof(*g));
	g->garbage_proc = garbage_proc;
	g->client_data = client_data;

	/* Add to list. */
	g->next = garbage_list_head;
	garbage_list_head = g;

	return(0);
}

/* Should be called only from the main loop. */
int garbage_run()
{
	garbage_list_t *g;

	for (g = garbage_list_head; g; g = garbage_list_head) {
		garbage_list_head = g->next;
		(g->garbage_proc)(g->client_data);
		free(g);
	}
	return(0);
}
