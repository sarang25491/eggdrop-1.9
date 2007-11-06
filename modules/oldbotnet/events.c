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
static const char rcsid[] = "$Id: events.c,v 1.14 2007/11/06 00:05:40 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "oldbotnet.h"

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
	NULL, on_bcast, on_privmsg, on_nick, on_quit,
	on_chanmsg, on_join, on_part,
	on_new_bot, on_lost_bot, on_link, on_unlink, on_botmsg, on_botbroadcast, on_extension
};

static const char *filter(const char *text)
{
	char *p;
	static char buf[512] = {0};

	if (!strchr(text, '\n')) return text;

	strncpy(buf, text, 511);
	p = strchr(buf, '\n');
	if (p) *p = 0;
	return buf;
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

	if (src->what == ENTITY_BOT) name = entity_full_name(src);
	else if (src->user->bot) name = src->user->bot->name;
	else name = botnet_get_name();

	egg_iprintf(obot->idx, "ct %s %s\n", name, filter(text));

	return 0;
}

static int on_privmsg(void *client_data, botnet_entity_t *src, partymember_t *dst, const char *text, int len)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "p %s %s %s\n", entity_full_id_name(src), dst->full_id_name, filter(text));

	return 0;
}

static int on_quit(void *client_data, partymember_t *src, const char *text, int len)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "pt %s %s %s\n", src->bot ? src->bot->name : botnet_get_name(), src->nick, b64enc_int(src->id));

	return 0;
}

static int on_nick(void *client_data, partymember_t *src, const char *oldnick)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "nc %s %s %s\n", src->bot ? src->bot->name : botnet_get_name(), b64enc_int(src->id), src->nick);

	return 0;
}

static int on_chanmsg(void *client_data, partychan_t *chan, botnet_entity_t *src, const char *t, int len)
{
	const char *text = filter(t);
	oldbotnet_t *obot = client_data;

	if (len >= 9 && !strncasecmp(text, "\1ACTION ", 8) && text[len - 1] == 1) {
		egg_iprintf(obot->idx, "a %s %s %.*s\n", entity_full_name(src), b64enc_int(assoc_get_id(chan->name)), len - 9, text + 8);
	} else {
		egg_iprintf(obot->idx, "c %s %s %s\n", entity_full_name(src), b64enc_int(assoc_get_id(chan->name)), text);
	}
	return 0;
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking)
{
	char *cid;
	oldbotnet_t *obot = client_data;

	cid = strdup(b64enc_int(assoc_get_id(chan->name)));
	egg_iprintf(obot->idx, "j %s%s %s %s %c%s %s@%s\n", linking ? "!" : "", src->bot ? src->bot->name : botnet_get_name(), src->nick, cid, '*', b64enc_int(src->id), src->ident, src->host);
	free(cid);

	return 0;
}

static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *reason, int len)
{
	oldbotnet_t *obot = client_data;

	if (!src->nchannels) {
		if (reason && *reason) egg_iprintf(obot->idx, "pt %s %s %s %s\n", src->bot ? src->bot->name : botnet_get_name(), src->nick, b64enc_int(src->id), filter(reason));
		else egg_iprintf(obot->idx, "pt %s %s %s\n", src->bot ? src->bot->name : botnet_get_name(), src->nick, b64enc_int(src->id));
	} else {
		char *cid;

		cid = strdup(b64enc_int(assoc_get_id(src->channels[src->nchannels - 1]->name)));
		egg_iprintf(obot->idx, "j %s %s %s %c%s %s@%s\n", src->bot ? src->bot->name : botnet_get_name(), src->nick, cid, '*', b64enc_int(src->id), src->ident, src->host);
		free(cid);
	}

	return 0;
}

static int on_new_bot(void *client_data, botnet_bot_t *bot, int linking)
{
	int version = 1090000;
	const char *verstr;
	oldbotnet_t *obot = client_data;

	verstr = botnet_get_info(bot, "numversion");
	if (verstr) version = atoi(verstr);
	egg_iprintf(obot->idx, "n %s %s %c%s\n", bot->name, bot->uplink ? bot->uplink->name : botnet_get_name(), linking ? '-' : '!', b64enc_int(version));

	return 0;
}

static int on_lost_bot(void *client_data, botnet_bot_t *bot, const char *reason)
{
	oldbotnet_t *obot = client_data;

	if (bot == obot->bot) egg_iprintf(obot->idx, "bye %s\n", filter(reason));
	else egg_iprintf(obot->idx, "un %s %s\n", bot->name, filter(reason));

	return 0;
}

static int on_link(void *client_data, botnet_entity_t *from, struct botnet_bot *dst, const char *target)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "l %s %s %s\n", entity_full_id_name(from), dst->name, target);

	return 0;
}

static int on_unlink(void *client_data, botnet_entity_t *from, struct botnet_bot *bot, const char *reason)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "ul %s %s %s %s\n", entity_full_id_name(from), bot->uplink->name, bot->name, filter(reason));

	return 0;
}

static int on_botbroadcast(void *client_data, botnet_bot_t *src, const char *command, const char *text, int len)
{
	const char *srcname = src ? src->name : botnet_get_name();
	oldbotnet_t *obot = client_data;

	if (text && len > 0) egg_iprintf(obot->idx, "z %s %s %s\n", srcname, command, filter(text));
	else egg_iprintf(obot->idx, "z %s %s\n", srcname, command);

	return 0;
}

static int on_botmsg(void *client_data, botnet_bot_t *src, botnet_bot_t *dst, const char *command, const char *text, int len)
{
	const char *srcname = src ? src->name : botnet_get_name();
	oldbotnet_t *obot = client_data;

	if (text && len > 0) egg_iprintf(obot->idx, "zb %s %s %s %s\n", srcname, dst->name, command, filter(text));
	else egg_iprintf(obot->idx, "zb %s %s %s\n", srcname, dst->name, command);

	return 0;
}

static int on_extension(void *client_data, botnet_entity_t *src, botnet_bot_t *dst, const char *cmd, const char *t, int len)
{
	const char *text = filter(t);
	oldbotnet_t *obot = client_data;

	if (!strcmp(cmd, "note")) {
		egg_iprintf(obot->idx, "p %s %s\n", entity_full_id_name(src), text);
	} else if (!strcmp(cmd, "who")) {
		egg_iprintf(obot->idx, "w %s %s %s\n", entity_full_id_name(src), dst->name, text);
	} else if (!strcmp(cmd, "motd")) {
		egg_iprintf(obot->idx, "m %s %s\n", entity_full_id_name(src), dst->name);
	} else if (!strcmp(cmd, "versions") && src->what == ENTITY_PARTYMEMBER) {
		egg_iprintf(obot->idx, "v %s %s %d:%s\n", src->user->bot->name, dst->name, src->user->id, src->user->nick);
	} else if (!strcmp(cmd, "away") && src->what == ENTITY_PARTYMEMBER) {
		if (*text) egg_iprintf(obot->idx, "aw %s %s %s\n", src->user->bot ? src->user->bot->name : botnet_get_name(), b64enc_int(src->user->id), text);
		else egg_iprintf(obot->idx, "aw %s %s\n", src->user->bot ? src->user->bot->name : botnet_get_name(), b64enc_int(src->user->id));
	}

	return 0;
}
