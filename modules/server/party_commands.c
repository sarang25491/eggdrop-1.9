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
static const char rcsid[] = "$Id: party_commands.c,v 1.15 2004/08/26 18:32:07 darko Exp $";
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
	const char *settings = NULL, *tmp;
	char *channame = NULL;
	channel_t *chanptr = NULL;

	egg_get_arg(text, &settings, &channame);

	if (!channame) {
		partymember_printf(p, _("Syntax: +chan <name> [settings]"));
		return 0;
	}

	if (server_support("CHANTYPES", &tmp) == -1)
		tmp = "#&";

	if (!strchr(tmp, *channame)) {
		partymember_printf(p, _("Error: '%c' is not a valid channel prefix."), *channame);
		partymember_printf(p, _("       This server supports '%s'"), tmp);
		free(channame);
		return 0;
	}

	if (channel_lookup(channame, 1, &chanptr, NULL) == -1) {
		partymember_printf(p, _("Error: Channel '%s' already exists!"), channame);
		free(channame);
		return 0;
	}

/* FIXME - At this early stage just create channel with given name, with no settings considered. */

	partymember_printf(p, _("Channel '%s' has been created but marked +inactive to give you a chance"), channame);
	partymember_printf(p, _("  to finish configuring channel settings. When you are ready, type:"));
	partymember_printf(p, _("    .chanset %s -inactive"), channame);
	partymember_printf(p, _("  to let the bot join %s"), channame);
	channel_set(chanptr, "+inactive", NULL);

	free(channame);
	return BIND_RET_LOG;
}

/* -chan <name> */
static int party_mns_chan(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (text)
		while (isspace(*text))
			text++;

	if (!text || !*text) {
		partymember_printf(p, _("Syntax: -chan <name>"));
		return 0;
	}

	if (destroy_channel_record(text) == 0)
		partymember_printf(p, _("Removed channel '%s'"), text);
	else
		partymember_printf(p, _("Error: Invalid channel '%s'"), text);

	return BIND_RET_LOG;
}

/* chanset <channel / *> <setting> [data] */
static int party_chanset(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	const char *args = NULL;
	char *channame = NULL, *setting = NULL;
	channel_t *chanptr = NULL;
	int allchans;

	egg_get_args(text, &args, &channame, &setting, NULL);

	if (!channame || !setting) {
		partymember_printf(p, _("Syntax: chanset <channel/*> <setting> [value]"));
		free(channame);
		return 0;
	}

	allchans = !strcasecmp("*", channame);
	if (!allchans && channel_lookup(channame, 0, &chanptr, NULL) == -1) {
		partymember_printf(p, _("Error: Invalid channel '%s'"), channame);
		free(channame);
		free(setting);
		return 0;
	}

	if (!args && *setting != '+' && *setting != '-') {
		partymember_printf(p, _("Error: Setting '%s' requires another argument"), setting);
		free(channame);
		free(setting);
		return 0;
	}

	if (channel_set(allchans?NULL:chanptr, setting, args))
		partymember_printf(p, _("Ok, set"));
	else
		partymember_printf(p, _("Error: Invalid setting or value"));

	free(channame);
	free(setting);
	return BIND_RET_LOG;
}

/* chaninfo <channel> */
static int party_chaninfo(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (text)
		while (isspace(*text))
			text++;
	if (!text || !*text) {
		partymember_printf(p, _("Syntax: chaninfo <channel>"));
		return 0;
	}

	return channel_info(p, text);
}

/* chansave [filename] */
static int party_chansave(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (text)
		while (isspace(*text))
			text++;
	chanfile_save(text && *text ? text : NULL);

	return BIND_RET_LOG;
}

/* +ban/+invite/+exempt <mask> [channel] [%XdYhZm] ["comment"] */
/* +chanmask <type> <mask> [channel] [%XdYhZm] [comment] */
static int party_plus_mask(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	const char *rest;
	char typechar, *mask = NULL, *channame = NULL, *duration = NULL, *comment = NULL;
	channel_t *chanptr = NULL;
	int tmpint;
	/* FIXME - this should be long/time_t */
	int unixduration = 0;

	if (!strcasecmp("+ban", cmd))
		typechar = 'b';
	else if (!strcasecmp("+exempt", cmd))
		typechar = 'e';
	else if (!strcasecmp("+invite", cmd))
		typechar = 'I';
	else {
		char *type;
		egg_get_arg(text, &rest, &type);
		if (!type || strlen(type) != 1) {
			partymember_printf(p, _("Syntax: +chanmask <type> <mask> [channel] [%%XdYhZm] [\"comment\"]"));
			free(type);
			return BIND_RET_LOG;
		}
		text = rest;
		typechar = *type;
		free(type);
	}

	egg_get_args(text, &rest, &mask, &channame, &duration, NULL);

	if (!mask || !*mask) {
		partymember_printf(p, _("Syntax: %s %s<mask> [channel] [%%XdYhZm] [\"comment\"]"),
					cmd, !strcasecmp("+chanmask", cmd)?"<type> ":"");
		goto ppb_out;
	}

	if (rest && *rest) {
		comment = strdup(rest);
		if (channel_lookup(channame, 0, &chanptr, NULL) == -1) {
			partymember_printf(p, _("Error: Invalid channel '%s'"), channame);
			goto ppb_out;
		}
		if (*duration != '%') {
			partymember_printf(p, _("Error: Invalid duration. Must be of the form %%XdYhZm"));
			goto ppb_out;
		}
	}
	else if (duration) {
		if (channel_lookup(channame, 0, &chanptr, NULL) == -1) {
			if (*channame != '%') {
				partymember_printf(p, _("Error: Invalid channel %s"), channame);
				goto ppb_out;
			}
			comment = duration;
			duration = channame;
			channame = NULL;
		} else if (*duration != '%') {
			comment = duration;
			duration = NULL;
		}
	}
	else if (channame) {
		if (channel_lookup(channame, 0, &chanptr, NULL) == -1) {
			if (*channame == '%')
				duration = channame;
			else
				comment = channame;
			channame = NULL;
		}
	}

	if (duration)
		unixduration = parse_expire_string(duration);
	if (unixduration) {
		/* FIXME - int2long thing again */
		int tmptime;
		timer_get_now_sec(&tmptime);
		unixduration+=tmptime;
	}

	if (comment && *comment == '@')
		tmpint = channel_notirc_add_mask(chanptr, typechar, mask, u->handle, &comment[1], unixduration, 0, 1, 1);
	else
		tmpint = channel_notirc_add_mask(chanptr, typechar, mask, u->handle, comment, unixduration, 0, 0, 1);

	if (tmpint == 2)
		partymember_printf(p, _("Error: Entry already exists!"));
	else if (tmpint == 0)
		partymember_printf(p, _("Ok, added"));

ppb_out:
	free(mask);
	free(channame);
	free(duration);
	free(comment);
	return BIND_RET_LOG;
}

/* bans/invites/exempts [channel/all [mask]] */
/* chanmask <type> [channel/all [mask]] */
static int party_list_masks(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char typechar, *mask = NULL, *channame = NULL;
	channel_t *chanptr = NULL;
	channel_mask_t **cm = NULL;
	int allchans = 0, i;

	if (!strcasecmp("bans", cmd))
		typechar = 'b';
	else if (!strcasecmp("exempts", cmd))
		typechar = 'e';
	else if (!strcasecmp("invites", cmd))
		typechar = 'I';
	else {
		char *type = NULL;
		const char *rest = NULL;
		egg_get_arg(text, &rest, &type);
		if (!type || strlen(type) != 1) {
			partymember_printf(p, _("Syntax: chanmasks <type> [channel/all] [mask]"));
			free(type);
			return BIND_RET_LOG;
		}
		text = rest;
		typechar = *type;
		free(type);
	}

	egg_get_args(text, NULL, &channame, &mask, NULL);

	if (channame) {
		if (!strcasecmp("all", channame))
			allchans = 1;
		else if (channel_lookup(channame, 0, &chanptr, NULL) == -1) {
			if (mask) {
				partymember_printf(p, _("Error: Invalid channel '%s'"), channame),
				free(mask);
				free(channame);
				return BIND_RET_LOG;
			}
			else {
				mask = channame;
				channame = NULL;
				allchans = 1;
			}
		}
	}
	else
		allchans = 1;

	i = channel_list_masks(&cm, typechar, allchans?NULL:chanptr, mask?mask:"*");
	if (i) {
		int j;
		if (allchans)
			partymember_printf(p, _("  Global +%c list:"), typechar);
		else
			partymember_printf(p, _("  +%c list for channel '%s':"), typechar, channame);
		for (j = 0; j < i; j++) {
/* FIXME - sort out the aesthetics later, for now just print data */
			partymember_printf(p, _(""));
			partymember_printf(p, _("  %s%s"), cm[j]->mask,
						cm[j]->sticky?" (sticky)":"");
			if (cm[j]->creator)
				partymember_printf(p, _("    Created by:\t%s"), cm[j]->creator);
			if (cm[j]->set_by)
				partymember_printf(p, _("    Active:\tset by %s on %s"),
						cm[j]->set_by, ctime((time_t *)&cm[j]->time));
			if (cm[j]->creator)
				partymember_printf(p, _("    Expires:\t%s"),
				cm[j]->expire?ctime((time_t *)&cm[j]->expire):"never");
			if (cm[j]->last_used) {
				int daysago; /* FIXME - grr, another forced-to-declare int */
				timer_get_now_sec(&daysago);
				daysago -= cm[j]->last_used;
				partymember_printf(p, _("    Last used:\t%s ago"), duration_to_string(daysago));
			}
			if (cm[j]->comment)
				partymember_printf(p, _("    Comment:\t%s"), cm[j]->comment);
		}
		free(cm);
	}
	else
		partymember_printf(p, _("  No results."));

	return BIND_RET_LOG;
}

/* -ban/-invite/-exempt <mask/number> */
/* -chanmask <type> <mask/number> */
static int party_minus_mask(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char typechar, *mask = NULL, *channame = NULL;
	channel_t *chanptr = NULL;
	int tmpint;

	if (!strcasecmp("-ban", cmd))
		typechar = 'b';
	else if (!strcasecmp("-exempt", cmd))
		typechar = 'e';
	else if (!strcasecmp("-invite", cmd))
		typechar = 'I';
	else {
		const char *rest;
		char *type;
		egg_get_arg(text, &rest, &type);
		if (!type || strlen(type) != 1) {
			partymember_printf(p, _("Syntax: -chanmask <type> <mask/number> [channel]"));
			free(type);
			return BIND_RET_LOG;
		}
		text = rest;
		typechar = *type;
		free(type);
	}

	egg_get_args(text, NULL, &mask, &channame, NULL);

	if (!mask) {
		partymember_printf(p, _("Syntax: %s %s<mask/number> [channel]"),
					cmd, !strcasecmp("-chanmask", cmd)?"<type> ":"");
		return BIND_RET_LOG;
	}

	if (channame)
		channel_lookup(channame, 0, &chanptr, NULL);

	tmpint = channel_del_mask(chanptr, typechar, mask, 1);

	if (tmpint == -2)
		partymember_printf(p, _("Error: No matches for %s"), mask);
	else if (tmpint == -1)
		partymember_printf(p, _("Error: No '+%c' list"), typechar);
	else
		partymember_printf(p, _("Ok, removed %s from %s"), mask,
					chanptr?channame:"global list");

	free(mask);
	free(channame);
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
	{0}
};
