/*
 * linked_list.h
 *   linked_list implementation
 *
 *   wishlist:
 *     * it would be nice if you can specifiy the sort
 *       direction of a list (e.g. ascendand or descendand)
 *       (not just by supplying your own comparator)
 *
 * $Id: linked_list.h,v 1.2 2002/05/04 17:01:36 wingman Exp $
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
#ifndef _EGG_LINKED_LIST_H_
#define _EGG_LINKED_LIST_H_

/*
 * sort types
 */
#define LINKED_LIST_SORTED	1
#define LINKED_LIST_DOUBLE	2
#define LINKED_LIST_STATIC	4

/*
 * for loop 
 */
#define LINKED_LIST_FORLOOP(x, y) \
		 linked_list_cursor_home(x); \
			linked_list_cursor_get(x, y); \
				linked_list_cursor_next(x)

/*
 * compare function prototype
 *   returns a negatve integer, zero or a positive integer as left is
 *   less than, equal to or greater than right.
 */ 
typedef int (*linked_list_cmp_func)(const void *left, const void *right);

/*
 * node callback
 *   (used e.g. on linked_list_walk)
 */
typedef void (*linked_list_node_func)(void *data, void *param);

/*
 * list node entry
 */
typedef struct linked_list_node_b {
	void *key;
	void *data;
	struct linked_list_node_b *next, *prev;
} linked_list_node_t;

/*
 * list
 */
typedef struct linked_list_b {
	linked_list_cmp_func cmp;
	linked_list_node_t *head, *tail;
	int flags;
} linked_list_t;

/*
 * list cursor
 */
typedef struct linked_list_cursor_b {
	linked_list_node_t *node;
	linked_list_t *list;
	int flags;
} linked_list_cursor_t;


/*
 * creates (allocates) a new linked_list_t
 *
 *   parameters:
 *
 *     linked_list_t *list:
 *       liste to be based on (or NULL if you want to
 *       start with a fresh new list)
 *
 *     linked_list_cmp_func func:
 * 
 *       comparator function (see linked_list_cmp_func)
 *
 *     int flags:
 *
 *        sorting flags (can be either LINKED_LIST_SORTED, 
 *        LINKED_LIST_DOUBLE or LINKED_LIST_STATIC)
 */
linked_list_t *linked_list_create(linked_list_t *list,
				  linked_list_cmp_func func,
				  int flags);

/*
 * destroys (frees) a linked list
 */
int linked_list_destroy(linked_list_t *list);

/*
 * walk through the beginning to the end of the specified list.
 * On each node found, callback is called.
 */
int linked_list_walk(linked_list_t *list,
                     linked_list_node_func callback,
                     void *param);

/*
 * creates (allocates) a cursor for the given list
 */
linked_list_cursor_t *linked_list_cursor_create(linked_list_cursor_t *cursor,
					        linked_list_t *list);

/*
 * destroys (frees) a cursor
 */ 
int linked_list_cursor_destroy(linked_list_cursor_t *cursor);

/*
 * saves the data of the current cursor in itemptr.
 *   returns:
 *     1 if cursor contains data
 *     0 if not 
 */
int linked_list_cursor_get(linked_list_cursor_t *cursor, void *itemptr);

/*
 * sets the cursor to the beginning our his list
 */
int linked_list_cursor_home(linked_list_cursor_t *cursor);

/*
 * sets the cursor to the end of his list
 */
int linked_list_cursor_end(linked_list_cursor_t *cursor);

/*
 * prepends an key/data item to the start of cursor's list
 */
int linked_list_cursor_prepend(linked_list_cursor_t *cursor,
			       void *key,
			       void *data);

/*
 * appends a key/data item to the end of cursor's list
 */
int linked_list_cursor_append(linked_list_cursor_t *cursor,
			      void *key,
			      void *data);

/* 
 * moves cursor to the previous item in the list
 *   returns:
 *     1: if current item is not the fisrt item
 *     0: if there are no more items
 */
int linked_list_cursor_prev(linked_list_cursor_t *cursor);

/*
 * moves cursor to the next item in the list
 *   returns:
 *    1: if current item is no the last item
 *    0: if there are no more items
 */
int linked_list_cursor_next(linked_list_cursor_t *cursor);

/*
 * searches for the given key in cursor's list
 *   returns:
 *     1: if key is found
 *     0: if cursor's list doesn't contain an item with this key
 */
int linked_list_cursor_find(linked_list_cursor_t *cursor, void *key);

/*
 * replaces the given key with the new data
 */
int linked_list_cursor_replace(linked_list_cursor_t *cursor,
			       void *key,
			       void *data);

/*
 * delete the current cursor's item and moves the cursor to the
 * next item (if any, else to it's previous item).
 */
int linked_list_cursor_delete(linked_list_cursor_t *cursor);

/*
 * append the given key/data item to the end of the list
 */
int linked_list_append(linked_list_t *list, void *key, void *data);

/*
 * prepend the given key/data item to the beginning of the list
 */
int linked_list_prepend(linked_list_t *list, void *key, void *data);

/*
 * wingman: i don't get the idea behind linked_list_insert since
 * we already have append/prepend (perhaps that's the reason
 * it isn't implemented?)
 */
//int linked_list_insert(linked_list_t *list, void *key, void *data);

/*
 * removes the given key from the list
 * (wingman: uhm, for what purpose should a delete function contain
 * a callback?)
 */
//int linked_list_delete(linked_list_t *list,
//		       void *key,
//		       linked_list_node_func callback);
int linked_list_delete(linked_list_t *list, void *key);

/*
 * an int comparator (sorts ascending)
 */
int linked_list_int_cmp(const void *left, const void *right);

/*
 * an char comparator (sorts ascending)
 */
int linked_list_char_cmp(const void *left, const void *right);

#endif
