/* dccparty.c: dcc partyline interface
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
static const char rcsid[] = "$Id: dccparty.c,v 1.16 2007/09/13 22:20:56 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "dccparty.h"

dcc_config_t dcc_config = {0};

static config_var_t dcc_config_vars[] = {
	{"vhost", &dcc_config.vhost, CONFIG_STRING},
	{"port", &dcc_config.port, CONFIG_INT},
	{"stealth", &dcc_config.stealth, CONFIG_INT},
	{"max_retries", &dcc_config.max_retries, CONFIG_INT},

	{0}
};

EXPORT_SCOPE int dccparty_LTX_start(egg_module_t *modinfo);
static int dccparty_close(int why);

/* From events.c */
extern partyline_event_t dcc_party_handler;

static int got_chat_request(char *nick, char *uhost, user_t *u, char *type, char *ip, int port);

static int dcc_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int dcc_on_read(void *client_data, int idx, char *data, int len);
static int dcc_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int dcc_on_delete(event_owner_t *owner, void *client_data);
static int dcc_pm_delete(event_owner_t *owner, void *client_data);

static int ident_result(void *client_data, const char *ip, int port, const char *reply);
static int dns_result(void *client_data, const char *ip, char **hosts);
static int process_results(dcc_session_t *session);

static event_owner_t dcc_generic_owner = {
	"dccparty", 0,
	0, 0,
	0
};

static event_owner_t dcc_pm_owner = {
	"dccparty", 0,
	0, 0,
	dcc_pm_delete
};

static event_owner_t dcc_sock_owner = {
	"dccparty", 0,
	0, 0,
	dcc_on_delete
};

static sockbuf_handler_t dcc_handler = {
	"dcc",
	dcc_on_connect, dcc_on_eof, NULL,
	dcc_on_read, NULL
};

int dcc_init()
{
	bind_add_simple("dcc_chat", NULL, "*", got_chat_request);
	return(0);
}

static void kill_session(dcc_session_t *session)
{
	if (session->ident_id != -1) egg_ident_cancel(session->ident_id, 0);
	if (session->dns_id != -1) egg_dns_cancel(session->dns_id, 0);
	if (session->ip) free(session->ip);
	if (session->host) free(session->host);
	if (session->ident) free(session->ident);
	if (session->nick) free(session->nick);
	free(session);
}

static int got_chat_request(char *nick, char *uhost, user_t *u, char *type, char *ip, int port)
{
	int idx;
	dcc_session_t *session;

	/* Ignore chats from unknown users. */
	if (!u) {
		putlog(LOG_MISC, "*", _("Ignoring chat request from %s (user unknown)."), nick);
		return(0);
	}

	idx = egg_connect(ip, port, 0);

	session = calloc(1, sizeof(*session));
	session->nick = strdup(nick);
	session->idx = idx;

	sockbuf_set_handler(idx, &dcc_handler, session, &dcc_sock_owner);
	linemode_on(idx);
	return(0);
}

static int dcc_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	dcc_session_t *session = client_data;
	int sock, our_port = 0;

	sock = sockbuf_get_sock(idx);
	socket_get_name(sock, NULL, &our_port);
	session->ip = strdup(peer_ip);
	session->port = peer_port;
	egg_iprintf(idx, "Hello %s/%d!\r\n\r\n", peer_ip, peer_port);
	egg_iprintf(idx, "Please enter your nickname.\r\n");
	session->state = STATE_NICKNAME;
	session->count = 0;

	/* Start lookups. */
	session->ident_id = egg_ident_lookup(peer_ip, peer_port, our_port, -1, ident_result, session, &dcc_generic_owner);
	session->dns_id = egg_dns_reverse(peer_ip, -1, dns_result, session, &dcc_generic_owner);

	return(0);
}

static int ident_result(void *client_data, const char *ip, int port, const char *reply)
{
	dcc_session_t *session = client_data;

	session->ident_id = -1;
	if (reply) session->ident = strdup(reply);
	else session->ident = strdup("~dcc");
	process_results(session);
	return(0);
}

static int dns_result(void *client_data, const char *ip, char **hosts)
{
	dcc_session_t *session = client_data;
	const char *host;

	session->dns_id = -1;
	if (!hosts || !hosts[0]) host = ip;
	else host = hosts[0];

	session->host = strdup(host);
	process_results(session);
	return(0);
}

static int process_results(dcc_session_t *session)
{
	char fakehost[512];

	if (!session->ident || !session->host) return(0);

	if (session->state == STATE_PARTYLINE) {
		partyline_update_info(session->party, session->ident, session->host);
		return(0);
	}
	if (session->flags & STEALTH_LOGIN) {
		snprintf(fakehost, sizeof(fakehost), "-dcc!%s@%s", session->ident, session->host);
		fakehost[sizeof(fakehost)-1] = 0;
		if (!user_lookup_by_irchost(fakehost)) {
			sockbuf_delete(session->idx);
			return(0);
		}
		egg_iprintf(session->idx, "\r\nPlease enter your nickname.\r\n");
		session->state = STATE_NICKNAME;
	}
	return(0);
}

static int dcc_on_read(void *client_data, int idx, char *data, int len)
{
	dcc_session_t *session = client_data;

	switch (session->state) {
		case STATE_PARTYLINE:
			partyline_on_input(NULL, session->party, data, len);
			break;
		case STATE_NICKNAME:
			session->nick = strdup(data);
			session->state = STATE_PASSWORD;
			sockbuf_write(session->idx, "Please enter your password.\r\n", -1);
			break;
		case STATE_PASSWORD:
			session->user = user_lookup_authed(session->nick, data);
			if (!session->user) {
				sockbuf_write(session->idx, "Invalid username/password.\r\n\r\n", -1);
				session->count++;
				if (session->count > dcc_config.max_retries) {
					sockbuf_delete(session->idx);
					break;
				}
				free(session->nick);
				session->nick = NULL;
				sockbuf_write(session->idx, "Please enter your nickname.\r\n", -1);
				session->state = STATE_NICKNAME;
			}
			else {
				session->party = partymember_new(-1, session->user, NULL, session->nick, session->ident ? session->ident : "~dcc", session->host ? session->host : session->ip, &dcc_party_handler, session, &dcc_pm_owner);
				session->state = STATE_PARTYLINE;
				egg_iprintf(idx, _("\r\nWelcome to the dcc partyline interface!\r\n"));
				if (session->ident) egg_iprintf(idx, _("Your ident is: %s\r\n"), session->ident);
				if (session->host) egg_iprintf(idx, _("Your hostname is: %s\r\n"), session->host);
				/* Put them on the main channel. */
				partychan_join_name("*", session->party, 0);
			}
			break;
	}
	return(0);
}

static int dcc_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	dcc_session_t *session = client_data;

	if (session->party) {
		if (!err) errmsg = "Client disconnected";
		else if (!errmsg) errmsg = "Unknown error";
		partymember_delete(session->party, NULL, errmsg);
		/* This will call our on_quit handler which will delete the sockbuf
		 * which will call our on_delete handler which will kill the session.
		 * Summery: All the data is gone, don't touch any of the variables */
	} else {
		sockbuf_delete(idx);
	}
	return(0);
}

static int dcc_pm_delete(event_owner_t *owner, void *client_data)
{
	dcc_session_t *session = client_data;

	session->party = NULL;

	sockbuf_delete(session->idx);

	return 0;
}

static int dcc_on_delete(event_owner_t *owner, void *client_data)
{
	dcc_session_t *session = client_data;

	session->idx = -1;
	if (session->party) {
		partymember_delete(session->party, NULL, "Deleted!");
		session->party = NULL;
	}
	kill_session(session);
	return(0);
}

static int dccparty_close(int why)
{
	void *config_root;

	config_root = config_get_root("eggdrop");
	config_unlink_table(dcc_config_vars, config_root, "dccparty", 0, NULL);

	return(0);
}

int dccparty_LTX_start(egg_module_t *modinfo)
{
	void *config_root;

	dcc_generic_owner.module = dcc_sock_owner.module = dcc_pm_owner.module = modinfo;

	modinfo->name = "dccparty";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "dcc chat support for the partyline";
	modinfo->close_func = dccparty_close;

	/* Set defaults. */
	memset(&dcc_config, 0, sizeof(dcc_config));
	dcc_config.max_retries = 3;

	/* Link our vars in. */
	config_root = config_get_root("eggdrop");
	config_link_table(dcc_config_vars, config_root, "dccparty", 0, NULL);
	config_update_table(dcc_config_vars, config_root, "dccparty", 0, NULL);

	dcc_init();
	return(0);
}
