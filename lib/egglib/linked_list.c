/*
 * linked_list.c --
 *
 *	linked_list implementation
 *
 *	wishlist:
 *	  * it would be nice if you can specifiy the sort
 *	    direction of a list (e.g. ascendand or descendand)
 *	    (not just by supplying your own comparator)
 */
/*
 * Copyright (C) 1999, 2000, 2001, 2002 Eggheads Development Team
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
static const char rcsid[] = "$Id: linked_list.c,v 1.5 2002/05/05 16:40:33 tothwolf Exp $";
#endif

#include <malloc.h>		/* malloc	*/
#include <string.h>		/* strcmp	*/

#include "mempool.h"		/* mempool_*	*/
#include "linked_list.h"	/* prototypes	*/

static mempool_t *linked_list_node_mempool = NULL;

#define NEW_LIST ((linked_list_t *)malloc(sizeof(linked_list_t)))
#define NEW_NODE ((linked_list_node_t *)mempool_get_chunk( \
					  linked_list_node_mempool \
		 ))
#define KILL_LIST(x) (free((x)))
#define KILL_NODE(x) (mempool_free_chunk(linked_list_node_mempool, (x)))

linked_list_t *linked_list_create(linked_list_t *list,
			          linked_list_cmp_func cmp,
			          int flags)
{
	linked_list_t *mylist;

	if (!linked_list_node_mempool) {
		linked_list_node_mempool = mempool_create(
					     NULL,
					     32,
					     sizeof(linked_list_node_t)
					   );
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

int linked_list_walk(linked_list_t *list,
		     linked_list_node_func func,
		     void *param)
{
	linked_list_node_t *node;

	for (node = list->head; node; node = node->next) {
		func(node->data, param);
	}
	return(0);
}

linked_list_cursor_t *linked_list_cursor_create(linked_list_cursor_t *cursor,
					        linked_list_t *list)
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

int linked_list_delete(linked_list_t *list, void *key) {
   int ret;
   linked_list_cursor_t *cursor;
   
   cursor = linked_list_cursor_create(NULL, list);
   linked_list_cursor_home(cursor);
   if ((ret = linked_list_cursor_find(cursor, key)) != 1) {
     linked_list_cursor_destroy(cursor);
     return ret;
   }

   linked_list_cursor_delete(cursor);
   linked_list_cursor_destroy(cursor);

   return ret;
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

	if (cursor->list->flags & LINKED_LIST_SORTED && cursor->node) {
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

int linked_list_cursor_prepend(linked_list_cursor_t *cursor,
			       void *key,
			       void *data)
{
	linked_list_node_t *node;

	node = NEW_NODE;
	if (cursor->node) {
		if ((node->prev = cursor->node->prev)) node->prev->next = node;
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

int linked_list_cursor_append(linked_list_cursor_t *cursor,
			      void *key,
			      void *data)
{
	linked_list_node_t *node;

	node = NEW_NODE;
	if (cursor->node) {
		node->prev = cursor->node;
		if ((node->next = cursor->node->next)) node->next->prev = node;
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

int linked_list_cursor_replace(linked_list_cursor_t *cursor,
			       void *key,
			       void *data)
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
	if ((node->prev = list->tail)) node->prev->next = node;
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

int linked_list_char_cmp(const void *left, const void *right) {
  int ret;

  if (left == NULL && right == NULL) 
    ret = 0;
  else if (left != NULL && right == NULL) 
    ret = -1;
  else if (left == NULL && right != NULL)
    ret = 1;
  else
    ret = strcmp((const char *)left, (const char *)right);

  return ret;
}

int linked_list_int_cmp(const void *left, const void *right)
{
	return((int)left - (int)right);
}
