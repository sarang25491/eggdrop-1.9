/* serverlist.h: header for serverlist.c
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
 * $Id: serverlist.h,v 1.4 2003/12/18 06:50:47 wcc Exp $
 */

#ifndef _EGG_MOD_SERVER_SERVERLIST_H_
#define _EGG_MOD_SERVER_SERVERLIST_H_

typedef struct server {
	struct server *next;
	char *host;
	int port;
	char *pass;
} server_t;

extern server_t *server_list;
extern int server_list_index;
extern int server_list_len;

server_t *server_get_next();
void server_set_next(int next);
int server_add(char *host, int port, char *pass);
int server_del(int num);
int server_find(const char *host, int port, const char *pass);
int server_clear();

#endif /* !_EGG_MOD_SERVER_SERVERLIST_H_ */
