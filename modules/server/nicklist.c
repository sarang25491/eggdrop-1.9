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
static const char rcsid[] = "$Id: nicklist.c,v 1.10 2004/06/24 06:19:56 wcc Exp $";
#endif

#include "server.h"

char **nick_list = NULL;
int nick_list_index = -1;
int nick_list_len = 0;
int nick_list_cycled = 0;

void try_next_nick()
{
	const char *nick;

	nick = nick_get_next();
	if (!nick) {
		putlog(LOG_MISC, "*", _("Using random nick because %s."),
		       nick_list_len ? _("none of the defined nicks are valid") : _("there are no nicks defined"));
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
	if (num < 0 || num >= nick_list_len)
		return(-1); /* Invalid nick index. */

	free(nick_list[num]);
	memmove(nick_list + num, nick_list + num + 1, nick_list_len - num - 1);
	nick_list_len--;
	if (num < nick_list_index)
		nick_list_index--;

	return(0);
}

/* Clear out the nick list. */
int nick_clear()
{
	int i;

	for (i = 0; i < nick_list_len; i++)
		free(nick_list[i]);
	if (nick_list)
		free(nick_list);
	nick_list = NULL;
	nick_list_len = 0;

	return(0);
}

/* Return the index of the first matching server in the list. */
int nick_find(const char *nick)
{
	int i;

	for (i = 0; i < nick_list_len; i++)
		if (!nick || !strcasecmp(nick_list[i], nick))
			return(i);
	return(-1);
}
