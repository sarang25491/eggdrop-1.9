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
static const char rcsid[] = "$Id: events.c,v 1.10 2007/04/18 01:45:52 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "oldbotnet.h"

/* botnet callbacks */
static int on_bcast(void *client_data, botnet_entity_t *src, const char *text, int len);
static int on_privmsg(void *client_data, botnet_entity_t *src, partymember_t *dst, const char *text, int len);
static int on_quit(void *client_data, partymember_t *src, const char *text, int len);
static int on_chanmsg(void *client_data, partychan_t *chan, botnet_entity_t *src, const char *text, int len);
static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking);
static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *reason, int len);
static int on_new_bot(void *client_data, botnet_bot_t *bot, int linking);
static int on_lost_bot(void *client_data, botnet_bot_t *bot, const char *reason);
static int on_unlink(void *client_data, botnet_entity_t *from, botnet_bot_t *bot, const char *reason);
static int on_botmsg(void *client_data, botnet_bot_t *src, botnet_bot_t *dst, const char *command, const char *text, int len);
static int on_botbroadcast(void *client_data, botnet_bot_t *src, const char *command, const char *text, int len);
static int on_extension(void *client_data, botnet_entity_t *src, botnet_bot_t *dst, const char *cmd, const char *text, int len);

botnet_handler_t bothandler = {
	on_bcast, on_privmsg, NULL, on_quit,
	on_chanmsg, on_join, on_part,
	on_new_bot, on_lost_bot, on_unlink, on_botmsg, on_botbroadcast, on_extension
};

static int tobase64[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '[', ']'
};

/*!
 * \brief Calculate the base64 value of an int.
 *
 * Uses the eggdrop1.6 style base64 algotithm to convert an int to a
 * base64 string.
 *
 * \param i The number to convert.
 *
 * \return A string containing the base64 value of \e i.
 *
 * \warning This function returns a pointer to a static buffer. Calling it
 *          again will overwrite it. \b Always \b remember!
 */

static const char *itob(int i)
{
	char *pos;
	static char ret[12];

	pos = ret + 11;
	*pos = 0;

	do {
		--pos;
		*pos = tobase64[i & 0x3F];
		i >>= 6;
	} while (i);

	return pos;
}

/*!
 * \brief Send a broadcast event out.
 *
 * Send a broadcast (ct) out. eggdrop1.6 expects broadcasts to originate from
 * a bot. If the source is a person, change it to the bot the person is on.
 *
 * \param client_data The ::oldbotnet_t struct of the bot.
 * \param src The source of the broadcast.
 * \param text The string to broadcast.
 * \param len The string length.
 *
 * \return Always 0.
 */

static int on_bcast(void *client_data, botnet_entity_t *src, const char *text, int len)
{
	const char *name;
	oldbotnet_t *obot = client_data;

	if (src->what == ENTITY_BOT) name = src->full_name;
	else if (src->user->bot) name = src->user->bot->name;
	else name = botnet_get_name();

	egg_iprintf(obot->idx, "ct %s %s\n", name, text);

	return 0;
}

static int on_privmsg(void *client_data, botnet_entity_t *src, partymember_t *dst, const char *text, int len)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "p %s %s %s\n", src->full_id_name, dst->full_id_name, text);

	return 0;
}

static int on_quit(void *client_data, partymember_t *src, const char *text, int len)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "pt %s %s %s\n", src->bot ? src->bot->name : botnet_get_name(), src->nick, itob(src->id));

	return 0;
}

static int on_chanmsg(void *client_data, partychan_t *chan, botnet_entity_t *src, const char *text, int len)
{
	oldbotnet_t *obot = client_data;

	if (len >= 9 && !strncasecmp(text, "\1ACTION ", 8) && text[len - 1] == 1) {
		egg_iprintf(obot->idx, "a %s %s %.*s\n", src->full_name, itob(assoc_get_id(chan->name)), len - 9, text + 8);
	} else {
		egg_iprintf(obot->idx, "c %s %s %s\n", src->full_name, itob(assoc_get_id(chan->name)), text);
	}
	return 0;
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking)
{
	char *cid;
	oldbotnet_t *obot = client_data;

	cid = strdup(itob(assoc_get_id(chan->name)));
	egg_iprintf(obot->idx, "j %s%s %s %s %c%s %s@%s\n", linking ? "!" : "", src->bot ? src->bot->name : botnet_get_name(), src->nick, cid, '*', itob(src->id), src->ident, src->host);
	free(cid);

	return 0;
}

static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *reason, int len)
{
	oldbotnet_t *obot = client_data;

	if (!src->nchannels) {
		if (reason && *reason) egg_iprintf(obot->idx, "pt %s %s %s %s\n", src->bot ? src->bot->name : botnet_get_name(), src->nick, itob(src->id), reason);
		else egg_iprintf(obot->idx, "pt %s %s %d\n", src->bot ? src->bot->name : botnet_get_name(), src->nick, itob(src->id));
	} else {
		char *cid;

		cid = strdup(itob(assoc_get_id(src->channels[src->nchannels - 1]->name)));
		egg_iprintf(obot->idx, "j %s %s %s %c%s %s@%s\n", src->bot ? src->bot->name : botnet_get_name(), src->nick, cid, '*', itob(src->id), src->ident, src->host);
		free(cid);
	}

	return 0;
}

static int on_new_bot(void *client_data, botnet_bot_t *bot, int linking)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "n %s %s %cEDNI\n", bot->name, bot->uplink ? bot->uplink->name : botnet_get_name(), linking ? '-' : '!');

	return 0;
}

static int on_lost_bot(void *client_data, botnet_bot_t *bot, const char *reason)
{
	oldbotnet_t *obot = client_data;

	if (bot == obot->bot) egg_iprintf(obot->idx, "bye %s\n", reason);
	else egg_iprintf(obot->idx, "un %s %s\n", bot->name, reason);

	return 0;
}

static int on_unlink(void *client_data, botnet_entity_t *from, struct botnet_bot *bot, const char *reason)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "ul %s %s %s %s\n", from->full_name, bot->uplink->name, bot->name, reason);

	return 0;
}

static int on_botbroadcast(void *client_data, botnet_bot_t *src, const char *command, const char *text, int len)
{
	const char *srcname = src ? src->name : botnet_get_name();
	oldbotnet_t *obot = client_data;

	if (text && len > 0) egg_iprintf(obot->idx, "z %s %s %s\n", srcname, command, text);
	else egg_iprintf(obot->idx, "z %s %s\n", srcname, command);

	return 0;
}

static int on_botmsg(void *client_data, botnet_bot_t *src, botnet_bot_t *dst, const char *command, const char *text, int len)
{
	const char *srcname = src ? src->name : botnet_get_name();
	oldbotnet_t *obot = client_data;

	if (text && len > 0) egg_iprintf(obot->idx, "zb %s %s %s %s\n", srcname, dst->name, command, text);
	else egg_iprintf(obot->idx, "zb %s %s %s\n", srcname, dst->name, command);

	return 0;
}

static int on_extension(void *client_data, botnet_entity_t *src, botnet_bot_t *dst, const char *cmd, const char *text, int len)
{
	oldbotnet_t *obot = client_data;

	if (!strcmp(cmd, "note")) {
		egg_iprintf(obot->idx, "p %s %s\n", src->full_id_name, text);
	} else if (!strcmp(cmd, "who")) {
		egg_iprintf(obot->idx, "w %s %s %s\n", src->full_id_name, dst->name, text);
	}

	return 0;
}
