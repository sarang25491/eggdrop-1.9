/* botnet.c: support for linking with other bots
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
static const char rcsid[] = "$Id: botnet.c,v 1.1 2007/06/03 23:43:45 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "botnet.h"

EXPORT_SCOPE int botnet_LTX_start(egg_module_t *modinfo);

typedef struct {
	char *ip;
	int port;
} botnet_config_t;

static int bot_close(int why);
static int do_link(user_t *u, const char *text);

static int bot_on_delete(event_owner_t *owner, void *client_data);
//static int sock_on_delete(event_owner_t *owner, void *client_data);

/* Partyline commands. */
static int party_plus_bot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text);
static int party_minus_bot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text);

/* Sockbuf handler. */
static int idx_on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port);
static int idx_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int idx_on_read(void *client_data, int idx, char *data, int len);
static int idx_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int idx_on_delete(void *client_data, int idx);

static int listen_idx;

static botnet_config_t botnet_config;

static sockbuf_handler_t server_handler = {
	"botnet server",
	NULL, NULL, idx_on_newclient,
	NULL, NULL
};

static config_var_t botnet_config_vars[] = {
	{"my-ip", &botnet_config.ip, CONFIG_STRING},
	{"port", &botnet_config.port, CONFIG_INT},
	{0}
};

static event_owner_t bot_owner = {
	"botnet", NULL,
	NULL, NULL,
	bot_on_delete
};

static event_owner_t generic_owner = {
	"botnet", NULL,
	NULL, NULL,
	NULL
};

/*static event_owner_t sock_owner = {
	"oldbotnet", NULL,
	NULL, NULL,
	sock_on_delete
};*/

static bind_list_t party_binds[] = {
	{"n", "+bot", party_plus_bot},
	{"n", "-bot", party_minus_bot},
	{0}
};

static sockbuf_handler_t client_handler = {
	"botnet",
	idx_on_connect, idx_on_eof, NULL,
	idx_on_read, NULL,
	idx_on_delete
};

/* +bot <bot> <host> <port> */
static int party_plus_bot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text)
{
	char *name, *host, *port;
	user_t *bot;

	egg_get_words(text, NULL, &name, &host, &port, NULL);
	if (!port) {
		partymember_printf(p, _("Syntax: +bot <bot> <host> <port>"));
		goto done;
	}

	bot = user_new(name);
	if (!bot) {
		partymember_printf(p, _("Could not create bot '%s'."), name);
		goto done;
	}

	user_set_flags_str(bot, NULL, "+b");
	user_set_setting(bot, "bot", "type", "eggdrop");
	user_set_setting(bot, "bot", "host", host);
	user_set_setting(bot, "bot", "port", port);

done:
	if (name) free(name);
	if (host) free(host);
	if (port) free(port);
	return 0;
}

/* -bot <bot> */
static int party_minus_bot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text)
{
	char *type;
	user_t *bot;

	while (isspace(*text)) text++;

	if (!text || !*text) {
		partymember_printf(p, "Syntax: -bot <bot>");
		return(0);
	}

	bot = user_lookup_by_handle(text);
	if (!bot) {
		partymember_printf(p, _("Could not find user '%s'."), text);
		return(0);
	}

	user_get_setting(bot, "bot", "type", &type);
	if (!type || !(bot->flags | USER_BOT) || strcmp(type, "eggdrop")) {
		partymember_printf(p, _("Error: '%s' is not an eggdrop bot."), bot->handle);
		return(0);
	}

	partymember_printf(p, _("Deleting user '%s'."), bot->handle);
	user_delete(bot);
	return(BIND_RET_LOG);
}

static int do_link(user_t *user, const char *type)
{
	char *host = NULL, *portstr = NULL, *password = NULL;
	int port;
	bot_t *data;

	user_get_setting(user, NULL, "bot.host", &host);
	user_get_setting(user, NULL, "bot.port", &portstr);
	user_get_setting(user, NULL, "bot.password", &password);

	if (portstr) port = atoi(portstr);
	if (!host || !portstr || port < 0 || port > 65535) {
		putlog(LOG_MISC, "*", _("Error linking %s: Invalid telnet address:port stored."), user->handle);
		return BIND_RET_BREAK;
	}

	data = malloc(sizeof(*data));
	data->bot = NULL;
	data->user = user;
	data->idx = egg_connect(host, port, -1);
	if (password) data->password = strdup(password);
	else data->password = NULL;
	data->incoming = 0;
	data->linking = 1;
	data->version = 0;
	data->idle = 0;

	sockbuf_set_handler(data->idx, &client_handler, data);
	netstring_on(data->idx);

	putlog(LOG_MISC, "*", _("Linking to %s (%s %d) on idx %d."), user->handle, host, port, data->idx);
	return BIND_RET_BREAK;
}

static int idx_on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port)
{
	bot_t *session;

	session = malloc(sizeof(*session));
	session->bot = NULL;
	session->user = NULL;
	session->idx = newidx;
	session->password = NULL;
	session->incoming = 1;
	session->linking = 1;
	session->version = 0;
	session->idle = 0;

	sockbuf_set_handler(newidx, &client_handler, session);
	netstring_on(newidx);

	return 0;
}

static int idx_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	sockbuf_write(idx, "p", 1);

	return 0;
}

static int idx_on_read(void *client_data, int idx, char *data, int len)
{
	bot_t *bot = client_data;

	return 0;
}

static int idx_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	bot_t *bot = client_data;

	if (!bot->bot) {
		if (!bot->incoming) {
			if (bot->user->flags & USER_LINKING_BOT) botnet_link_failed(bot->user, errmsg ? errmsg : "no error");
			bot->user = NULL;   /* Might already be in the process of being reconnected, forget about it. */
		}
		sockbuf_delete(idx);
	} else {
		putlog(LOG_MISC, "*", _("eof from %s (%s)."), bot->bot->name, errmsg ? errmsg : "no error");
		botnet_delete(bot->bot, errmsg ? errmsg : "eof");
	}

	return 0;
}

static int idx_on_delete(void *client_data, int idx)
{
	bot_t *bot = client_data;

	bot->idx = -1;
	if (bot->bot) botnet_delete(bot->bot, _("Socket deleted."));
	else if (!bot->incoming && bot->user && bot->user->flags & USER_LINKING_BOT) botnet_link_failed(bot->user, "Socket deleted.");

	if (bot->password) free(bot->password);
	free(bot);

	return 0;
}

static int bot_on_delete(event_owner_t *owner, void *client_data)
{
	bot_t *bot = client_data;

	bot->bot = NULL;
	if (bot->idx >= 0) sockbuf_delete(bot->idx);

	return 0;
}

static void bot_init()
{
	int real_port;
	void *config_root;

	botnet_config.port = 3333;

	config_root = config_get_root("eggdrop");
	config_link_table(botnet_config_vars, config_root, "botnet", 0, NULL);
	config_update_table(botnet_config_vars, config_root, "botnet", 0, NULL);

	listen_idx = egg_server(botnet_config.ip, botnet_config.port, &real_port);
	sockbuf_set_handler(listen_idx, &server_handler, NULL);
}

static int bot_close(int why)
{
	void *config_root;

	config_root = config_get_root("eggdrop");
	config_unlink_table(botnet_config_vars, config_root, "botnet", 0, NULL);

	sockbuf_delete(listen_idx);

	return 0;
}

int botnet_LTX_start(egg_module_t *modinfo)
{
	bot_owner.module = generic_owner.module = modinfo;
	modinfo->name = "botnet";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "botnet support";
	modinfo->close_func = bot_close;
	
	bot_init();

	return 0;
}
