/* ircparse.c: functions for parsing irc messages
 *
 * Copyright (C) 2003, 2004 Eggheads Development Team
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
static const char rcsid[] = "$Id: ircparse.c,v 1.2 2003/12/17 07:39:14 wcc Exp $";
#endif

#include <stdlib.h>
#include <string.h>
#include "ircparse.h"

/* Take care of array manipulation in adding an argument. */
static void add_arg(irc_msg_t *msg, char *text)
{
	if (msg->nargs >= IRC_MSG_NSTATIC_ARGS) {
		if (msg->nargs > IRC_MSG_NSTATIC_ARGS) {
			msg->args = (char **)realloc(msg->args, sizeof(char *) * (msg->nargs+1));
		}
		else {
			msg->args = (char **)malloc(sizeof(char *) * (IRC_MSG_NSTATIC_ARGS+1));
			memcpy(msg->args, msg->static_args, sizeof(char *) * IRC_MSG_NSTATIC_ARGS);
		}
	}
	msg->args[msg->nargs] = text;
	msg->nargs++;
}

/* Parses an irc message into standard parts. Be sure to call irc_msg_cleanup()
 * when you are done using it, so that it can free the argument array if
 * necessary. */
void irc_msg_parse(char *text, irc_msg_t *msg)
{
	char *space;

	memset(msg, 0, sizeof(*msg));
	msg->args = msg->static_args;
	if (*text == ':') {
		msg->prefix = text+1;
		space = strchr(msg->prefix, ' ');
		if (!space) return;
		*space = 0;
		text = space+1;
	}
	msg->cmd = text;
	space = strchr(text, ' ');
	if (!space) return;
	*space = 0;
	text = space+1;

	while (*text && (*text != ':')) {
	       	space = strchr(text, ' ');
		add_arg(msg, text);
		if (space) {
			*space = 0;
			text = space+1;
		}
		else break;
	}
	if (*text == ':') add_arg(msg, text+1);
	add_arg(msg, NULL);
	msg->nargs--;
}

/* Replace nulls with spaces again, in case you want to re-use the text. */
void irc_msg_restore(irc_msg_t *msg)
{
	int i;
	for (i = 0; i < msg->nargs; i++) {
		if (msg->args[i][-1] == ':') msg->args[i][-2] = ' ';
		else msg->args[i][-1] = ' ';
	}
}

/* Reset the argument array, freeing any dynamic memory. */
void irc_msg_cleanup(irc_msg_t *msg)
{
	if (msg->args && msg->args != msg->static_args) free(msg->args);
	msg->args = msg->static_args;
	msg->nargs = 0;
}
