/* nicklist.c: manages the bot's list of nicks
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
 */

#ifndef lint
static const char rcsid[] = "$Id: nicklist.c,v 1.6 2003/12/18 06:50:47 wcc Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <stdlib.h>
#include <string.h>

#include "egg_server_api.h"
#include "nicklist.h"
#include "output.h"

char **nick_list = NULL;
int nick_list_index = -1;
int nick_list_len = 0;
int nick_list_cycled = 0;

void try_next_nick()
{
	const char *nick;

	nick = nick_get_next();
	if (!nick) {
		putlog(LOG_MISC, "*", _("try_next_nick: using random nick because %s."), nick_list_len ? _("none of the defined nicks are valid") : _("there are no nicks defined"));
		try_random_nick();
		return;
	}
	printserv(SERVER_QUICK, "NICK %s\r\n", nick);
}

void try_random_nick()
{
	printserv(SERVER_QUICK, "NICK egg%d\r\n", random());
}

void nick_list_on_connect()
{
	nick_list_index = -1;
	nick_list_cycled = 0;
}

/* Get the next nick from the list. */
const char *nick_get_next()
{
	if (nick_list_len <= 0) return(NULL);
	nick_list_index++;
	if (nick_list_index >= nick_list_len) {
		if (nick_list_cycled) return(NULL);
		nick_list_cycled = 1;
		nick_list_index = 0;
	}

	return(nick_list[nick_list_index]);
}

/* Add a nick to the nick list. */
int nick_add(const char *nick)
{
	nick_list = realloc(nick_list, sizeof(char *) * (nick_list_len+1));
	nick_list[nick_list_len] = strdup(nick);
	nick_list_len++;
	return(0);
}

/* Remove a nick from the nick list based on its index. */
int nick_del(int num)
{
	if (num >= 0 && num < nick_list_len) {
		free(nick_list[num]);
		memmove(nick_list+num, nick_list+num+1, nick_list_len-num-1);
	}
	if (num < nick_list_index) nick_list_index--;
	nick_list_len--;

	return(0);
}

/* Clear out the nick list. */
int nick_clear()
{
	int i;

	for (i = 0; i < nick_list_len; i++) free(nick_list[i]);
	if (nick_list) free(nick_list);
	return(0);
}
