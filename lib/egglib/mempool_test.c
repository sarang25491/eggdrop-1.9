/*
 * mempool_test.c --
 */
/*
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
 */

#ifndef lint
static const char rcsid[] = "$Id: mempool_test.c,v 1.4 2003/12/11 00:49:10 wcc Exp $";
#endif

#include <stdio.h>
#include "mempool.h"

main ()
{
	mempool_t *pool;
	char *test;
	int i;

	pool = mempool_create(NULL, 10, 10);

	printf("Testing mempool\n");
	for (i = 0; i < 10000000; i++) {
		test = (char *)mempool_get_chunk(pool);
		mempool_free_chunk(pool, test);
	}
	mempool_destroy(pool);
	printf("Testing malloc\n");
	for (i = 0; i < 10000000; i++) {
		test = (char *)malloc(10);
		free(test);
	}
	return(0);
}
