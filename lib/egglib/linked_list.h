#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#define LINKED_LIST_SORTED	1
#define LINKED_LIST_DOUBLE	2
#define LINKED_LIST_STATIC	4

typedef int (*linked_list_cmp_func)(const void *left, const void *right);
typedef void (*linked_list_node_func)(void *data, void *param);

typedef struct linked_list_node_b {
	void *key;
	void *data;
	struct linked_list_node_b *next, *prev;
} linked_list_node_t;

typedef struct linked_list_b {
	linked_list_cmp_func cmp;
	linked_list_node_t *head, *tail;
	int flags;
} linked_list_t;

typedef struct linked_list_cursor_b {
	linked_list_node_t *node;
	linked_list_t *list;
	int flags;
} linked_list_cursor_t;


linked_list_t *linked_list_create(linked_list_t *list, linked_list_cmp_func func, int flags);
int linked_list_destroy(linked_list_t *list);
int linked_list_walk(linked_list_t *list, linked_list_node_func callback, void *param);
linked_list_cursor_t *linked_list_cursor_create(linked_list_cursor_t *cursor, linked_list_t *list);
int linked_list_cursor_destroy(linked_list_cursor_t *cursor);
int linked_list_cursor_get(linked_list_cursor_t *cursor, void *itemptr);
int linked_list_cursor_home(linked_list_cursor_t *cursor);
int linked_list_cursor_end(linked_list_cursor_t *cursor);
int linked_list_cursor_prepend(linked_list_cursor_t *cursor, void *key, void *data);
int linked_list_cursor_append(linked_list_cursor_t *cursor, void *key, void *data);
int linked_list_cursor_prev(linked_list_cursor_t *cursor);
int linked_list_cursor_next(linked_list_cursor_t *cursor);
int linked_list_cursor_find(linked_list_cursor_t *cursor, void *key);
int linked_list_cursor_replace(linked_list_cursor_t *cursor, void *key, void *data);
int linked_list_cursor_delete(linked_list_cursor_t *cursor);
int linked_list_append(linked_list_t *list, void *key, void *data);
int linked_list_prepend(linked_list_t *list, void *key, void *data);
int linked_list_insert(linked_list_t *list, void *key, void *data);
int linked_list_delete(linked_list_t *list, void *key, linked_list_node_func callback);
int linked_list_int_cmp(const void *left, const void *right);

#endif
