#include <stdio.h>
#include "mempool.h"
#include "linked_list.h"

static mempool_t *linked_list_node_mempool = NULL;

#define NEW_LIST ((linked_list_t *)malloc(sizeof(linked_list_t)))
#define NEW_NODE ((linked_list_node_t *)mempool_get_chunk(linked_list_node_mempool))
#define KILL_LIST(x) (free((x)))
#define KILL_NODE(x) (mempool_free_chunk(linked_list_node_mempool, (x)))

linked_list_t *linked_list_create(linked_list_t *list, linked_list_cmp_func cmp, int flags)
{
	linked_list_t *mylist;

	if (!linked_list_node_mempool) {
		linked_list_node_mempool = mempool_create(NULL, 32, sizeof(linked_list_node_t));
	}

	if (list) mylist = list;
	else mylist = NEW_LIST;
	if (!mylist) return((linked_list_t *)0);
	memset(mylist, 0, sizeof(*mylist));
	mylist->cmp = cmp;
	mylist->flags = flags;
	return(mylist);
}

int linked_list_destroy(linked_list_t *list)
{
	linked_list_node_t *node, *next;

	if (!list) return(0);
	for (node = list->head; node; node = next) {
		next = node->next;
		KILL_NODE(node);
	}
	if (!(list->flags & LINKED_LIST_STATIC)) KILL_LIST(list);

	return(0);
}

int linked_list_walk(linked_list_t *list, linked_list_node_func func, void *param)
{
	linked_list_node_t *node;

	for (node = list->head; node; node = node->next) {
		func(node->data, param);
	}
	return(0);
}

linked_list_cursor_t *linked_list_cursor_create(linked_list_cursor_t *cursor, linked_list_t *list)
{
	linked_list_cursor_t *c;

	if (cursor) c = cursor;
	else c = (linked_list_cursor_t *)malloc(sizeof(*c));

	memset(c, 0, sizeof(*c));
	c->list = list;
	c->node = list->head;
	if (cursor) c->flags = LINKED_LIST_STATIC;
	return(c);
}

int linked_list_cursor_destroy(linked_list_cursor_t *cursor)
{
	if (cursor && !(cursor->flags & LINKED_LIST_STATIC)) free(cursor);
	return(0);
}

int linked_list_cursor_home(linked_list_cursor_t *cursor)
{
	cursor->node = cursor->list->head;
	return(0);
}

int linked_list_cursor_end(linked_list_cursor_t *cursor)
{
	cursor->node = cursor->list->tail;
	return(0);
}

int linked_list_cursor_get(linked_list_cursor_t *cursor, void *itemptr)
{
	if (cursor->node) {
		*(void **)itemptr = cursor->node->data;
		return(1);
	}
	return(0);
}

int linked_list_cursor_prev(linked_list_cursor_t *cursor)
{
	if (cursor->node) {
		cursor->node = cursor->node->prev;
		return(1);
	}
	return(0);
}

int linked_list_cursor_next(linked_list_cursor_t *cursor)
{
	if (cursor->node) {
		cursor->node = cursor->node->next;
		return(1);
	}
	return(0);
}

int linked_list_cursor_find(linked_list_cursor_t *cursor, void *key)
{
	linked_list_node_t *node;
	int diff;
	int dir, lastdir;

	if (cursor->list->flags & LINKED_LIST_SORTED) {
		node = cursor->node;
		diff = cursor->list->cmp(key, node->key);
		if (diff > 0) {
			while (node && diff > 0) {
				diff = cursor->list->cmp(key, node->key);
				if (diff) node = node->next;
			}
		}
		else if (diff < 0) {
			while (node && diff < 0) {
				diff = cursor->list->cmp(key, node->key);
				if (diff) node = node->prev;
			}
		}
	}
	else {
		for (node = cursor->list->head; node; node = node->next) {
			diff = cursor->list->cmp(key, node->key);
			if (!diff) break;
		}
	}

	if (node) {
		cursor->node = node;
		return(1);
	}
	else return(0);
}

int linked_list_cursor_prepend(linked_list_cursor_t *cursor, void *key, void *data)
{
	linked_list_node_t *node;

	node = NEW_NODE;
	if (cursor->node) {
		if (node->prev = cursor->node->prev) node->prev->next = node;
		else cursor->list->head = node;
		node->next = cursor->node;
		cursor->node->prev = node;
	}
	else if (cursor->list->head) {
		node->next = cursor->list->head;
		node->prev = (linked_list_node_t *)0;
		node->next->prev = node;
		cursor->list->head = node;
	}
	else {
		cursor->list->head = node;
		cursor->list->tail = node;
		node->next = node->prev = (linked_list_node_t *)0;
	}

	node->key = key;
	node->data = data;
	cursor->node = node;
	return(0);
}

int linked_list_cursor_append(linked_list_cursor_t *cursor, void *key, void *data)
{
	linked_list_node_t *node;

	node = NEW_NODE;
	if (cursor->node) {
		node->prev = cursor->node;
		if (node->next = cursor->node->next) node->next->prev = node;
		else cursor->list->tail = node;
		cursor->node->next = node;
	}
	else if (cursor->list->tail) {
		node->prev = cursor->list->tail;
		node->next = (linked_list_node_t *)0;
		cursor->list->tail = node;
		node->prev->next = node;
	}
	else {
		cursor->list->head = node;
		cursor->list->tail = node;
		node->next = node->prev = (linked_list_node_t *)0;
	}

	node->key = key;
	node->data = data;
	cursor->node = node;
	return(0);
}

int linked_list_cursor_replace(linked_list_cursor_t *cursor, void *key, void *data)
{
	cursor->node->key = key;
	cursor->node->data = data;
	return(0);
}

int linked_list_cursor_delete(linked_list_cursor_t *cursor)
{
	if (cursor->node->prev) cursor->node->prev->next = cursor->node->next;
	else cursor->list->head = cursor->node->next;
	if (cursor->node->next) {
		cursor->node->next->prev = cursor->node->prev;
		cursor->node = cursor->node->next;
	}
	else {
		cursor->list->tail = cursor->node->prev;
		cursor->node = cursor->node->prev;
	}
	return(0);
}

int linked_list_append(linked_list_t *list, void *key, void *data)
{
	linked_list_node_t *node;

	node = NEW_NODE;
	if (node->prev = list->tail) node->prev->next = node;
	node->next = (linked_list_node_t *)0;
	list->tail = node;
	if (!list->head) list->head = node;
	node->key = key;
	node->data = data;
	return(0);
}

int linked_list_prepend(linked_list_t *list, void *key, void *data)
{
	linked_list_node_t *node;

	node = NEW_NODE;
	node->prev = (linked_list_node_t *)0;
	if (list->head) list->head->prev = node;
	node->next = list->head;
	node->key = key;
	node->data = data;
	list->head = node;
	if (!list->tail) list->tail = node;
	return(0);
}

int linked_list_int_cmp(const void *left, const void *right)
{
	return((int)left - (int)right);
}
