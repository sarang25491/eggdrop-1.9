/* ircmasks.h: header for ircmasks.c
 *
 * Copyright (C) 2002, 2003, 2004 Eggheads Development Team
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
 *
 * $Id: ircmasks.h,v 1.4 2004/06/22 20:12:37 wingman Exp $
 */

#ifndef _EGG_IRCMASKS_H_
#define _EGG_IRCMASKS_H_

typedef struct ircmask_list_entry {
	struct ircmask_list_entry *next;
	char *ircmask;
	int len;
	void *data;
} ircmask_list_entry_t;

typedef struct {
	ircmask_list_entry_t *head;
} ircmask_list_t;

int ircmask_list_add(ircmask_list_t *list, const char *ircmask, void *data);
int ircmask_list_del(ircmask_list_t *list, const char *ircmask, void *data);
int ircmask_list_find(ircmask_list_t *list, const char *irchost, void *dataptr);
int ircmask_list_clear(ircmask_list_t *list);

#endif /* _EGG_IRCMASKS_H_ */
