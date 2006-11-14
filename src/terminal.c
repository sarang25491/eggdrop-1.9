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
static const char rcsid[] = "$Id: terminal.c,v 1.7 2006/11/14 14:51:24 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>			/* partyline_*		*/
#include "terminal.h"				/* protoypes		*/

extern int terminal_enabled; /* from logfile.c */

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len);
static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick);
static int on_quit(void *client_data, partymember_t *src, const botnet_bot_t *lostbot, const char *text, int len);
static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking);
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
	int in_idx, out_idx;
	partymember_t *party;
} terminal_session = {-1, -1, NULL};

static user_t *terminal_user = NULL;

int terminal_init(void)
{
	if (terminal_user == NULL) {
		terminal_user = user_lookup_by_handle(TERMINAL_HANDLE);
		if (terminal_user == NULL) {
			terminal_user = user_new(TERMINAL_HANDLE);
			user_set_flags_str(terminal_user, NULL, "n");
 		}
	}

	/* Connect an idx to stdin. */
	terminal_session.in_idx = sockbuf_new();
	sockbuf_set_sock(terminal_session.in_idx, dup(fileno(stdin)), SOCKBUF_CLIENT);
	sockbuf_set_handler(terminal_session.in_idx, &terminal_sockbuf_handler, NULL);
	linemode_on(terminal_session.in_idx);

	/* And one for stdout. */
	terminal_session.out_idx = sockbuf_new();
	sockbuf_set_sock(terminal_session.out_idx, dup(fileno(stdout)), SOCKBUF_NOREAD);

	putlog(LOG_MISC, "*", _("Entering terminal mode."));

	terminal_session.party = partymember_new(-1,
		terminal_user, NULL, TERMINAL_NICK, TERMINAL_USER,
			TERMINAL_HOST, &terminal_party_handler, NULL);

	/* Put them on the main channel. */
	partychan_join_name("*", terminal_session.party, 0);

	return (0);
}

#define RUNMODE_RESTART 3

int terminal_shutdown(int runmode)
{
	if (runmode == RUNMODE_RESTART) {
		terminal_user = NULL;
		return(0);
	}

	if (terminal_session.party) {
		partymember_delete(terminal_session.party, NULL, _("Shutdown"));
		terminal_session.party = NULL;
	}

	if (terminal_session.in_idx != -1) {
		sockbuf_set_sock(terminal_session.in_idx, -1, 0);
		sockbuf_delete(terminal_session.in_idx);
		terminal_session.in_idx = -1;
	}

	if (terminal_session.out_idx != -1) {
		sockbuf_set_sock(terminal_session.out_idx, -1, 0);
		sockbuf_delete(terminal_session.out_idx);
		terminal_session.out_idx = -1;
	}

	if (terminal_user) {
		user_delete(terminal_user);
		terminal_user = NULL;
	}

	/* let logfile.c know that there's no longer the faked
	 * stdout sockbuf of our HQ partymember */
	terminal_enabled = 0;

	putlog(LOG_MISC, "*", _("Leaving terminal mode."));

	return (0);
}

static int on_read(void *client_data, int idx, char *data, int len)
{
	partyline_on_input(NULL, terminal_session.party, data, len);
	return 0;
}

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len)
{
	return partyline_idx_privmsg(terminal_session.out_idx, dest, src, text, len);
}

static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick)
{
	return partyline_idx_nick (terminal_session.out_idx, src, oldnick, newnick);
}

static int on_quit(void *client_data, partymember_t *src, const botnet_bot_t *lostbot, const char *text, int len)
{
	if (lostbot) return 0;
	partyline_idx_quit(terminal_session.out_idx, src, text, len);

	/* if this quit are we delete our sockbuf. */
	if (src == terminal_session.party)
		sockbuf_delete(terminal_session.out_idx);

	return(0);
}

static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	return partyline_idx_chanmsg(terminal_session.out_idx, chan, src, text, len);
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking)
{
	if (linking) return 0;
	return partyline_idx_join(terminal_session.out_idx, chan, src);
}

static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	return partyline_idx_part(terminal_session.out_idx, chan, src, text, len);
}
