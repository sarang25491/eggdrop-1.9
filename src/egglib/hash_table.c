#include <stdio.h>
#include <string.h>

#include "hash_table.h"

hash_table_t *hash_table_create(hash_table_hash_alg alg, hash_table_cmp_alg cmp, int nrows, int flags)
{
	hash_table_t *ht;
	int size;

	if (nrows <= 0) nrows = 13; /* Give them a small table to start with. */

	size = sizeof(*ht) + (nrows-1) * sizeof(hash_table_entry_t);
	ht = calloc(1, size);

	if (alg) ht->hash = alg;
	else {
		if (flags & HASH_TABLE_STRINGS) ht->hash = my_string_hash;
		else if (flags & HASH_TABLE_INTS) ht->hash = my_int_hash;
		else ht->hash = my_mixed_hash;
	}
	if (cmp) ht->cmp = cmp;
	else {
		if (flags & HASH_TABLE_INTS) ht->cmp = my_int_cmp;
		else ht->cmp = (hash_table_cmp_alg) strcmp;
	}
	ht->flags = flags;
	ht->max_rows = nrows;
	ht->rows_in_use = 0;
	ht->collisions = 0;
	return(ht);
}

int hash_table_destroy(hash_table_t *ht)
{
	return(0);
}

int hash_table_insert(hash_table_t *ht, void *key, void *data)
{
	int idx;
	hash_table_entry_t *entry;

	idx = (ht->hash)(key) % ht->max_rows;
	entry = ht->entries+idx;

	/* Is this slot empty? */
	if (!entry->next) {
		/* Add one to the row counter */
		ht->rows_in_use++;
	} else {
		/* No.. append new entry to end */
		while (entry->next != entry) entry = entry->next;
		entry->next = (hash_table_entry_t *)malloc(sizeof(*entry));
		entry = entry->next;

		/* Add one to the collision counter */
		ht->collisions++;
	}
	entry->key = key;
	entry->data = data;
	entry->next = entry;
	return(0);
}

int hash_table_find(hash_table_t *ht, void *key, void *dataptr)
{
	int idx;
	hash_table_entry_t *entry, *last;

	idx = (ht->hash)(key) % ht->max_rows;

	last = (hash_table_entry_t *)0;
	for (entry = ht->entries+idx; entry->next != last; entry = entry->next) {
		if (!ht->cmp(key, entry->key)) {
			*(void **)dataptr = entry->data;
			return(0);
		}
		last = entry;
	}
	return(1);
}

int hash_table_delete(hash_table_t *ht, void *key)
{
	int idx;
	hash_table_entry_t *entry, *last;

	idx = (ht->hash)(key) % ht->max_rows;

	last = (hash_table_entry_t *)0;
	for (entry = ht->entries+idx; entry->next != last; entry = entry->next) {
		if (!ht->cmp(key, entry->key)) {
			if (last) {
				/* Do we need to update the previous one? */
				if (entry == entry->next) last->next = last;
				else last->next = entry->next;
				free(entry);
			}
			else if (entry->next != entry) {
				/* Ok, we are the first in the chain. */
				/* Copy the next one onto us. */
				entry->key = entry->next->key;
				entry->data = entry->next->data;
				last = entry->next;
				if (entry->next->next == entry->next) entry->next = entry;
				else entry->next = entry->next->next;
				/* Now free the next one (we just copied). */
				free(last);
			}
			else {
				/* We are the only entry on this row. */
				entry->next = (hash_table_entry_t *)0;
			}
			return(0);
		}
		last = entry;
	}
	return(1);
}

int hash_table_walk(hash_table_t *ht, hash_table_node_func callback, void *param)
{
	hash_table_entry_t *entry, *last;
	int i;

	for (i = 0; i < ht->max_rows; i++) {
		last = (hash_table_entry_t *)0;
		for (entry = ht->entries+i; entry->next != last; entry = entry->next) {
			callback(entry->key, entry->data, param);
			last = entry;
		}
	}
}

static int my_int_cmp(const void *left, const void *right)
{
	return((int) left - (int) right);
}

static unsigned int my_string_hash(void *key)
{
	int hash, loop, keylen;
	unsigned char *k;

#define HASHC hash = *k++ + 65599 * hash
	hash = 0;
	k = (unsigned char *)key;
	keylen = strlen((char *)key);

	loop = (keylen + 8 - 1) >> 3;
	switch (keylen & (8 - 1)) {
		case 0:
			do {
				HASHC;
		case 7:
				HASHC;
		case 6:
				HASHC;
		case 5:
				HASHC;
		case 4:
				HASHC;
		case 3:
				HASHC;
		case 2:
				HASHC;
		case 1:
				HASHC;
			} while (--loop);
	}
	return(hash);
}

static unsigned int my_int_hash(void *key)
{
	return((unsigned int)key);
}

static unsigned int my_mixed_hash (void *key)
{
	unsigned char *k;
	unsigned int hash;

        k = (unsigned char *)key;
	hash = 0;
	while (*k) {
		hash *= 16777619;
		hash ^= *k++;
	}
	return(hash);
}
