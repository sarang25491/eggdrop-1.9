#include <stdio.h>
#include <stdlib.h>
#include "linked_list.h"

main ()
{
	linked_list_t *list;
	linked_list_cursor_t *cursor;
	int i, data;

	list = linked_list_create(NULL, linked_list_int_cmp, LINKED_LIST_SORTED);

	printf("creating list...\n");
	for (i = 10; i > 0; i--) linked_list_prepend(list, (void *)i, (void *)(i+1));
	for (i = 11; i < 20; i++) linked_list_append(list, (void *)i, (void *)(i+1));

	cursor = linked_list_cursor_create(NULL, list);
	printf("searching...\n");
	linked_list_cursor_find(cursor, (void *)900000);
	linked_list_cursor_get(cursor, &data);
	printf("%d\n", data);
	linked_list_cursor_find(cursor, (void *)900001);
	linked_list_cursor_get(cursor, &data);
	printf("%d\n", data);
	linked_list_cursor_find(cursor, (void *)899999);
	linked_list_cursor_get(cursor, &data);
	printf("%d\n", data);

	printf("walking list...\n");
	for (linked_list_cursor_home(cursor); linked_list_cursor_get(cursor, &data); linked_list_cursor_next(cursor)) {
		//printf("data: %d\n", data);
	}
}
