/*
 * mstack.c --
 *
 *	Implement a stack based on malloc.
 */
/*
 * Copyright (C) 2002 Eggheads Development Team
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
static const char rcsid[] = "$Id: mstack.c,v 1.5 2002/05/05 16:40:33 tothwolf Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mstack.h"

mstack_t *mstack_new(int initial_size)
{
	mstack_t *m;

	if (initial_size <= 0) initial_size = 10;
	m = (mstack_t *)calloc(1, sizeof(mstack_t));
	m->len = 0;
	m->max = initial_size;
	m->stack = (void **)calloc(1, sizeof(void *) * initial_size);
	return(m);
}

int mstack_destroy(mstack_t *m)
{
	if (m->stack) {
		memset(m->stack, 0, m->max * sizeof(void *));
		free(m->stack);
	}
	memset(m, 0, sizeof(*m));
	free(m);
	return(0);
}

void *mstack_push(mstack_t *m, void *item)
{
	if (m->len == m->max) mstack_grow(m, 10);
	m->stack[m->len] = item;
	m->len++;
	return(item);
}

int mstack_pop(mstack_t *m, void *itemptr)
{
	if (m->len == 0) return(1);
	m->len--;
	*(void **)itemptr = m->stack[m->len];
	return(0);
}

int mstack_grow(mstack_t *m, int nsteps)
{
	m->stack = (void **)realloc(m->stack, sizeof(void *) * (m->max + nsteps));
	memset(m->stack+m->max, 0, sizeof(void *) * nsteps);

	m->max += nsteps;
	return(0);
}
