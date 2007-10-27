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
static const char rcsid[] = "$Id: botnet.c,v 1.5 2007/10/27 19:55:51 sven Exp $";
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
static int idx_on_delete(event_owner_t *owner, void *client_data);

static int got_bbroadcast(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_botmsg(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_bquit(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_broadcast(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_chanmsg(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_endlink(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_extension(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_join(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_link(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_login(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_newbot(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_nick(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_part(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_privmsg(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_quit(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
static int got_unlink(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);

static struct {
	char *cmd;
	int source;
	int min_argc;
	int (*function)(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len);
} cmd_mapping[] = {
	{"bbroadcast", ENTITY_BOT, 1, got_bbroadcast},
	{"botmsg", ENTITY_BOT, 1, got_botmsg},
	{"bquit", ENTITY_BOT, 0, got_bquit},
	{"broadcast", 0, 1, got_broadcast},
	{"chanmsg", 0, 2, got_chanmsg},
	{"el", ENTITY_BOT, 0, got_endlink},
	{"extension", 0, 2, got_extension},
	{"join", ENTITY_PARTYMEMBER, 1, got_join},
	{"link", 0, 2, got_link},
	{"login", ENTITY_BOT, 4, got_login},
	{"newbot", ENTITY_BOT, 4, got_newbot},
	{"nick", ENTITY_PARTYMEMBER, 1, got_nick},
	{"part", ENTITY_PARTYMEMBER, 1, got_part},
	{"privmsg", 0, 2, got_privmsg},
	{"quit", ENTITY_PARTYMEMBER, 0, got_quit},
	{"unlink", 0, 2, got_unlink}
};

static int cmd_num = sizeof(cmd_mapping) / sizeof(cmd_mapping[0]);

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

static event_owner_t sock_owner = {
	"botnet", NULL,
	NULL, NULL,
	idx_on_delete
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
	idx_on_read, NULL
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

static int get_entity(botnet_entity_t *ent, char *text)
{
	char *p;
	int id = -1;
	botnet_bot_t *bot;
	partymember_t *pm;

	p = strchr(text, '@');
	if (p) {
		*p = 0;
		id = b64dec_int(text);
		text = p + 1;
	}
	bot = botnet_lookup(text);
	if (!bot && strcmp(text, botnet_get_name())) return 1;
	if (id == -1) {
		set_bot_entity(ent, bot);
		return 0;
	}
	pm = partymember_lookup(NULL, bot, id);
	if (!pm) return 1;
	set_user_entity(ent, pm);
	return 0;
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
	if (!host || !portstr || port <= 0 || port > 65535) {
		putlog(LOG_MISC, "*", _("Error linking %s: Invalid telnet address:port stored."), user->handle);
		return BIND_RET_BREAK;
	}

	data = malloc(sizeof(*data));
	data->bot = NULL;
	data->user = user;
	data->idx = egg_connect(host, port, -1);
	data->proto = NULL;
	if (password) data->pass = strdup(password);
	else data->pass = NULL;
	data->incoming = 0;
	data->linking = 1;
	data->idle = 0;

	sockbuf_set_handler(data->idx, &client_handler, data, &sock_owner);
	netstring_on(data->idx);

	putlog(LOG_MISC, "*", _("Linking to %s (%s %d) on idx %d."), user->handle, host, port, data->idx);
	return BIND_RET_BREAK;
}

static int got_bbroadcast(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	if (argc <= 1) len = 0;
	botnet_botbroadcast(src->bot, argv[0], argv[argc - 1], len);

	return 0;
}

static int got_botmsg(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	botnet_entity_t dst;

	if (get_entity(&dst, argv[0])) return 0;
	if (dst.what != ENTITY_BOT) return 0;

	if (argc <= 2) len = 0;
	botnet_botmsg(src->bot, dst.bot, argv[1], argv[argc - 1], len);

	return 0;
}

static int got_bquit(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	botnet_delete(src->bot, len ? argv[0] : "No reason");

	return 0;
}

static int got_broadcast(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	botnet_broadcast(src, argv[0], len);

	return 0;
}

static int got_chanmsg(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	partychan_msg_name(argv[0], src, argv[1], len);

	return 0;
}

static int got_endlink(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	botnet_link_success(src->bot);

	return 0;
}

static int got_extension(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	botnet_entity_t dst;

	if (argc <= 2) len = 0;

	if (argv[0][0] == '*') {
		botnet_extension(EXTENSION_ALL, src, NULL, NULL, argv[1], argv[argc - 1], len);
	}

	if (get_entity(&dst, argv[0])) return 0;
	if (dst.what != ENTITY_BOT) return 0;

	botnet_extension(EXTENSION_ONE, src, dst.bot, NULL, argv[1], argv[argc - 1], len);

	return 0;
}

/* login channel [netburst] */

static int got_join(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	int netburst = 0;

	if (argc >= 2) netburst = b64dec_int(argv[1]) & 1;
	partychan_join_name(argv[0], src->user, netburst);

	return 0;
}

static int got_link(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	botnet_entity_t dst;

	if (get_entity(&dst, argv[0])) return 0;
	if (dst.what != ENTITY_BOT) return 0;

	botnet_link(src, dst.bot, argv[1]);
	return 0;
}

/* login nick ident host id */

static int got_login(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	partymember_new(b64dec_int(argv[3]), NULL, src->bot, argv[0], argv[1], argv[2], NULL, NULL, &generic_owner);
	return 0;
}

/* newbot uplink name type version fullversion linking */

static int got_newbot(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	int flags = 0;
	xml_node_t *info;
	botnet_bot_t *new;

	if (argc >= 5) flags = b64dec_int(argv[4]);

	info = xml_node_new();
	xml_node_set_str(argv[1], info, "type", 0, (void *) 0);
	xml_node_set_int(b64dec_int(argv[2]), info, "numversion", 0, (void *) 0);
	xml_node_set_str(argv[3], info, "version", 0, (void *) 0);

	new = botnet_new(argv[0], NULL, src->bot, bot->bot, info, NULL, NULL, &generic_owner, flags & 1);
	if (!new) {
		botnet_delete(bot->bot, _("Couldn't create introduced bot"));
		xml_node_delete(info);
		return 0;
	}

	return 0;
}

static int got_nick(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	partymember_set_nick(src->user, argv[0]);
	return 0;
}

static int got_part(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	char *reason = NULL;

	if (argc >= 2) reason = argv[argc - 1];
	partychan_part_name(argv[0], src->user, reason);

	return 0;
}

static int got_privmsg(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	partymember_t *dst;
	botnet_entity_t ent;

	if (get_entity(&ent, argv[0])) return 0;
	if (ent.what != ENTITY_PARTYMEMBER) return 0;
	dst = ent.user;
	partymember_msg(dst, src, argv[1], len);
	return 0;
}

static int got_quit(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	partymember_delete(src->user, NULL, len ? argv[0] : "No reason.");
	return 0;
}

static int got_unlink(bot_t *bot, botnet_entity_t *src, char *cmd, int argc, char *argv[21], int len)
{
	botnet_entity_t dst;

	if (get_entity(&dst, argv[0])) return 0;
	if (dst.what != ENTITY_BOT) return 0;

	botnet_link(src, dst.bot, argv[1]);
	return 0;
}

static int idx_on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port)
{
	bot_t *session;

	session = malloc(sizeof(*session));
	session->bot = NULL;
	session->user = NULL;
	session->idx = newidx;
	session->proto = NULL;
	session->pass = NULL;
	session->incoming = 1;
	session->linking = 1;
	session->idle = 0;

	sockbuf_set_handler(newidx, &client_handler, session, &sock_owner);
	netstring_on(newidx);

	return 0;
}

static int idx_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	egg_iprintf(idx, ":%s proto", botnet_get_name());

	return 0;
}

/*!
 * \brief Handles the login for incomming links.
 *
 * \param bot The ::bot_t struct for this link.
 * \param src The source of this message. This is a string!
 * \param cmd The command.
 * \param argc The number of parameters.
 * \param argv Up to 20 parameters. NULL terminated.
 * \param len The length of the last parameter.
 *
 * \return Always 0.
 */

static int recving_login(bot_t *bot, char *src, char *cmd, int argc, char *argv[], int len) {
	if (!bot->user) {
		if (!src) {
			sockbuf_delete(bot->idx);
			return 0;
		} else {
			bot->user = user_lookup_by_handle(src);
			if (!bot->user || !(bot->user->flags & USER_BOT) || !strcmp(src, botnet_get_name())) {
				sockbuf_delete(bot->idx);
				return 0;
			}
			if (botnet_lookup(src) || bot->user->flags & (USER_LINKING_BOT | USER_LINKED_BOT)) {
				sockbuf_delete(bot->idx);
				return 0;
			}
		}
	}
	if (src && strcmp(src, bot->user->handle)) {
		sockbuf_delete(bot->idx);
		return 0;
	}
	if (!strcasecmp(cmd, "PROTO")) {
		if (bot->proto) {
			sockbuf_delete(bot->idx);
			return 0;
		}
		bot->proto = malloc(sizeof(*bot->proto));
		bot->proto->dummy = 0;
		egg_iprintf(bot->idx, "PROTO");
	} else if (!strcasecmp(cmd, "HELLO")) {
		int i;
		char salt[33], *pass = NULL;
		unsigned char hash[16];
		MD5_CTX md5;

		if (bot->pass || argc != 1 || strcmp(argv[0], botnet_get_name())) {
			sockbuf_delete(bot->idx);
			return 0;
		}
		user_get_setting(bot->user, NULL, "bot.password", &pass);
		if (!pass || !*pass) {
			egg_iprintf(bot->idx, "thisbot eggdrop %s %s %s :%s", botnet_get_name(), "1090000", "eggdrop1.9.0+cvs", "some informative stuff");
			bot->pass = calloc(1, 1);
			return 0;
		}
		for (i = 0; i < 32; ++i) {
			salt[i] = random() % 62;
			if (salt[i] < 26) salt[i] += 'A';
			else if (salt[i] < 52) salt[i] += 'a';
			else salt[i] += '0';
		}
		salt[32] = 0;
		MD5_Init(&md5);
		MD5_Update(&md5, salt, 32);
		MD5_Update(&md5, pass, strlen(pass));
		MD5_Final(hash, &md5);
		bot->pass = malloc(33);
		MD5_Hex(hash, bot->pass);
		egg_iprintf(bot->idx, "passreq %s", salt);
	} else if (!strcasecmp(cmd, "PASS")) {
		if (!bot->pass || argc != 1 || strcmp(argv[0], bot->pass)) {
			sockbuf_delete(bot->idx);
			return 0;
		}
		*bot->pass = 0;
		egg_iprintf(bot->idx, ":%s thisbot eggdrop %s %s :%s", botnet_get_name(), b64enc_int(1090000), "eggdrop1.9.0+cvs", "some informative stuff");
	} else if (!strcasecmp(cmd, "THISBOT")) {
		xml_node_t *info;

		if (!bot->pass || *bot->pass || argc != 4) {
			sockbuf_delete(bot->idx);
			return 0;
		}
		free(bot->pass);
		bot->pass = NULL;
		bot->linking = 0;
		
		info = xml_node_new();
		xml_node_set_str(argv[0], info, "type", 0, (void *) 0);
		xml_node_set_int(b64dec_int(argv[1]), info, "numversion", 0, (void *) 0);
		xml_node_set_str(argv[2], info, "version", 0, (void *) 0);
		
		bot->bot = botnet_new(bot->user->handle, bot->user, NULL, NULL, info, &bothandler, bot, &bot_owner, 0);
		botnet_replay_net(bot->bot);
		egg_iprintf(bot->idx, "el");
	} else {
		sockbuf_delete(bot->idx);
	}
	return 0;
}

static int sending_login(bot_t *bot, char *src, char *cmd, int argc, char *argv[], int len)
{
	if (src && strcmp(src, bot->user->handle)) {
		egg_iprintf(bot->idx, "error :Wrong bot.");
		sockbuf_delete(bot->idx);
		return 0;
	}
	if (!strcasecmp(cmd, "PROTO")) {
		if (bot->proto) {
			egg_iprintf(bot->idx, "error :Been there, done that.");
			sockbuf_delete(bot->idx);
			return 0;
		}
		bot->proto = malloc(sizeof(*bot->proto));
		bot->proto->dummy = 0;
		egg_iprintf(bot->idx, "hello %s", bot->user->handle);
	} else if (!strcasecmp(cmd, "PASSREQ")) {
		char buf[33];
		unsigned char hash[16];
		MD5_CTX md5;
		if (argc != 1 || !bot->proto || !bot->pass || !*bot->pass) {
			if (!bot->pass) putlog(LOG_MISC, "*", "botnet error: password on %s needs to be reset.", bot->user->handle);
			egg_iprintf(bot->idx, "error :Expected something else.");
			sockbuf_delete(bot->idx);
			return 0;
		}
		MD5_Init(&md5);
		MD5_Update(&md5, argv[0], len);
		MD5_Update(&md5, bot->pass, strlen(bot->pass));
		MD5_Final(hash, &md5);
		MD5_Hex(hash, buf);
		egg_iprintf(bot->idx, "pass %s", buf);
		*bot->pass = 0;
	} else if (!strcasecmp(cmd, "THISBOT")) {
		xml_node_t *info;

		if (argc != 4) {
			sockbuf_delete(bot->idx);
			return 0;
		}
		egg_iprintf(bot->idx, ":%s thisbot eggdrop %s %s :%s", botnet_get_name(), b64enc_int(1090000), "eggdrop1.9.0+cvs", "some informative stuff");
		free(bot->pass);
		bot->pass = NULL;
		bot->linking = 0;

		info = xml_node_new();
		xml_node_set_str(argv[0], info, "type", 0, (void *) 0);
		xml_node_set_int(b64dec_int(argv[1]), info, "numversion", 0, (void *) 0);
		xml_node_set_str(argv[2], info, "version", 0, (void *) 0);

		bot->bot = botnet_new(bot->user->handle, bot->user, NULL, NULL, info, &bothandler, bot, &bot_owner, 0);
		botnet_replay_net(bot->bot);
		egg_iprintf(bot->idx, "el");
	} else {
		sockbuf_delete(bot->idx);
	}
	return 0;
}

static int idx_on_read(void *client_data, int idx, char *data, int len)
{
	int argc = 0;
	char *start = data, *p, *srcstr = NULL, *argv[22];
	bot_t *bot = client_data;
	botnet_entity_t src;

	if (!len) return 0;
	if (*data == ':') {
		srcstr = data + 1;
		p = strchr(srcstr, ' ');
		if (!p) return 0;
		*p = 0;
		data = p + 1;
		while (isspace(*data)) ++data;
	}

	while (*data) {
		argv[argc++] = data;
		if (*data == ':') {
			argv[argc - 1]++;
			break;
		}
		if (argc == 21) break;
		p = strchr(data, ' ');
		if (!p) break;
		*p = 0;
		data = p + 1;
		while (isspace(*data)) ++data;
	}

	if (!argc) return 0;

	len -= argv[argc - 1] - start;
	argv[argc] = NULL;
	if (!bot->bot) {
		if (bot->incoming) return recving_login(bot, srcstr, argv[0], argc - 1, argv + 1, len);
		else return sending_login(bot, srcstr, argv[0], argc - 1, argv + 1, len);
	}

	if (srcstr) {
		char *at;
		botnet_bot_t *srcbot;

		at = strchr(srcstr, '@');
		if (get_entity(&src, srcstr)) {
			if (at && !*at) putlog(LOG_MISC, "*", _("Botnet: Desync! %s says %s came from %s@%s who doesn't exist!"), bot->bot->name, argv[0], srcstr, at + 1);
			else putlog(LOG_MISC, "*", _("Botnet: Desync! %s says %s came from %s who doesn't exist!"), bot->bot->name, argv[0], srcstr);
			return 0;
		}
		if (src.what == ENTITY_BOT) srcbot = src.bot;
		else srcbot = src.user->bot;
		if (botnet_check_direction(bot->bot, srcbot)) return 0;
	} else {
		set_bot_entity(&src, bot->bot);
	}

	int min = 0, max = cmd_num - 1, cur = max / 2;
	while (min <= max) {
		int ret = strcasecmp(argv[0], cmd_mapping[cur].cmd);
		if (!ret) {
			if (argc < cmd_mapping[cur].min_argc) return 0;
			if (cmd_mapping[cur].source && cmd_mapping[cur].source != src.what) return 0;
			return cmd_mapping[cur].function(bot, &src, argv[0], argc - 1, argv + 1, len);
		} else if (ret < 0) {
			max = cur - 1;
		} else {
			min = cur + 1;
		}
		cur = (min + max) / 2;
	}
	putlog(LOG_MISC, "*",  _("Botnet: Got unknown something from %s: %s"), bot->user->handle, argv[0]);
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

static int idx_on_delete(event_owner_t *owner, void *client_data)
{
	bot_t *bot = client_data;

	bot->idx = -1;
	if (bot->bot) botnet_delete(bot->bot, _("Socket deleted."));
	else if (!bot->incoming && bot->user && bot->user->flags & USER_LINKING_BOT) botnet_link_failed(bot->user, "Socket deleted.");

	if (bot->pass) free(bot->pass);
	free(bot);

	return 0;
}

/*!
 * \brief on_delete callback for ::botnet_bot_t.
 *
 * Gets called every time a directly linked bot created by this module is
 * deleted. Marks it as deleted in the ::bot_t struct and deletes the
 * ::sockbuf_t.
 *
 * \param owner The ::event_owner_t struct belonging to this bot. It's bot_owner.
 * \param client_data Our callback data. The ::bot_t struct of the deleted bot.
 * \return Always 0.
 */

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
	sockbuf_set_handler(listen_idx, &server_handler, NULL, &sock_owner);

	bind_add_simple(BTN_BOTNET_REQUEST_LINK, NULL, "eggdrop", do_link);
}

static int bot_close(int why)
{
	void *config_root;

	config_root = config_get_root("eggdrop");
	config_unlink_table(botnet_config_vars, config_root, "botnet", 0, NULL);

	sockbuf_delete(listen_idx);

	bind_rem_simple(BTN_BOTNET_REQUEST_LINK, NULL, "eggdrop", do_link); 

	return 0;
}

int botnet_LTX_start(egg_module_t *modinfo)
{
	bot_owner.module = sock_owner.module = generic_owner.module = modinfo;
	modinfo->name = "botnet";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "botnet support";
	modinfo->close_func = bot_close;
	
	bot_init();

	return 0;
}
