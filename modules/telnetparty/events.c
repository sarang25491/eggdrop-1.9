/* events.c: events for telnet partyline interface
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
static const char rcsid[] = "$Id: events.c,v 1.9 2004/06/21 11:33:40 wingman Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include "telnetparty.h"

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len);
static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick);
static int on_quit(void *client_data, partymember_t *src, const char *text, int len);
static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
static int on_join(void *client_data, partychan_t *chan, partymember_t *src);
static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);

partyline_event_t telnet_party_handler = {
	on_privmsg,
	on_nick,
	on_quit,
	on_chanmsg,
	on_join,
	on_part
};

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len)
{
	return partyline_idx_privmsg (((telnet_session_t *)client_data)->idx, dest, src, text, len);
}

static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick)
{
	return partyline_idx_nick (((telnet_session_t *)client_data)->idx, src, oldnick, newnick);
}

static int on_quit(void *client_data, partymember_t *src, const char *text, int len)
{
	telnet_session_t *session = client_data;

	partyline_idx_quit(session->idx, src, text, len);

	/* if this quit are we delete our sockbuf. */
	if (src == session->party)
		sockbuf_delete(session->idx);

	return(0);
}

static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	return partyline_idx_chanmsg (((telnet_session_t *)client_data)->idx, chan, src, text, len);
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src)
{
	return partyline_idx_join(((telnet_session_t *)client_data)->idx, chan, src);
}

static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	return partyline_idx_part(((telnet_session_t *)client_data)->idx, chan, src, text, len);
}
