/*
 * hash_table_test.c --
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
static const char rcsid[] = "$Id: hash_table_test.c,v 1.2 2002/05/05 16:40:33 tothwolf Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include "hash_table.h"

int my_node_func(void *key, void *data, void *param)
{
	printf("my_node_func: %d, %d\n", key, data);
	return(0);
}

int main ()
{
	hash_table_t *ht;
	int data, i;

	ht = hash_table_create(NULL, NULL, 3, HASH_TABLE_INTS);

	srand(time(NULL));
	printf("Creating hash table...\n");
	for (i = 0; i < 10; i++) {
		hash_table_insert(ht, (void *)i, (void *)i);
	}

	printf("first walk:\n");
	hash_table_walk(ht, my_node_func, NULL);
	//hash_table_delete(ht, (void *)5);
	//hash_table_delete(ht, (void *)5);
	hash_table_delete(ht, (void *)8);
	hash_table_delete(ht, (void *)2);
	hash_table_delete(ht, (void *)2);
	printf("second walk:\n");
	hash_table_walk(ht, my_node_func, NULL);

	printf("Retrieving from hash table...\n");
	for (i = 0; i < 10; i++) {
		if (hash_table_find(ht, (void *)i, &data)) {
			printf("key not found: %d\n", i);
		}
		else if (i != data) {
			printf("key %d has wrong data: %d\n", i, data);
		}
	}
	return(0);
}
