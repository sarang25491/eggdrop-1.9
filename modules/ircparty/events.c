/* events.c: ircparty events
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
static const char rcsid[] = "$Id: events.c,v 1.10 2007/01/13 12:23:40 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "ircparty.h"

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len);
static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick);
static int on_quit(void *client_data, partymember_t *src, const botnet_bot_t *lostbot, const char *text, int len);
static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking);
static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);

partyline_event_t irc_party_handler = {
	on_privmsg,
	on_nick,
	on_quit,
	on_chanmsg,
	on_join,
	on_part
};

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len)
{
	irc_session_t *session = client_data;

	if (src && src->nick && src->bot) egg_iprintf(session->idx, ":%s*%s:%d!%s@%s PRIVMSG %s :%s\r\n", src->nick, src->bot->name, src->id, src->ident, src->host, dest->nick, text);
	else if (src && src->nick) egg_iprintf(session->idx, ":%s:%d!%s@%s PRIVMSG %s :%s\r\n", src->nick, src->id, src->ident, src->host, dest->nick, text); 
	else if (src) egg_iprintf(session->idx, ":%s!%s@%s PRIVMSG %s :%s\r\n", src->bot->name, src->bot->name, src->bot->name, dest->nick, text);
	else egg_iprintf(session->idx, ":* NOTICE * :%s\r\n", text);
	return(0);
}

static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick)
{
	irc_session_t *session = client_data;

	if (src != session->party && src->bot) egg_iprintf(session->idx, ":%s*%s:%d!%s@%s NICK %s*%s:%d\r\n", oldnick, src->bot->name, src->id, src->ident, src->host, newnick, src->bot->name, src->id);
	else if (src != session->party) egg_iprintf(session->idx, ":%s:%d!%s@%s NICK %s:%d\r\n", oldnick, src->id, src->ident, src->host, newnick, src->bot->name, src->id); 
	else egg_iprintf(session->idx, ":%s!%s@%s NICK %s\r\n", oldnick, src->ident, src->host, newnick);
	return(0);
}

static int on_quit(void *client_data, partymember_t *src, const botnet_bot_t *lostbot, const char *text, int len)
{
	irc_session_t *session = client_data;

	if (src != session->party && src->bot) egg_iprintf(session->idx, ":%s*%s:%d!%s@%s QUIT :%s\r\n", src->nick, src->bot->name, src->id, src->ident, src->host, text);
	else if (src != session->party) egg_iprintf(session->idx, ":%s:%d!%s@%s QUIT :%s\r\n", src->nick, src->id, src->ident, src->host, text);
	else egg_iprintf(session->idx, "ERROR :%s\r\n", text);

	return(0);
}

static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	irc_session_t *session = client_data;

	if (src == session->party) return 0;
	if (src && src->nick && src->bot) egg_iprintf(session->idx, ":%s*%s:%d!%s@%s PRIVMSG #%s :%s\r\n", src->nick, src->bot->name, src->id, src->ident, src->host, chan->name, text);
	else if (src && src->nick) egg_iprintf(session->idx, ":%s:%d!%s@%s PRIVMSG #%s :%s\r\n", src->nick, src->id, src->ident, src->host, chan->name, text);
	else if (src) egg_iprintf(session->idx, ":%s!%s@%s PRIVMSG #%s :%s\r\n", src->bot->name, src->bot->name, src->bot->name, chan->name, text);
	else egg_iprintf(session->idx, ":* PRIVMSG #%s :%s\r\n", chan->name, text);
	return 0;
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking)
{
	irc_session_t *session = client_data;
	partychan_member_t *m;
	partymember_t *p;
	int i, cur, len;
	char buf[510], nick[128];

	if (session->party != src) {
		if (src->bot) egg_iprintf(session->idx, ":%s*%s:%d!%s@%s JOIN #%s\r\n", src->nick, src->bot->name, src->id, src->ident, src->host, chan->name);
		else egg_iprintf(session->idx, ":%s:%d!%s@%s JOIN #%s\r\n", src->nick, src->id, src->ident, src->host, chan->name);
		return(0);
	}

	egg_iprintf(session->idx, ":%s!%s@%s JOIN #%s\r\n", src->nick, src->ident, src->host, chan->name);

	sprintf(buf, ":eggdrop.bot 353 %s @ #%s :", session->nick, chan->name);
	cur = strlen(buf);

	for (i = 0; i < chan->nmembers; i++) {
		m = chan->members+i;
		if (chan->members[i].flags & PARTY_DELETED) continue;
		p = m->p;
		if (session->party == p) snprintf(nick, sizeof(nick), "%s", p->nick);
		else if (p->bot) snprintf(nick, sizeof(nick), "%s*%s:%d", p->nick, p->bot->name, p->id);
		else snprintf(nick, sizeof(nick), "%s:%d", p->nick, p->id);
		len = strlen(nick);
		if (cur + len > 500) {
			cur--;
			buf[cur] = 0;
			egg_iprintf(session->idx, "%s\r\n", buf);
			sprintf(buf, ":eggdrop.bot 353 %s @ #%s :", session->nick, chan->name);
			cur = strlen(buf);
		}
		strcpy(buf+cur, nick);
		cur += len;
		buf[cur++] = ' ';
	}

	if (cur > 0) {
		cur--;
		buf[cur] = 0;
		egg_iprintf(session->idx, "%s\r\n", buf);
	}
	egg_iprintf(session->idx, ":eggdrop.bot 366 %s #%s :End of /NAMES list.\r\n", session->nick, chan->name);
	return(0);
}

static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	irc_session_t *session = client_data;

	if (session->party == src) egg_iprintf(session->idx, ":%s!%s@%s PART #%s :%s\r\n", src->nick, src->ident, src->host, chan->name, text);
	else if (src->bot) egg_iprintf(session->idx, ":%s*%s:%d!%s@%s PART #%s :%s\r\n", src->nick, src->bot->name, src->id, src->ident, src->host, chan->name, text);
	else egg_iprintf(session->idx, ":%s:%d!%s@%s PART #%s :%s\r\n", src->nick, src->id, src->ident, src->host, chan->name, text);
	return(0);
}
