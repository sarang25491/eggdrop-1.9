/*
 * hash_table.h --
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
/*
 * $Id: hash_table.h,v 1.3 2002/05/05 16:40:33 tothwolf Exp $
 */

#ifndef _EGG_HASH_TABLE_H
#define _EGG_HASH_TABLE_H

#define HASH_TABLE_STRINGS 1
#define HASH_TABLE_INTS    2
#define HASH_TABLE_MIXED   4

/* Turns a key into an unsigned int. */
typedef unsigned int (*hash_table_hash_alg)(void *key);

/* Returns -1, 0, or 1 if left is <, =, or > than right. */
typedef int (*hash_table_cmp_alg)(const void *left, const void *right);

typedef int (*hash_table_node_func)(const void *key, void *data, void *param);

typedef struct hash_table_entry_b {
	struct hash_table_entry_b *next;
	void *key;
	void *data;
} hash_table_entry_t;

typedef struct hash_table_b {
	int flags;
	int max_rows;
	int rows_in_use;
	int collisions;
	hash_table_hash_alg hash;
	hash_table_cmp_alg cmp;
	hash_table_entry_t entries[1];
} hash_table_t;

hash_table_t *hash_table_create(hash_table_hash_alg alg, hash_table_cmp_alg cmp, int nrows, int flags);
int hash_table_destroy(hash_table_t *ht);
int hash_table_insert(hash_table_t *ht, void *key, void *data);
int hash_table_replace(hash_table_t *ht, void *key, void *data);
int hash_table_find(hash_table_t *ht, void *key, void *dataptr);
int hash_table_delete(hash_table_t *ht, void *key);

#endif				/* _EGG_HASH_TABLE_H */
