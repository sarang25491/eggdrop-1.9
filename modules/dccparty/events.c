/* events.c: dccparty events
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
static const char rcsid[] = "$Id: events.c,v 1.4 2003/12/18 03:54:45 wcc Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <string.h>
#include "dccparty.h"

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len);
static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick);
static int on_quit(void *client_data, partymember_t *src, const char *text, int len);
static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
static int on_join(void *client_data, partychan_t *chan, partymember_t *src);
static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);

partyline_event_t dcc_party_handler = {
	on_privmsg,
	on_nick,
	on_quit,
	on_chanmsg,
	on_join,
	on_part
};

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len)
{
	dcc_session_t *session = client_data;

	if (src) egg_iprintf(session->idx, "[%s] %s\r\n", src->nick, text);
	else egg_iprintf(session->idx, "%s\r\n", text);
	return(0);
}

static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick)
{
	dcc_session_t *session = client_data;

	egg_iprintf(session->idx, "%s is now known as %s.\n", oldnick, newnick);
	return(0);
}

static int on_quit(void *client_data, partymember_t *src, const char *text, int len)
{
	dcc_session_t *session = client_data;

	egg_iprintf(session->idx, "%s (%s@%s) has quit: %s\n", src->nick, src->ident, src->host, text);
	if (src == session->party) sockbuf_delete(session->idx);

	return(0);
}

static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	dcc_session_t *session = client_data;

	if (src) egg_iprintf(session->idx, "%s <%s> %s\r\n", chan->name, src->nick, text);
	else egg_iprintf(session->idx, "%s %s\r\n", chan->name, text);
	return(0);
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src)
{
	dcc_session_t *session = client_data;

	egg_iprintf(session->idx, "%s %s (%s@%s) has joined the channel.\r\n", chan->name, src->nick, src->ident, src->host);
	return(0);
}

static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	dcc_session_t *session = client_data;

	egg_iprintf(session->idx, "%s %s (%s@%s) has left the channel: %s\r\n", chan->name, src->nick, src->ident, src->host, text);
	return(0);
}
