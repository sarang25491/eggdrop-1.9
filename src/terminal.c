/* terminal.c: terminal mode support 
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
static const char rcsid[] = "$Id: terminal.c,v 1.1 2004/06/20 13:33:48 wingman Exp $";
#endif

#include <eggdrop/eggdrop.h>			/* partyline_*		*/
#include "terminal.h"				/* protoypes		*/

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len);
static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick);
static int on_quit(void *client_data, partymember_t *src, const char *text, int len);
static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
static int on_join(void *client_data, partychan_t *chan, partymember_t *src);
static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
static int on_read(void *client_data, int idx, char *data, int len);

static sockbuf_handler_t terminal_sockbuf_handler = {
	"terminal",
	NULL,
	NULL,
	NULL,
	on_read,
	NULL,
	NULL
};

static partyline_event_t terminal_party_handler = {
	on_privmsg,
	on_nick,
	on_quit,
	on_chanmsg,
	on_join,
	on_part
};

struct {
	int idx;
	partymember_t *party;
} terminal_session;

static user_t *terminal_user = NULL;

void terminal_init(void)
{
	putlog(LOG_MISC, "*", "Entering terminal mode.");

	if (terminal_user == NULL) {
		terminal_user = user_lookup_by_handle (TERMINAL_HANDLE);
		if (terminal_user == NULL) {
			terminal_user = user_new (TERMINAL_HANDLE);
			user_set_flags_str (terminal_user, NULL, "n");
 		}
	}

	terminal_session.idx = sockbuf_new();
	sockbuf_set_sock(terminal_session.idx, fileno(stdin), SOCKBUF_CLIENT);
	sockbuf_set_handler(terminal_session.idx, &terminal_sockbuf_handler, NULL);
	linemode_on(terminal_session.idx);
	
	terminal_session.party = partymember_new (-1,
		terminal_user, TERMINAL_NICK, TERMINAL_USER,
			TERMINAL_HOST, &terminal_party_handler, NULL);

	/* Put them on the main channel. */
	partychan_join_name("*", terminal_session.party);	
}

static int on_read(void *client_data, int idx, char *data, int len)
{
	partyline_on_input(NULL, terminal_session.party, data, len);
	return 0;
}

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len)
{
	return partyline_idx_privmsg(terminal_session.idx, dest, src, text, len);
}

static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick)
{
	return partyline_idx_nick (terminal_session.idx, src, oldnick, newnick);
}

static int on_quit(void *client_data, partymember_t *src, const char *text, int len)
{
	partyline_idx_quit(terminal_session.idx, src, text, len);

	/* if this quit are we delete our sockbuf. */
	if (src == terminal_session.party)
		sockbuf_delete(terminal_session.idx);

	return(0);
}

static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	return partyline_idx_chanmsg(terminal_session.idx, chan, src, text, len);
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src)
{
	return partyline_idx_join(terminal_session.idx, chan, src);
}

static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	return partyline_idx_part(terminal_session.idx, chan, src, text, len);
}

