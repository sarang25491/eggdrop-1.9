/* events.c -- respond to irc events
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
static const char rcsid[] = "$Id: events.c,v 1.1 2004/12/13 15:02:29 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "chanserv.h"

static int got_join(const char *nick, const char *uhost, user_t *u, const char *chan);

static int got_join(const char *nick, const char *uhost, user_t *u, const char *chan)
{
	if (u) {
		/* Check if we're sending greets. */
		server->printserv(SERVER_NORMAL, "PRIVMSG %s :[%s] <greet goes here>", chan, u->handle);
	}
	return(0);
}

int events_init()
{
	bind_add_simple("join", NULL, NULL, got_join);
	return(0);
}
