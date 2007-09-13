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
static const char rcsid[] = "$Id: events.c,v 1.3 2007/09/13 22:20:56 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "botnet.h"

/* botnet callbacks */
static int on_login(void *client_data, partymember_t *src);
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
	on_login, on_bcast, on_privmsg, on_nick, on_quit,
	on_chanmsg, on_join, on_part,
	on_new_bot, on_lost_bot, on_link, on_unlink, on_botmsg, on_botbroadcast, on_extension
};

static int on_login(void *client_data, partymember_t *src)
{
	bot_t *b = client_data;

	if (src->bot) egg_iprintf(b->idx, ":%s login %s %s %s %s %d", src->bot->name, src->nick, src->ident, src->host, b64enc_int(src->id));
	else egg_iprintf(b->idx, "login %s %s %s %s %d", src->nick, src->ident, src->host, b64enc_int(src->id));

	return 0;
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
	bot_t *b = client_data;

	egg_iprintf(b->idx, ":%s broadcast :%s", entity_net_name(src), text ? text : "");

	return 0;
}

static int on_privmsg(void *client_data, botnet_entity_t *src, partymember_t *dst, const char *text, int len)
{
	bot_t *b = client_data;

	egg_iprintf(b->idx, ":%s privmsg %s :%s", entity_net_name(src), dst->net_name, text);

	return 0;
}

static int on_quit(void *client_data, partymember_t *src, const char *text, int len)
{
	bot_t *b = client_data;

	egg_iprintf(b->idx, ":%s quit :%s", src->net_name, text ? text : "");

	return 0;
}

static int on_nick(void *client_data, partymember_t *src, const char *oldnick)
{
	bot_t *b = client_data;

	egg_iprintf(b->idx, ":%s nick %s", src->net_name, src->nick);

	return 0;
}

static int on_chanmsg(void *client_data, partychan_t *chan, botnet_entity_t *src, const char *text, int len)
{
	bot_t *b = client_data;

	egg_iprintf(b->idx, ":%s chanmsg %s :%s", entity_net_name(src), chan->name, text ? text : text);

	return 0;
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking)
{
	bot_t *b = client_data;

	egg_iprintf(b->idx, ":%s join %s%s", src->net_name, chan->name, linking ? " B" : "");

	return 0;
}

static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *reason, int len)
{
	bot_t *b = client_data;

	egg_iprintf(b->idx, ":%s part %s :%s", src->net_name, chan->name, reason ? reason : "");

	return 0;
}

static int on_new_bot(void *client_data, botnet_bot_t *bot, int linking)
{
	int version = 1090000;
	const char *ver, *type, *fullversion;
	bot_t *b = client_data;

	type = botnet_get_info(bot, "type");
	ver = botnet_get_info(bot, "numversion");
	fullversion = botnet_get_info(bot, "numversion");
	if (!type) type = "unknown";
	if (ver) version = atoi(ver);
	if (!fullversion) fullversion = "unknown";
	if (bot->uplink) egg_iprintf(b->idx, ":%s newbot %s %s %s %s %s", bot->uplink->name, bot->name, type, b64enc_int(version), fullversion, linking ? "B" : "A");
	else egg_iprintf(b->idx, "newbot %s %s %s %s%s", bot->name, type, b64enc_int(version), fullversion, linking ? " B" : "");

	return 0;
}

static int on_lost_bot(void *client_data, botnet_bot_t *bot, const char *reason)
{
	bot_t *b = client_data;

	egg_iprintf(b->idx, ":%s bquit :%s", bot->name, reason ? reason : reason);

	return 0;
}

static int on_link(void *client_data, botnet_entity_t *from, struct botnet_bot *dst, const char *target)
{
	bot_t *b = client_data;

	egg_iprintf(b->idx, ":%s link %s %s", entity_net_name(from), dst->name, target);

	return 0;
}

static int on_unlink(void *client_data, botnet_entity_t *from, struct botnet_bot *bot, const char *reason)
{
	bot_t *b = client_data;

	egg_iprintf(b->idx, ":%s unlink %s :%s", entity_net_name(from), bot->name, reason);

	return 0;
}

static int on_botbroadcast(void *client_data, botnet_bot_t *src, const char *command, const char *text, int len)
{
	int printlen, totallen;
	char sbuf[512], *obuf;
	bot_t *b = client_data;

	if (src) totallen = 1 + strlen(src->name) + 12 + strlen(command) + 2 + len;
	else totallen = 11 + strlen(command) + 2 + len;
	if (totallen < 512) obuf = sbuf;
	else obuf = malloc(totallen + 1);

	if (src) printlen = sprintf(obuf, ":%s bbroadcast %s :", src->name, command);
	else printlen = sprintf(obuf, "bbroadcast %s :", command);
	memcpy(obuf + printlen, text, len);
	sockbuf_write(b->idx, obuf, totallen);
	if (obuf != sbuf) free(obuf);

	return 0;
}

static int on_botmsg(void *client_data, botnet_bot_t *src, botnet_bot_t *dst, const char *command, const char *text, int len)
{
	int printlen, totallen;
	char sbuf[512], *obuf;
	bot_t *b = client_data;

	if (src) totallen = 1 + strlen(src->name) + 8 + strlen(dst->name) + 1 + strlen(command) + 2 + len;
	else totallen = 7 + strlen(dst->name) + 1 + strlen(command) + 2 + len;
	if (totallen < 512) obuf = sbuf;
	else obuf = malloc(totallen + 1);

	if (src) printlen = sprintf(obuf, ":%s botmsg %s %s :", src->name, dst->name, command);
	else printlen = sprintf(obuf, "botmsg %s %s :", dst->name, command);
	memcpy(obuf + printlen, text, len);
	sockbuf_write(b->idx, obuf, totallen);
	if (obuf != sbuf) free(obuf);

	return 0;
}

static int on_extension(void *client_data, botnet_entity_t *src, botnet_bot_t *dst, const char *cmd, const char *text, int len)
{
	char sbuf[512], *obuf;
	int printlen, totallen = 1 + strlen(entity_net_name(src)) + 11 + strlen(dst->name) + 1 + strlen(cmd) + 2 + len;
	bot_t *b = client_data;

	if (totallen < 512) obuf = sbuf;
	else obuf = malloc(totallen + 1);

	printlen = sprintf(obuf, ":%s extension %s %s :", entity_net_name(src), dst->name, cmd);
	memcpy(obuf + printlen, text, len);
	sockbuf_write(b->idx, obuf, totallen);
	if (obuf != sbuf) free(obuf);

	return 0;
}
