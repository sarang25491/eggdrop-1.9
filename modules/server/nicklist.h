/* nicklist.h: header for nicklist.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: nicklist.h,v 1.4 2004/06/24 06:19:56 wcc Exp $
 */

#ifndef _EGG_MOD_SERVER_NICKLIST_H_
#define _EGG_MOD_SERVER_NICKLIST_H_

void try_next_nick();
void try_random_nick();
void nick_list_on_connect();
const char *nick_get_next();
int nick_add(const char *);
int nick_del(int);
int nick_clear();
int nick_find(const char *);

extern char **nick_list;
extern int nick_list_len;

#endif /* !_EGG_MOD_SERVER_NICKLIST_H_ */
