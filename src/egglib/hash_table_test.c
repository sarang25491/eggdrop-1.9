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
