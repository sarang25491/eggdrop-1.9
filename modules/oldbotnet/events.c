/* events.c: oldbotnet events
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
static const char rcsid[] = "$Id: events.c,v 1.6 2004/10/04 16:05:32 stdarg Exp $";
#endif

#include <ctype.h>
#include <stdlib.h>

#include <eggdrop/eggdrop.h>

#include "oldbotnet.h"

static int on_pub(const char *name, int cid, partymember_t *src, const char *text, int len);
static int on_join(const char *name, int cid, partymember_t *src);
static int on_part(const char *name, int cid, partymember_t *src, const char *text, int len);

int oldbotnet_events_init()
{
	bind_add_simple(BTN_PARTYLINE_PUBLIC, NULL, NULL, on_pub);
	bind_add_simple(BTN_PARTYLINE_JOIN, NULL, NULL, on_join);
	bind_add_simple(BTN_PARTYLINE_PART, NULL, NULL, on_part);
	return(0);
}

/* Taken from botmsg.c in eggdrop 1.6. */
static char tobase64array[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'[', ']'
};

/* Modified from int_to_base64() in botmsg.c in eggdrop 1.6. */
static char *ito64(int val)
{
	static char s[16];
	char *ptr = s;

	do {
		*ptr++ = tobase64array[val & 0x3f];
		val = val >> 6;
	} while (val);
	*ptr = 0;
	return(s);
}

static char *sto64(const char *s)
{
	int val = atoi(s);
	return ito64(val);
}

static int validchan(const char *chan)
{
	while (*chan) if (!isdigit(*chan++)) return(0);
	return(1);
}

#if 0
static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len)
{
	return(0);
}

static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick)
{
	return(0);
}

static int on_quit(void *client_data, partymember_t *src, const char *text, int len)
{
	return(0);
}
#endif

static int on_pub(const char *name, int cid, partymember_t *src, const char *text, int len)
{
	if (!oldbotnet.connected || !validchan(name)) return(0);

	if (!src) egg_iprintf(oldbotnet.idx, "c %s %s %s\n", oldbotnet.name, sto64(name), text);
	else if (src->pid < BOT_PID_MULT) egg_iprintf(oldbotnet.idx, "c %s@%s %s %s\n", src->nick, oldbotnet.name, sto64(name), text);

	return(0);
}

static int on_join(const char *name, int cid, partymember_t *src)
{
	if (!oldbotnet.connected || !validchan(name) || src->pid >= BOT_PID_MULT) return(0);

	egg_iprintf(oldbotnet.idx, "j %s %s %s *%s %s@%s\n", oldbotnet.name, src->nick, sto64(name), ito64(src->pid), src->ident, src->host);

	return(0);
}

static int on_part(const char *name, int cid, partymember_t *src, const char *text, int len)
{
	if (!oldbotnet.connected || !validchan(name) || src->pid >= BOT_PID_MULT) return(0);

	egg_iprintf(oldbotnet.idx, "pt %s %s %s %s\n", oldbotnet.name, src->nick, ito64(src->pid), text ? text : "");

	return(0);
}
