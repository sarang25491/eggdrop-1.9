/*
 * linked_list_test.c
 *   examples for usage of linked_list
 *
 * $Id: linked_list_test.c,v 1.2 2002/05/04 17:01:36 wingman Exp $
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

#include <stdio.h>		/* printf	*/
#include "linked_list.h"	/* linked_list	*/

int test_list_int();
int test_list_char();

int main(int argc, char *argv[]) {
  
  // test an int list
  test_list_int();

  // test a char list
  test_list_char();

  return 0;
}

int test_list_char() {
  linked_list_t *list;
  linked_list_cursor_t *cursor;
  int i;
  char *data;

  // create a new list
  list = linked_list_new(
           NULL,                 /* original list               */
           linked_list_char_cmp, /* default char comparator     */
           LINKED_LIST_SORTED    /* sort list                   */
         );

  // add 5 entries
  for (i = 0; i < 5; i++) {
    data = (char *)malloc(10);
    sprintf(data, "item %i", i);
    linked_list_append(list, (void *)data, (void *)data);
  }

  // create a cursor for cycling/searching
  cursor = linked_list_cursor_new(NULL, list);

  // walk trough listh
  for (LINKED_LIST_FORLOOP(cursor, &data))
    printf("%s\n", data);
  
  // scroll to item 4
  printf("finding 'item 4'\n");
  if (linked_list_cursor_find(cursor, "item 4") != 1) {
    printf("didn't find item item 4\n");
    return;
  }
  
  printf("replacing item 'item 4' with 'item 4 (new)'\n");

  // replace it
  linked_list_cursor_replace(cursor, "item 4", "item 4 (new)");
  printf("done\n");

  // show it again
  for (LINKED_LIST_FORLOOP(cursor, &data))
    printf("%s\n", data);

  printf("deleting 'item 2'\n");
  linked_list_cursor_find(cursor, "item 2");
  linked_list_cursor_delete(cursor); 

  // and again
  for (LINKED_LIST_FORLOOP(cursor, &data)) 
    printf("%s\n", data);
}

int test_list_int() {
  linked_list_t *list;
  linked_list_cursor_t *cursor;
  int i, data;

  // create a new list
  list = linked_list_new(
	   NULL,		 /* original list		*/
	   linked_list_int_cmp,	 /* default int comparator	*/
	   LINKED_LIST_SORTED	 /* sort list			*/
	 );

  // add 10 entries
  for (i = 0; i < 10; i++) {
    linked_list_append(list, (void *)i, (void *)i);
  } 

  // create a cursor for cycling/searching
  cursor = linked_list_cursor_new(NULL, list);

  // walk trough listh
  for (LINKED_LIST_FORLOOP(cursor, &data)) {
    printf("%d\n", data);
  }

}
