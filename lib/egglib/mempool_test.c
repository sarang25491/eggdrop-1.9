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
