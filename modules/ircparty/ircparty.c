/* ircparty.c: irc partyline interface
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
static const char rcsid[] = "$Id: ircparty.c,v 1.10 2004/06/17 02:01:14 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "ircparty.h"

irc_config_t irc_config = {0};

static config_var_t irc_config_vars[] = {
	{"vhost", &irc_config.vhost, CONFIG_STRING},
	{"port", &irc_config.port, CONFIG_INT},
	{"stealth", &irc_config.stealth, CONFIG_INT},
	{0}
};

EXPORT_SCOPE int ircparty_LTX_start(egg_module_t *modinfo);
static int ircparty_close(int why);

/* From events.c */
extern partyline_event_t irc_party_handler;

static int irc_idx = -1;
static int irc_port = 0;
static bind_table_t *BT_ircparty = NULL;

/* Ircparty binds. */
static int got_join(partymember_t *p, int idx, char *cmd, int nargs, char *args[]);
static int got_part(partymember_t *p, int idx, char *cmd, int nargs, char *args[]);
static int got_quit(partymember_t *p, int idx, char *cmd, int nargs, char *args[]);
static int got_privmsg(partymember_t *p, int idx, char *cmd, int nargs, char *args[]);
static int got_who(partymember_t *p, int idx, char *cmd, int nargs, char *args[]);
static int got_mode(partymember_t *p, int idx, char *cmd, int nargs, char *args[]);

/* Sockbuf handler. */
static int irc_on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port);
static int irc_on_read(void *client_data, int idx, char *data, int len);
static int irc_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int irc_on_delete(void *client_data, int idx);

static int ident_result(void *client_data, const char *ip, int port, const char *reply);
static int dns_result(void *client_data, const char *ip, char **hosts);
static int process_results(irc_session_t *session);

static bind_list_t ircparty_binds[] = {
	{NULL, "PRIVMSG", got_privmsg},
	{NULL, "JOIN", got_join},
	{NULL, "PART", got_part},
	{NULL, "QUIT", got_quit},
	{NULL, "MODE", got_mode},
	{NULL, "WHO", got_who},
	{0}
};

static sockbuf_handler_t server_handler = {
	"ircparty server",
	NULL, NULL, irc_on_newclient,
	NULL, NULL
};

static sockbuf_handler_t client_handler = {
	"ircparty",
	NULL, irc_on_eof, NULL,
	irc_on_read, NULL,
	irc_on_delete
};

int irc_init()
{
	BT_ircparty = bind_table_add("ircparty", 5, "PisiS", MATCH_MASK, BIND_STACKABLE);	/* DDD */
	bind_add_list("ircparty", ircparty_binds);
	/* Open our listening socket. */
	irc_idx = egg_server(irc_config.vhost, irc_config.port, &irc_port);
	sockbuf_set_handler(irc_idx, &server_handler, NULL);
	return(0);
}

static void kill_session(irc_session_t *session)
{
	if (session->ip) free(session->ip);
	if (session->host) free(session->host);
	if (session->ident) free(session->ident);
	if (session->nick) free(session->nick);
	if (session->pass) free(session->pass);
	free(session);
}

static int irc_on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port)
{
	irc_session_t *session;

	session = calloc(1, sizeof(*session));
	session->ip = strdup(peer_ip);
	session->port = peer_port;
	session->idx = newidx;

	sockbuf_set_handler(newidx, &client_handler, session);
	linemode_on(newidx);

	/* Stealth logins are where we don't say anything until we know they
	 * are a valid user. */
	if (irc_config.stealth) {
		session->state = STATE_RESOLVE;
		session->flags |= STEALTH_LOGIN;
	}
	else {
		egg_iprintf(newidx, ":eggdrop.bot NOTICE AUTH :*** Hello %s/%d!\r\n", peer_ip, peer_port);
		sockbuf_write(newidx, ":eggdrop.bot NOTICE AUTH :*** Don't forget to use the /PASS command to send your password in addition to normal registration!\r\n", -1);
		session->state = STATE_UNREGISTERED;
	}

	/* Start lookups. */
	egg_ident_lookup(peer_ip, peer_port, irc_port, -1, ident_result, session);
	egg_dns_reverse(peer_ip, -1, dns_result, session);

	return(0);
}

static int ident_result(void *client_data, const char *ip, int port, const char *reply)
{
	irc_session_t *session = client_data;

	if (reply) session->ident = strdup(reply);
	else session->ident = strdup("~ircparty");
	process_results(session);
	return(0);
}

static int dns_result(void *client_data, const char *ip, char **hosts)
{
	irc_session_t *session = client_data;
	const char *host;

	if (!hosts || !hosts[0]) host = ip;
	else host = hosts[0];

	session->host = strdup(host);
	process_results(session);
	return(0);
}

static int process_results(irc_session_t *session)
{
	char fakehost[512];

	if (!session->ident || !session->host) return(0);

	if (session->state == STATE_PARTYLINE) {
		partyline_update_info(session->party, session->ident, session->host);
		return(0);
	}
	if (session->flags & STEALTH_LOGIN) {
		snprintf(fakehost, sizeof(fakehost), "-ircparty!%s@%s", session->ident, session->host);
		fakehost[sizeof(fakehost)-1] = 0;
		if (!user_lookup_by_irchost(fakehost)) {
			sockbuf_delete(session->idx);
			return(0);
		}
		sockbuf_write(session->idx, ":eggdrop.bot NOTICE AUTH :*** Don't forget to use the /PASS command to send your password in addition to normal registration!\r\n", -1);
		session->state = STATE_UNREGISTERED;
	}
	return(0);
}

static void irc_greet(irc_session_t *session)
{
	egg_iprintf(session->idx, ":eggdrop.bot 001 %s :Welcome to the Eggdrop Partyline!\r\n", session->nick);
	egg_iprintf(session->idx, ":eggdrop.bot 375 %s :Message of the Day\r\n", session->nick);
	egg_iprintf(session->idx, ":eggdrop.bot 372 %s :There isn't one.\r\n", session->nick);
	egg_iprintf(session->idx, ":eggdrop.bot 376 %s :End of /MOTD command.\r\n", session->nick);
}

static int got_join(partymember_t *p, int idx, char *cmd, int nargs, char *args[])
{
	if (nargs != 1 || strlen(args[0]) < 2) egg_iprintf(idx, ":eggdrop.bot 461 %s JOIN :Not enough parameters.\r\n", p->nick);
	else partychan_join_name(args[0]+1, p);
	return(0);
}

static int got_part(partymember_t *p, int idx, char *cmd, int nargs, char *args[])
{
	if (nargs != 1 || strlen(args[0]) < 2) egg_iprintf(idx, ":eggdrop.bot 461 %s PART :Not enough parameters.\r\n", p->nick);
	else partychan_part_name(args[0]+1, p, args[1]);
	return(0);
}

static int got_mode(partymember_t *p, int idx, char *cmd, int nargs, char *args[])
{
	if (nargs != 1) return(0);
	if (*args[0] == '#') egg_iprintf(idx, ":eggdrop.bot 324 %s %s +tn\r\n", p->nick, args[0]);
	else egg_iprintf(idx, ":eggdrop.bot 221 %s +i\r\n", p->nick);
	return(0);
}

static int got_privmsg(partymember_t *p, int idx, char *cmd, int nargs, char *args[])
{
	if (nargs != 2) {
		egg_iprintf(idx, ":eggdrop.bot 461 %s PRIVMSG :Not enough parameters.\r\n", p->nick);
		return(0);
	}

	if (*args[0] == '#') {
		partychan_t *chan = partychan_lookup_name(args[0]+1);
		if (!chan) egg_iprintf(idx, ":eggdrop.bot 401 %s %s :No such nick/channel\r\n", p->nick, args[0]);
		else partyline_on_input(partychan_lookup_name(args[0]+1), p, args[1], -1);
	}
	else {
		partymember_t *dest = partymember_lookup_nick(args[0]);
		if (!dest) egg_iprintf(idx, ":eggdrop.bot 401 %s %s :No such nick/channel\r\n", p->nick, args[0]);
		else partymember_msg(dest, p, args[1], -1);
	}
	return(0);
}

static int got_who(partymember_t *p, int idx, char *cmd, int nargs, char *args[])
{
	partychan_t *chan;
	partychan_member_t *m;
	partymember_t *q;
	int i;

	if (nargs != 1 || strlen(args[0]) < 2) return(0);
	chan = partychan_lookup_name(args[0]+1);
	if (!chan) return(0);

	for (i = 0; i < chan->nmembers; i++) {
		m = chan->members+i;
		if (chan->members[i].flags & PARTY_DELETED) continue;
		q = m->p;
		egg_iprintf(idx, ":eggdrop.bot 352 %s %s %s %s eggdrop.bot %s H :0 real name\r\n", p->nick, args[0], q->ident, q->host, q->nick);
		putlog(LOG_MISC, "ircpartylog", ":eggdrop.bot 352 %s %s %s %s eggdrop.bot %s H :0 real name\r\n", p->nick, args[0], q->ident, q->host, q->nick);
	}

	egg_iprintf(idx, ":eggdrop.bot 315 %s %s :End of /WHO list.\r\n", p->nick, args[0]);
	return(0);
}

static int got_quit(partymember_t *p, int idx, char *cmd, int nargs, char *args[])
{
	sockbuf_delete(idx);
	return(0);
}

static int irc_on_read(void *client_data, int idx, char *data, int len)
{
	irc_session_t *session = client_data;
	irc_msg_t msg;
	int i;

	irc_msg_parse(data, &msg);
	putlog(LOG_MISC, "ircpartylog", "%d, '%s'", msg.nargs, msg.cmd);
	for (i = 0; i < msg.nargs; i++) {
		putlog(LOG_MISC, "ircpartylog", "--> '%s'", msg.args[i]);
	}

	switch (session->state) {
		case STATE_PARTYLINE:
			bind_check(BT_ircparty, NULL, msg.cmd, session->party, idx, msg.cmd, msg.nargs, msg.args);
			break;
		case STATE_UNREGISTERED:
			if (!strcasecmp(msg.cmd, "nick") && msg.nargs == 1) {
				if (session->nick) egg_iprintf(session->idx, ":%s NICK %s\r\n", session->nick, msg.args[0]);
				str_redup(&session->nick, msg.args[0]);
			}
			else if (!strcasecmp(msg.cmd, "pass") && msg.nargs == 1) {
				str_redup(&session->pass, msg.args[0]);
			}
			else break;
			if (session->nick && session->pass) {
				session->user = user_lookup_authed(session->nick, session->pass);
				if (!session->user) {
					sockbuf_write(session->idx, ":eggdrop.bot NOTICE AUTH :*** Invalid username/password.\r\n", -1);
					sockbuf_delete(session->idx);
					break;
				}
				free(session->pass);
				session->pass = NULL;
				session->party = partymember_new(-1, session->user, session->nick, session->ident ? session->ident : "~ircparty", session->host ? session->host : session->ip, &irc_party_handler, session);
				session->state = STATE_PARTYLINE;
				irc_greet(session);
				partychan_join_name("*", session->party);
			}
			break;
	}
	irc_msg_cleanup(&msg);
	return(0);
}

static int irc_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	irc_session_t *session = client_data;

	if (session->party) {
		if (!err) errmsg = "Client disconnected";
		else if (!errmsg) errmsg = "Unknown error";
		partymember_delete(session->party, errmsg);
		session->party = NULL;
	}
	sockbuf_delete(idx);
	return(0);
}

static int irc_on_delete(void *client_data, int idx)
{
	irc_session_t *session = client_data;

	if (session->party) {
		partymember_delete(session->party, "Deleted!");
		session->party = NULL;
	}
	kill_session(session);
	return(0);
}

static int ircparty_close(int why)
{
	sockbuf_delete(irc_idx);
	return(0);
}

int ircparty_LTX_start(egg_module_t *modinfo)
{
	void *config_root;

	modinfo->name = "ircparty";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "irc client support for the partyline";
	modinfo->close_func = ircparty_close;

	/* Set defaults. */
	memset(&irc_config, 0, sizeof(irc_config));

	/* Link our vars in. */
	config_root = config_get_root("eggdrop");
	config_link_table(irc_config_vars, config_root, "ircparty", 0, NULL);
	config_update_table(irc_config_vars, config_root, "ircparty", 0, NULL);

	irc_init();
	return(0);
}
