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
static const char rcsid[] = "$Id: events.c,v 1.1 2007/06/03 23:43:46 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "botnet.h"

/* botnet callbacks */
static int on_bcast(void *client_data, botnet_entity_t *src, const char *text, int len);
static int on_privmsg(void *client_data, botnet_entity_t *src, partymember_t *dst, const char *text, int len);
static int on_nick(void *client_data, partymember_t *src, const char *oldnick);
static int on_quit(void *client_data, partymember_t *src, const char *text, int len);
static int on_chanmsg(void *client_data, partychan_t *chan, botnet_entity_t *src, const char *text, int len);
static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking);
static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *reason, int len);
static int on_new_bot(void *client_data, botnet_bot_t *bot, int linking);
static int on_lost_bot(void *client_data, botnet_bot_t *bot, const char *reason);
static int on_link(void *client_data, botnet_entity_t *from, botnet_bot_t *dst, const char *target);
static int on_unlink(void *client_data, botnet_entity_t *from, botnet_bot_t *bot, const char *reason);
static int on_botmsg(void *client_data, botnet_bot_t *src, botnet_bot_t *dst, const char *command, const char *text, int len);
static int on_botbroadcast(void *client_data, botnet_bot_t *src, const char *command, const char *text, int len);
static int on_extension(void *client_data, botnet_entity_t *src, botnet_bot_t *dst, const char *cmd, const char *text, int len);

botnet_handler_t bothandler = {
	on_bcast, on_privmsg, on_nick, on_quit,
	on_chanmsg, on_join, on_part,
	on_new_bot, on_lost_bot, on_link, on_unlink, on_botmsg, on_botbroadcast, on_extension
};

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
	return 0;
}

static int on_privmsg(void *client_data, botnet_entity_t *src, partymember_t *dst, const char *text, int len)
{
	return 0;
}

static int on_quit(void *client_data, partymember_t *src, const char *text, int len)
{
	return 0;
}

static int on_nick(void *client_data, partymember_t *src, const char *oldnick)
{
	return 0;
}

static int on_chanmsg(void *client_data, partychan_t *chan, botnet_entity_t *src, const char *text, int len)
{
	return 0;
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking)
{
	return 0;
}

static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *reason, int len)
{
	return 0;
}

static int on_new_bot(void *client_data, botnet_bot_t *bot, int linking)
{
	return 0;
}

static int on_lost_bot(void *client_data, botnet_bot_t *bot, const char *reason)
{
	return 0;
}

static int on_link(void *client_data, botnet_entity_t *from, struct botnet_bot *dst, const char *target)
{
	return 0;
}

static int on_unlink(void *client_data, botnet_entity_t *from, struct botnet_bot *bot, const char *reason)
{
	return 0;
}

static int on_botbroadcast(void *client_data, botnet_bot_t *src, const char *command, const char *text, int len)
{
	return 0;
}

static int on_botmsg(void *client_data, botnet_bot_t *src, botnet_bot_t *dst, const char *command, const char *text, int len)
{
	return 0;
}

static int on_extension(void *client_data, botnet_entity_t *src, botnet_bot_t *dst, const char *cmd, const char *text, int len)
{
	return 0;
}
