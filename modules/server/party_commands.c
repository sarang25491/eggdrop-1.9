/* party_commands.c: server partyline commands
 *
 * Copyright (C) 2002, 2003, 2004 Eggheads Development Team
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
static const char rcsid[] = "$Id: party_commands.c,v 1.23 2005/11/28 07:09:28 wcc Exp $";
#endif

#include "server.h"

static int party_servers(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	server_t *s;

	if (server_list_len <= 0) {
		partymember_printf(p, _("The server list is empty."));
		return(BIND_RET_LOG);
	}

	partymember_printf(p, _("%d server%s:"), server_list_len, (server_list_len != 1) ? "s" : "");
	for (s = server_list; s; s = s->next) {
		if (s->port) partymember_printf(p, _("   %s (port %d)%s"), s->host, s->port, s->pass ? _(" (password set)") : "");
		else partymember_printf(p, _("   %s (default port)%s"), s->host, s->pass ? _(" (password set)") : "");
	}
	return(BIND_RET_LOG);
}

static void parse_server(const char *text, char **host, int *port, char **pass)
{
	char *sep, *sport = NULL;

	*pass = NULL;
	*port = 0;
	while (isspace(*text)) text++;
	*host = strdup(text);
	sep = strchr(*host, ' ');
	if (sep) {
		*sep++ = 0;
		while (isspace(*sep)) sep++;
		sport = sep;
		sep = strchr(sep, ' ');
		if (sep) {
			*sep++ = 0;
			while (isspace(*sep)) sep++;
			*pass = sep;
		}
		*port = atoi(sport);
	}
}

static int party_plus_server(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *host, *pass;
	int port;

	if (!text || !*text) {
		partymember_printf(p, _("Syntax: +server <host> [port] [pass]"));
		return(0);
	}

	parse_server(text, &host, &port, &pass);
	if (!strlen(host)) {
		partymember_printf(p, _("Please specify a valid host."));
		free(host);
		return(0);
	}

	server_add(host, port, pass);
	partymember_printf(p, _("Added %s/%d.\n"), host, port ? port : server_config.default_port);
	free(host);
	return(BIND_RET_LOG);
}

static int party_minus_server(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *host, *pass;
	int port, i;

	if (!text || !*text) {
		partymember_printf(p, _("Syntax: -server <host> [port] [pass]"));
		return(0);
	}

	parse_server(text, &host, &port, &pass);
	i = server_find(host, port, pass);
	free(host);
	if (i < 0) partymember_printf(p, _("No matching servers."));
	else {
		server_del(i);
		partymember_printf(p, _("Deleted server %d."), i+1);
	}
	return(BIND_RET_LOG);
}

static int party_dump(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || !*text) {
		partymember_printf(p, _("Syntax: dump <server stuff>"));
		return(0);
	}

	printserv(SERVER_NORMAL, "%s\r\n", text);
	return(BIND_RET_LOG);
}

static int party_jump(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *host, *pass;
	int port, num;

	if (text && *text) {
		parse_server(text, &host, &port, &pass);
		num = server_find(host, port, pass);
		if (num < 0) {
			num = server_add(host, port, pass);
			partymember_printf(p, _("Added %s/%d as server #%d.\n"), host, port ? port : server_config.default_port, num+1);
		}
		free(host);
	}
	else num = server_list_index+1;
	server_set_next(num);
	kill_server("changing servers");
	return(BIND_RET_LOG);
}

static int party_chanmembermode(partymember_t *p, user_t *u, const char *cmd, const char *text, const char *flags, const char *mode)
{
	char *nick, *chan;
	partychan_t *pchan;
	channel_t *ircchan;

	egg_get_arg(text, &text, &chan);

	if (!chan) {
		partymember_printf(p, "Syntax: %s [channel] <nick>", cmd);
		return(0);
	}

	if (text && *text) nick = strdup(text);
	else {
		nick = chan;
		pchan = partychan_get_default(p);
		if (pchan) chan = strdup(pchan->name);
		else {
			partymember_printf(p, "You are not on a partyline channel! Please specify a channel: %s [channel] <nick>", cmd);
			free(nick);
			return(0);
		}
	}

	ircchan = channel_probe(chan, 0);
	if (!ircchan) {
		partymember_printf(p, "I'm not on '%s', as far as I know!", chan);
		free(nick);
		free(chan);
		return(0);
	}

	if (user_check_partial_flags_str(u, NULL, flags) || user_check_flags_str(u, chan, flags)) {
		partymember_printf(p, "Setting mode %s for %s on %s", mode, nick, ircchan->name);
		printserv(SERVER_NORMAL, "MODE %s %s %s", ircchan->name, mode, nick);
	}
	else {
		partymember_printf(p, "You need +%s to set that mode!", flags);
	}
	free(nick);
	free(chan);
	return(BIND_RET_LOG);
}

static int party_op(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return party_chanmembermode(p, u, cmd, text, "o", "+o");
}

static int party_deop(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return party_chanmembermode(p, u, cmd, text, "o", "-o");
}

static int party_halfop(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return party_chanmembermode(p, u, cmd, text, "lo", "+l");
}

static int party_dehalfop(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return party_chanmembermode(p, u, cmd, text, "lo", "-l");
}

static int party_voice(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return party_chanmembermode(p, u, cmd, text, "lo", "+v");
}

static int party_devoice(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return party_chanmembermode(p, u, cmd, text, "lo", "-v");
}

static int party_msg(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *dest;
	const char *next;

	egg_get_arg(text, &next, &dest);
	if (!next || !*next || !dest || !*dest) {
		partymember_printf(p, _("Syntax: %s <nick/chan> <message>"), cmd);
		if (dest) free(dest);
		return(0);
	}

	printserv(SERVER_NORMAL, "PRIVMSG %s :%s\r\n", dest, next);
	partymember_printf(p, _("Msg to %1$s: %2$s"), dest, next);
	free(dest);
	return(BIND_RET_LOG);
}

static int party_act(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *dest;
	const char *next;

	egg_get_arg(text, &next, &dest);
	if (!next || !*next || !dest || !*dest) {
		partymember_printf(p, _("Syntax: act <nick/chan> <action>"));
		if (dest) free(dest);
		return(0);
	}

	printserv(SERVER_NORMAL, "PRIVMSG %s :\001ACTION %s\001\r\n", dest, next);
	partymember_printf(p, _("Action to %1$s: %2$s"), dest, next);
	free(dest);
	return(BIND_RET_LOG);
}

static void parse_chanset(channel_t *chan, const char *settings)
{
	char *name, *value;
	char flagbuf[2] = {0, 0};

	while (settings) {
		egg_get_arg(settings, &settings, &name);
		if (!name) break;
		if (*name == '+' || *name == '-') {
			if (*name == '+') flagbuf[0] = '1';
			else flagbuf[0] = '0';
			value = flagbuf;
			if (*(name+1)) channel_set(chan, flagbuf, name+1, 0, NULL);
		}
		else {
			egg_get_arg(settings, &settings, &value);
			channel_set(chan, value, name, 0, NULL);
			if (value) free(value);
		}
		free(name);
	}
}

/* +chan <name> [settings] */
static int party_pls_chan(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	const char *settings;
	char *name;
	channel_t *chan;
	char *key;

	egg_get_arg(text, &settings, &name);
	if (!name) {
		partymember_printf(p, "Syntax: %s <channel> [settings]", cmd);
		return(0);
	}

	chan = channel_add(name);
	free(name);
	parse_chanset(chan, settings);

	channel_get(chan, &key, "key", 0, NULL);
	if (key && strlen(key)) printserv(SERVER_NORMAL, "JOIN %s %s", chan->name, key);
	else printserv(SERVER_NORMAL, "JOIN %s", chan->name);

	return(BIND_RET_LOG);
}

/* -chan <name> */
static int party_mns_chan(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	while (text && isspace(*text)) text++;

	if (!text || !*text) {
		partymember_printf(p, "Syntax: %s <channel>", cmd);
		return(0);
	}

	if (channel_remove(text)) {
		partymember_printf(p, "Channel not found!");
		return(0);
	}

	partymember_printf(p, "Channel removed.");
	printserv(SERVER_NORMAL, "PART %s", text);
	return(BIND_RET_LOG);
}

/* channels */
static int party_channels(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	channel_t *chan;

	if (nchannels <= 0) {
		partymember_printf(p, _("The channel list is empty."));
		return(BIND_RET_LOG);
	}

	for (chan = channel_head; chan; chan = chan->next) {
		partymember_printf(p, "   %s : %d member%s", chan->name, chan->nmembers, chan->nmembers == 1 ? "" : "s");
	}
	return(BIND_RET_LOG);
}

/* chanset <channel / *> <setting> [data] */
static int party_chanset(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	const char *settings;
	char *name;
	channel_t *chan;

	egg_get_arg(text, &settings, &name);
	if (!name) {
		partymember_printf(p, "Syntax: %s <channel> [settings]", cmd);
		return(0);
	}

	chan = channel_probe(name, 0);
	free(name);
	if (!chan) {
		partymember_printf(p, "Channel not found! Use +chan if you want to create a new channel.");
		return(0);
	}

	parse_chanset(chan, settings);
	return(BIND_RET_LOG);
}

/* chaninfo <channel> */
static int party_chaninfo(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	channel_t *chan;
	char *next, *item, *value;

	while (text && isspace(*text)) text++;

	if (!text || !*text) {
		partymember_printf(p, "Syntax: %s <channel>", cmd);
		return(0);
	}

	chan = channel_probe(text, 0);
	if (!chan) {
		partymember_printf(p, "Channel not found!");
		return(0);
	}

	partymember_printf(p, "Information about '%s':", chan->name);
	if (!(chan->flags & CHANNEL_STATIC)) {
		partymember_printf(p, "Saved settings: none (not a static channel)");
	}
	else {
		next = server_config.chaninfo_items;
		while (next && *next) {
			egg_get_arg(next, &next, &item);
			if (!item) break;
			channel_get(chan, &value, item, 0, NULL);
			if (value) {
				partymember_printf(p, "%s: %s", item, value);
			}
			free(item);
		}
	}
	return(BIND_RET_LOG);
}

/* chansave [filename] */
static int party_chansave(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	while (text && isspace(*text)) text++;

	channel_save(text && *text ? text : server_config.chanfile);
	return(BIND_RET_LOG);
}

/* +ban/+invite/+exempt <mask> [channel] [%XdYhZm] ["comment"] */
/* +chanmask <type> <mask> [channel] [%XdYhZm] [comment] */
static int party_plus_mask(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return(BIND_RET_LOG);
}

/* bans/invites/exempts [channel/all [mask]] */
/* chanmask <type> [channel/all [mask]] */
static int party_list_masks(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return(BIND_RET_LOG);
}

/* -ban/-invite/-exempt <mask/number> */
/* -chanmask <type> <mask/number> */
static int party_minus_mask(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return(BIND_RET_LOG);
}


bind_list_t server_party_commands[] = {					/* Old flag requirements */
	{"m", "servers", party_servers},		/* DDD	*/

	/* Normal irc commands. */
	{"o", "halfop", party_halfop},
	{"o", "dehalfop", party_dehalfop},
	{"o", "op", party_op},
	{"o", "deop", party_deop},
	{"o", "voice", party_voice},
	{"o", "devoice", party_devoice},
	{"m", "msg", party_msg},			/* DDD	*/
	{"m", "say", party_msg},			/* DDD	*/
	{"m", "act", party_act},			/* DDD	*/

	{"l", "+ban", party_plus_mask},			/* DDD	*/	/* lo|lo */
	{"l", "+invite", party_plus_mask},		/* DDD	*/	/* lo|lo */
	{"l", "+exempt", party_plus_mask},		/* DDD	*/	/* lo|lo */
	{"l", "+chanmask", party_plus_mask},		/* DDD	*/	/* lo|lo */
	{"l", "-ban", party_minus_mask},		/* DDD	*/	/* lo|lo */
	{"l", "-invite", party_minus_mask},		/* DDD	*/	/* lo|lo */
	{"l", "-exempt", party_minus_mask},		/* DDD	*/	/* lo|lo */
	{"l", "-chanmask", party_minus_mask},		/* DDD	*/	/* lo|lo */
	{"l", "bans", party_list_masks},		/* DDD	*/	/* lo|lo */
	{"l", "invites", party_list_masks},		/* DDD	*/	/* lo|lo */
	{"l", "exempts", party_list_masks},		/* DDD	*/	/* lo|lo */
	{"l", "chanmasks", party_list_masks},		/* DDD	*/	/* lo|lo */
	{"m", "+server", party_plus_server},		/* DDC	*/
	{"m", "-server", party_minus_server},		/* DDD	*/
	{"m", "dump", party_dump},			/* DDD	*/
	{"m", "jump", party_jump},			/* DDD	*/
	{"m", "channels", party_channels},		/* DDD	*/	/* m */
	{"n", "+chan", party_pls_chan},			/* DDC	*/	/* n */
	{"n", "-chan", party_mns_chan},			/* DDC	*/	/* n */
	{"n", "chanset", party_chanset},		/* DDC	*/	/* n|n */
	{"m", "chaninfo", party_chaninfo},		/* DDC	*/	/* m|m */
	{"n", "chansave", party_chansave},		/* DDC	*/	/* n|n */
	{0, 0, 0}
};
