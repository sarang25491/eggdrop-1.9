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
static const char rcsid[] = "$Id: party_commands.c,v 1.17 2004/09/26 09:42:09 stdarg Exp $";
#endif

#include "server.h"

static int party_servers(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	server_t *s;

	if (server_list_len <= 0) partymember_printf(p, _("The server list is empty."));
	else partymember_printf(p, _("%d server%s:"), server_list_len, (server_list_len != 1) ? "s" : "");
	for (s = server_list; s; s = s->next) {
		if (s->port) partymember_printf(p, _("   %s (port %d)%s"), s->host, s->port, s->pass ? _(" (password set)") : "");
		else partymember_printf(p, _("   %s (default port)%s"), s->host, s->pass ? _(" (password set)") : "");
	}
	return(0);
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
	return(0);
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
	return(0);
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
	return(0);
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
	return(0);
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
	return(0);
}

/* +chan <name> [settings] */
static int party_pls_chan(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return BIND_RET_LOG;
}

/* -chan <name> */
static int party_mns_chan(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return BIND_RET_LOG;
}

/* chanset <channel / *> <setting> [data] */
static int party_chanset(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return BIND_RET_LOG;
}

/* chaninfo <channel> */
static int party_chaninfo(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return BIND_RET_LOG;
}

/* chansave [filename] */
static int party_chansave(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return BIND_RET_LOG;
}

/* +ban/+invite/+exempt <mask> [channel] [%XdYhZm] ["comment"] */
/* +chanmask <type> <mask> [channel] [%XdYhZm] [comment] */
static int party_plus_mask(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return BIND_RET_LOG;
}

/* bans/invites/exempts [channel/all [mask]] */
/* chanmask <type> [channel/all [mask]] */
static int party_list_masks(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return BIND_RET_LOG;
}

/* -ban/-invite/-exempt <mask/number> */
/* -chanmask <type> <mask/number> */
static int party_minus_mask(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return BIND_RET_LOG;
}


bind_list_t server_party_commands[] = {					/* Old flag requirements */
	{"", "servers", party_servers},			/* DDD	*/
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
	{"m", "msg", party_msg},			/* DDD	*/
	{"m", "say", party_msg},			/* DDD	*/
	{"m", "act", party_act},			/* DDD	*/
	{"n", "+chan", party_pls_chan},			/* DDC	*/	/* n */
	{"n", "-chan", party_mns_chan},			/* DDC	*/	/* n */
	{"n", "chanset", party_chanset},		/* DDC	*/	/* n|n */
	{"m", "chaninfo", party_chaninfo},		/* DDC	*/	/* m|m */
	{"n", "chansave", party_chansave},		/* DDC	*/	/* n|n */
	{0, 0, 0}
};
