#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

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

#endif /* _HASH_TABLE_H_ */
