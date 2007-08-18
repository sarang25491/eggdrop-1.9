/* oldbotnet.c: support for linking with pre-1.9 bots
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
static const char rcsid[] = "$Id: oldbotnet.c,v 1.22 2007/08/18 22:32:24 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "oldbotnet.h"

EXPORT_SCOPE int oldbotnet_LTX_start(egg_module_t *modinfo);
static int oldbotnet_close(int why);
static int do_link(user_t *u, const char *text);
static int bot_on_delete(event_owner_t *owner, void *client_data);
static int anonymous_on_delete(event_owner_t *owner, void *client_data);
//static int sock_on_delete(event_owner_t *owner, void *client_data);

/* Partyline commands. */
static int party_plus_obot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text);
static int party_minus_obot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text);

/* Oldbotnet commands. */
static int got_actchan(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_away(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_bcast(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_botmsg(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_botbroadcast(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_bye(botnet_bot_t *bot, const char *cmd, const char *text);
static int got_chat(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_endlink(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_handshake(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_idle(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_join(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_motd(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_nickchange(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_part(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_ping(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_pong(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_link(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_nlinked(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_unlink(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_unlinked(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_versions(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_thisbot(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_privmsg(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_who(botnet_bot_t *bot, const char *cmd, const char *next);

/* Sockbuf handler. */
static int oldbotnet_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int oldbotnet_on_read(void *client_data, int idx, char *data, int len);
static int oldbotnet_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int oldbotnet_on_delete(void *client_data, int idx);

static event_owner_t bot_owner = {
	"oldbotnet", NULL,
	NULL, NULL,
	bot_on_delete
};

static event_owner_t anonymous_owner = {
	"oldbotnet", NULL,
	NULL, NULL,
	anonymous_on_delete
};

static event_owner_t generic_owner = {
	"oldbotnet", NULL,
	NULL, NULL,
	NULL
};

/*static event_owner_t sock_owner = {
	"oldbotnet", NULL,
	NULL, NULL,
	sock_on_delete
};*/

typedef struct assoc {
	char *name;
	int id;
	int dynamic;
	time_t last_bounced;
	struct assoc *next;
} assoc_t;

static assoc_t assocs = {
	"*",
	0,
	0,
	0,
	NULL
};

static bind_list_t party_binds[] = {
	{"n", "+obot", party_plus_obot},
	{"n", "-obot", party_minus_obot},
	{0}
};

static bind_list_t obot_binds[] = {
	{NULL, "a", got_actchan},
	{NULL, "actchan", got_actchan},

	{NULL, "aw", got_away},
	{NULL, "away", got_away},
	{NULL, "unaway", got_away},

	{NULL, "bye", got_bye},

	{NULL, "c", got_chat},
	{NULL, "chan", got_chat},

	{NULL, "ct", got_bcast}, 
	{NULL, "chat", got_bcast},

	{NULL, "el", got_endlink},

	{NULL, "h", got_handshake},
	{NULL, "handshake", got_handshake},

	{NULL, "i", got_idle},
	{NULL, "idle", got_idle},

	{NULL, "j", got_join},
	{NULL, "join", got_join},

	{NULL, "l", got_link},
	{NULL, "link", got_link},

	{NULL, "m", got_motd},
	{NULL, "motd", got_motd},

	{NULL, "n", got_nlinked},
	{NULL, "nlinked", got_nlinked},

	{NULL, "nc", got_nickchange},

	{NULL, "p", got_privmsg},
	{NULL, "priv", got_privmsg},

	{NULL, "pi", got_ping},
	{NULL, "ping", got_ping},

	{NULL, "po", got_pong},
	{NULL, "pong", got_pong},

	{NULL, "pt", got_part},
	{NULL, "part", got_part},

	{NULL, "tb", got_thisbot},
	{NULL, "thisbot", got_thisbot},

	{NULL, "ul", got_unlink},
	{NULL, "unlink", got_unlink},

	{NULL, "v", got_versions},

	{NULL, "un", got_unlinked},
	{NULL, "unlinked", got_unlinked},

	{NULL, "w", got_who},
	{NULL, "who", got_who},

	{NULL, "z", got_botmsg},
	{NULL, "zapf", got_botmsg},

	{NULL, "zb", got_botbroadcast},
	{NULL, "zapf-broad", got_botbroadcast},
	{0}
};

static sockbuf_handler_t oldbotnet_handler = {
	"oldbotnet",
	oldbotnet_on_connect, oldbotnet_on_eof, NULL,
	oldbotnet_on_read, NULL,
	oldbotnet_on_delete
};

static bind_table_t *BT_obot = NULL;

const char *assoc_get_name(int id)
{
	int try = 0;
	assoc_t *a;

	for (a = &assocs; a; a = a->next) {
		if (a->id == id) break;
	}
	if (a) return a->name;

	a = malloc(sizeof(*a));
	do {
		if (!try) {
			a->name = egg_mprintf("%d", id);
		} else {
			free(a->name);
			a->name = egg_mprintf("%d_%d", id, try + 1);
		}
		++try;
	} while (partychan_lookup_name(a->name));
	a->id = id;
	a->next = assocs.next;
	a->dynamic = 1;
	a->last_bounced = 0;
	assocs.next = a;
	return a->name;
}

int assoc_get_id(const char *name)
{
	int highest_id = 0;
	assoc_t *a;

	for (a = &assocs; a; a = a->next) {
		if (!strcmp(a->name, name)) break;
		if (a->id > highest_id) highest_id = a->id;
	}
	if (a) return a->id;

	a = malloc(sizeof(*a));
	a->name = strdup(name);
	a->id = highest_id + 1;
	a->last_bounced = 0;
	a->dynamic = 0;
	a->next = assocs.next;
	assocs.next = a;
	return a->id;
}

static int get_entity(botnet_bot_t *bot, botnet_entity_t *src, char *word)
{
	oldbotnet_t *obot = bot->client_data;

	if (!strchr(word, '@')) {
		botnet_bot_t *b = botnet_lookup(word);
		if (botnet_check_direction(bot, b)) return -1;
		set_bot_entity(src, b);
		return 0;
	}

	partymember_t *p = partymember_lookup(word, NULL, -1);
	if (p) {
		set_user_entity(src, p);
		return 0;
	}

	int id = -1;
	char *nick = word, *ptr = strchr(word, '@');
	botnet_bot_t *srcbot = botnet_lookup(ptr + 1);

	if (botnet_check_direction(bot, srcbot)) return -1;
	*ptr = 0;
	ptr = strchr(word, ':');
	if (ptr) {
		*ptr = 0;
		id = atoi(word);
		nick = ptr + 1;
	}
	if (obot->anonymous) partymember_delete(obot->anonymous, NULL, "Temp user expired");
	obot->anonymous = p = partymember_new(id, NULL, srcbot, nick, "temp", "user.on.an.old.bot", NULL, obot, &anonymous_owner);
	if (!p) return 0;
	set_user_entity(src, p);
	return 1;
}

/* +obot <obot> <host> <port> [username] [password] */
static int party_plus_obot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text)
{
	char *name, *host, *port, *username, *password;
	user_t *obot;

	egg_get_words(text, NULL, &name, &host, &port, &username, &password, NULL);
	if (!port) {
		partymember_printf(p, _("Syntax: +obot <obot> <host> <port> [username] [password]"));
		goto done;
	}

	obot = user_new(name);
	if (!obot) {
		partymember_printf(p, _("Could not create obot '%s'."), name);
		goto done;
	}

	user_set_flags_str(obot, NULL, "+b");
	user_set_setting(obot, "bot", "type", "old-eggdrop");
	user_set_setting(obot, "bot", "host", host);
	user_set_setting(obot, "bot", "port", port);
	if (username) user_set_setting(obot, "bot", "username", username);
	if (password) user_set_setting(obot, "bot", "password", password);

done:
	if (name) free(name);
	if (host) free(host);
	if (port) free(port);
	if (username) free(username);
	if (password) free(password);
	return 0;
}

/* -obot <obot> */
static int party_minus_obot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text)
{
	char *host;
	user_t *obot;

	while (isspace(*text)) text++;

	if (!text || !*text) {
		partymember_printf(p, "Syntax: -obot <obot>");
		return(0);
	}

	obot = user_lookup_by_handle(text);
	if (!obot) {
		partymember_printf(p, _("Could not find user '%s'."), text);
		return(0);
	}

	user_get_setting(obot, "oldbotnet", "host", &host);
	if (!host) {
		partymember_printf(p, _("Error: '%s' is not an obot."), obot->handle);
		return(0);
	}

	partymember_printf(p, _("Deleting user '%s'."), obot->handle);
	user_delete(obot);
	return(BIND_RET_LOG);
}

static int do_link(user_t *user, const char *type)
{
	char *host = NULL, *portstr = NULL, *un = NULL, *password = NULL;
	const char *username;
	int port;
	oldbotnet_t *data;

	user_get_setting(user, NULL, "bot.host", &host);
	user_get_setting(user, NULL, "bot.port", &portstr);
	user_get_setting(user, NULL, "bot.username", &un);
	user_get_setting(user, NULL, "bot.password", &password);
	if (un) username = un;
	else username = botnet_get_name();
	if (portstr) port = atoi(portstr);
	if (!host || !portstr || port < 0 || port > 65535) {
		putlog(LOG_MISC, "*", _("Error linking %s: Invalid telnet address:port stored."), user->handle);
		return BIND_RET_BREAK;
	}

	data = malloc(sizeof(*data));
	data->bot = NULL;
	data->user = user;
	data->name = strdup(username);
	if (password) data->password = strdup(password);
	else data->password = NULL;
	data->idx = egg_connect(host, port, -1);
	data->idle = 0;
	data->anonymous = NULL;

	sockbuf_set_handler(data->idx, &oldbotnet_handler, data);
	linemode_on(data->idx);

	putlog(LOG_MISC, "*", _("Linking to %s (%s %d) on idx %d as %s."), user->handle, host, port, data->idx, data->name);
	return BIND_RET_BREAK;
}

/*!
 * \brief Send a password for login.
 *
 * The other bot has sent us a pseudorandom string. We append the plaintext
 * password to this string and encode the whole thing with MD5. This way the
 * actual password isn't send over the internet.
 *
 * \param bot We're not yet linked, so this is the ::oldbotnet_t struct.
 * \param next The challenge string the other bot has sent.
 */

static void got_passreq(oldbotnet_t *bot, const char *next)
{
	if (!bot->password) {
		putlog(LOG_MISC, "*", _("oldbotnet error: password on %s needs to be reset."), bot->user->handle);
		sockbuf_delete(bot->idx);
		return;
	}

	if (next && *next == '<') {
		MD5_CTX md5;
		unsigned char hash[16];
		char hex[33];

		MD5_Init(&md5);
		MD5_Update(&md5, next, strlen(next));
		MD5_Update(&md5, bot->password, strlen(bot->password));
		MD5_Final(hash, &md5);
		MD5_Hex(hash, hex);
		putlog(LOG_MISC, "*", _("Received challenge from %s... sending response ..."), bot->user->handle);
		egg_iprintf(bot->idx, "digest %s\n", hex);
	} else {
		egg_iprintf(bot->idx, "%s\n", bot->password);
	}
}

/*!
 * \brief Handles a "h" or "handshake" event from a linked bot.
 *
 * "handshake" means the linked bot set a new link password. Just save it.
 *
 * \format \b handshake new_password
 *
 * \param bot The bot the text came from.
 * \param cmd The first word it sent, the command. Always "h" or "handshake".
 * \param text Just one word: the new password in plain text.
 */

static int got_handshake(botnet_bot_t *bot, const char *cmd, const char *text)
{
	if (!text || !*text) return BIND_RET_BREAK;
	putlog(LOG_MISC, "*", "Saving password '%s'", text);
	user_set_setting(bot->user, NULL, "botnet.password", text);
	return BIND_RET_BREAK;
}

/*!
 * \brief Handle the version command.
 *
 * This function handles the version command. Once the other bot has sent it
 * the linking process is completed. We will send our version command after
 * we've seen the other bot's version so we'll know what handle length to send.
 * After that send our botnet and endlink.
 *
 * \format version numeric_version hand_len some version string \<network\>
 *
 * \param bot The bot it came from.
 * \param next The version text.
 *
 * \todo Send the actual version and network out here.
 */

static void got_version(oldbotnet_t *bot, const char *next)
{
	char buf[32];
	/* Get their version and handlen. */
	sscanf(next, "%d %d", &bot->version, &bot->handlen);

	/* Send back our version reply with their handlen. */
	egg_iprintf(bot->idx, "version 1090000 %d eggdrop v1.9 <alrighty.then>\n", bot->handlen);
	egg_iprintf(bot->idx, "tb %s\n", bot->name);

	/* And now we're connected. */
	bot->linking = 1;
	bot->bot = botnet_new(bot->user->handle, bot->user, NULL, NULL, &bothandler, bot, &bot_owner, 0);
	if (!bot->bot) {
		botnet_link_failed(bot->user, "Could not create bot.");
		bot->user = NULL;
		sockbuf_delete(bot->idx);
		return;
	}
	if (bot->version % 100) sprintf(buf, "eggdrop%d.%d.%d.%d", bot->version / 1000000, bot->version / 10000 % 100, bot->version / 100 % 100, bot->version % 100);
	else sprintf(buf, "eggdrop%d.%d.%d", bot->version / 1000000, bot->version / 10000 % 100, bot->version / 100 % 100);
	botnet_set_info(bot->bot, "type", "eggdrop");
	botnet_set_info_int(bot->bot, "numversion", bot->version);
	botnet_set_info(bot->bot, "version", buf);
	botnet_replay_net(bot->bot);
	egg_iprintf(bot->idx, "el\n");
}

static int got_actchan(botnet_bot_t *bot, const char *cmd, const char *next)
{
	char *word[2], buf[1024], *action;
	partymember_t *p;
	int n, len;

	n = egg_get_word_array(next, &next, word, 2);
	if (n != 2 || !next) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	p = partymember_lookup(word[0], NULL, -1);
	if (!p || botnet_check_direction(bot, p->bot)) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}
	botnet_entity_t src = user_entity(p);

	while (isspace(*next)) next++;
	action = egg_msprintf(buf, sizeof(buf), &len, "\1ACTION %s\1", next);
	partychan_msg_name(assoc_get_name(b64dec_int(word[1])), &src, action, len);  
	if (action != buf) free(action);

	egg_free_word_array(word, 2);
	return BIND_RET_BREAK;
}

static int got_bcast(botnet_bot_t *bot, const char *cmd, const char *text)
{
	char *srcname;
	botnet_bot_t *srcbot;

	if (egg_get_word(text, &text, &srcname)) return BIND_RET_BREAK;
	while (isspace(*text)) text++;

	srcbot = botnet_lookup(srcname);
	free(srcname);
	if (!botnet_check_direction(bot, srcbot)) return BIND_RET_BREAK;

	botnet_entity_t src = bot_entity(srcbot);
	botnet_broadcast(&src, text, -1);

	return BIND_RET_BREAK;
}

/*!
 * \brief Handle a ping.
 *
 * It's a ping. Send a pong.
 *
 * \param bot The bot the ping came from.
 * \param cmd "ping" or "pi".
 * \param next Nothing.
 */

static int got_ping(botnet_bot_t *bot, const char *cmd, const char *next)
{
	oldbotnet_t *obot = bot->client_data;

	egg_iprintf(obot->idx, "po\n");
	return BIND_RET_BREAK;
}

static int got_endlink(botnet_bot_t *bot, const char *cmd, const char *next)
{
	oldbotnet_t *obot = bot->client_data;

	obot->linking = 0;
	botnet_link_success(obot->bot);
	return BIND_RET_BREAK;
}

static int got_thisbot(botnet_bot_t *bot, const char *cmd, const char *next)
{
	if (strcmp(bot->name, next)) {
		putlog(LOG_MISC, "*", "Wrong bot--wanted %s, got %s", bot->name, next);
		botnet_delete(bot, "imposer");
	}
	return BIND_RET_BREAK;
}

static int got_motd(botnet_bot_t *bot, const char *cmd, const char *next)
{
	char *word[2];
	int n;
	botnet_bot_t *dst;
	botnet_entity_t src;

	n = egg_get_word_array(next, NULL, word, 2);
	if (n != 2) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	dst = botnet_lookup(word[1]);
	if (!dst && strcmp(word[1], botnet_get_name())) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	n = get_entity(bot, &src, word[0]);
	if (n < 0 || src.what != ENTITY_PARTYMEMBER || botnet_check_direction(bot, src.user->bot)) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	botnet_extension(EXTENSION_ONE, &src, dst, NULL, "motd", "", 0);
	return BIND_RET_BREAK;
}

static int got_versions(botnet_bot_t *bot, const char *cmd, const char *next)
{
	char *word[3];
	int n;
	botnet_bot_t *dst, *srcbot;
	partymember_t *src;

	n = egg_get_word_array(next, NULL, word, 3);
	if (n != 3) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	srcbot = botnet_lookup(word[0]);
	dst = botnet_lookup(word[1]);
	if (!srcbot || botnet_check_direction(bot, srcbot) || (!dst && strcmp(word[1], botnet_get_name()))) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	src = partymember_lookup(word[2], srcbot, -1);
	if (!src) {
		int id = -1;
		char *ptr = strchr(word[2], ':');
		oldbotnet_t *obot = bot->client_data;

		if (ptr) {
			*ptr = 0;
			id = atoi(word[2]);
			++ptr;
		} else {
			ptr = word[2];
		}
		if (obot->anonymous) partymember_delete(obot->anonymous, NULL, "Temp user expired");
		obot->anonymous = src = partymember_new(id, NULL, srcbot, ptr, "temp", "user.on.an.old.bot", NULL, obot, &anonymous_owner);
	}
	botnet_entity_t e = user_entity(src);

	botnet_extension(EXTENSION_ONE, &e, dst, NULL, "versions", "", 0);
	return BIND_RET_BREAK;
}

static int got_nickchange(botnet_bot_t *bot, const char *cmd, const char *next)
{
	char *word[3];
	int n, id;
	botnet_bot_t *srcbot;
	partymember_t *src;

	n = egg_get_word_array(next, NULL, word, 3);
	if (n != 3) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	srcbot = botnet_lookup(word[0]);
	if (botnet_check_direction(bot, srcbot)) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	id = b64dec_int(word[1]);
	src = partymember_lookup(NULL, srcbot, id);
	if (!src) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	partymember_set_nick(src, word[2]);

	return BIND_RET_BREAK;
}

/*!
 * \brief A new bot has linked to the botnet somewhere else.
 *
 * Triggered every time a new bot joins the net to introduce that bot and all
 * the bots behind it. There might be an '!' as the first char of the version
 * string. If that isn't there, don't announce the new bot on the partyline,
 * it's probably from a net join. Might get spammy otherwise.
 *
 * \param bot The bot the msg came from.
 * \param cmd "nlinked" or "n".
 * \param next The parameters.
 *
 * \format \b nlinked newbot uplink [!]version
 */

static int got_nlinked(botnet_bot_t *bot, const char *cmd, const char *next)
{
	char *word[3], buf[32];
	int n, linking = 1, version;
	botnet_bot_t *src, *new;
	oldbotnet_t *obot = bot->client_data;

	n = egg_get_word_array(next, NULL, word, 3);
	if (n != 3) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	src = botnet_lookup(word[1]);
	if (botnet_check_direction(bot, src)) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	new = botnet_lookup(word[0]);
	if (new) {
		putlog(LOG_MISC, "*", "Botnet loop detected! %s introduced %s from %s but it exists from %s\n", bot->name, new->name, src->name, new->direction->name);
		egg_iprintf(obot->idx, "error Loop! (%s exists from %s)\nbye Loop (%s)\n", new->name, new->direction->name, new->name);
		botnet_delete(bot, "Loop detected!");
		return BIND_RET_BREAK;
	}

	if (word[2][0] == '!') {
		linking = 0;
		version = b64dec_int(word[2] + 1);
	} else {
		version = b64dec_int(word[2]);
	}
	new = botnet_new(word[0], NULL, src, bot, NULL, NULL, &generic_owner, linking);
	if (!new) {
		/* Invalid botname ... should probably do some really clever name mangleing here ... */
		char obuf[512];
		snprintf(obuf, sizeof(obuf), "Botname incompatiblity: %s linked from %s", word[0], src->name);
		botnet_delete(bot, obuf);
		/* or just be lazy and kill the bot -_- */
	}
	if (version % 100) sprintf(buf, "eggdrop%d.%d.%d.%d", version / 1000000, version / 10000 % 100, version / 100 % 100, version % 100);
	else sprintf(buf, "eggdrop%d.%d.%d", version / 1000000, version / 10000 % 100, version / 100 % 100);
	botnet_set_info(new, "type", "eggdrop");
	botnet_set_info_int(new, "numversion", version);
	botnet_set_info(new, "version", buf);
	return BIND_RET_BREAK;
}

static int got_bye(botnet_bot_t *bot, const char *cmd, const char *text)
{
	botnet_delete(bot, text);
	return BIND_RET_BREAK;
}

/*!
 * \brief Handle pong event.
 *
 * Actually, don't handle pong event. The fact that a line was received is
 * enough to reset the bots idle counter and that already happened. So there's
 * nothing left to do.
 *
 * \param bot The bot the msg came from.
 * \param cmd "pong" or "po".
 * \param next Nothing.
 *
 * \format \b pong
 */

static int got_pong(botnet_bot_t *bot, const char *cmd, const char *next)
{
	return BIND_RET_BREAK;
}

/*!
 * \brief Remote request to unlink a bot
 *
 * Someone on another bot wants us or a bot further downstream to unlink from
 * a bot. In any case, the details are handled by botnet_unlink(), we just
 * pass it on.
 *
 * \param bot The bot the msg came from.
 * \param cmd "unlink" or "ul".
 * \param text The parameters.
 *
 * \format \b unlink from@@bot dst bot-to-kill [reason]
 */

static int got_unlink(botnet_bot_t *bot, const char *cmd, const char *text)
{
	int n;
	char *word[3];
	const char *reason;
	botnet_bot_t *target;
	botnet_entity_t src;

	n = egg_get_word_array(text, &text, word, 3);
	if (n != 3) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}
	while (isspace(*text)) text++;
	if (text && *text) reason = text;
	else reason = "No reason.";

	target = botnet_lookup(word[2]);

	if (!target) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	n = get_entity(bot, &src, word[0]);
	if (n < 0) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	botnet_unlink(&src, target, reason);
	return BIND_RET_BREAK;
}

static int got_away(botnet_bot_t *bot, const char *cmd, const char *next)
{
	char *word[2];
	int n, linking = 0;
	botnet_bot_t *src;
	partymember_t *p;

	n = egg_get_word_array(next, &next, word, 2);
	if (n != 2) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	if (word[0][0] == '!') {
		linking = 1;
		src = botnet_lookup(word[0] + 1);
	} else {
		src = botnet_lookup(word[0]);
	}
	if (botnet_check_direction(bot, src)) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	p = partymember_lookup(NULL, src, b64dec_int(word[1]));
	if (!p) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	while (isspace(*next)) ++next;

	botnet_entity_t s = user_entity(p);
	botnet_extension(EXTENSION_ALL, &s, NULL, NULL, "away", next, -1);

	return BIND_RET_BREAK;
}

static int got_unlinked(botnet_bot_t *bot, const char *cmd, const char *text)
{
	char *lostname;
	botnet_bot_t *lost;

	if (egg_get_word(text, &text, &lostname)) return BIND_RET_BREAK;

	lost = botnet_lookup(lostname);
	free(lostname);
	if (botnet_check_direction(bot, lost)) return BIND_RET_BREAK;

	while (isspace(*text)) ++text;
	botnet_delete(lost, text);
	return BIND_RET_BREAK;
}

static int got_idle(botnet_bot_t *bot, const char *cmd, const char *next)
{
	/*! We don't care.
	 *  \todo Care! */
	 return BIND_RET_BREAK;
}

static int got_join(botnet_bot_t *bot, const char *cmd, const char *next)
{
	char *word[5];
	partymember_t *p;
	char *ident, *sep, *host, *from;
	botnet_bot_t *frombot;
	oldbotnet_t *obot = bot->client_data;
	int n, id, linking = obot->linking;

	n = egg_get_word_array(next, NULL, word, 5);
	if (n != 5) {
		egg_free_word_array(word, 5);
		return BIND_RET_BREAK;
	}

	from = word[0];
	if (*from == '!') {
		from++;
		linking = 1;
	}
	frombot = botnet_lookup(from);

	if (botnet_check_direction(bot, frombot)) {
		egg_free_word_array(word, 5);
		return BIND_RET_BREAK;
	}

	id = b64dec_int(word[3] + 1);

	p = partymember_lookup(NULL, frombot, id);
	if (p) {
		if (p == obot->anonymous) {
			partymember_delete(p, NULL, "Temp user expired");
			p = NULL;
		} else if (strcmp(p->nick, word[1])) {
			partymember_delete(p, NULL, "Botnet desync");
			p = NULL;
		}
	}

	if (p) {
		/* We already know about him. */
		partychan_t *chan;
		chan = partychan_get_default(p);
		partychan_part(chan, p, "joining another channel");
	}
	else {
		/* Nope, he's new. */
		ident = word[4];
		sep = strchr(ident,'@');
		if (sep) {
			*sep = 0;
			host = sep+1;
		}
		else host = word[0];

		p = partymember_new(id, NULL, frombot, word[1], ident, host, NULL, NULL, &generic_owner);
	}
	partychan_join_name(assoc_get_name(b64dec_int(word[2])), p, linking);
	egg_free_word_array(word, 5);
	return BIND_RET_BREAK;
}

static int got_part(botnet_bot_t *bot, const char *cmd, const char *next)
{
	int id, n;
	char *word[3];
	const char *reason;
	botnet_bot_t *src;
	partymember_t *p;

	n = egg_get_word_array(next, &next, word, 3);
	if (n != 3) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}
	while (isspace(*next)) next++;
	if (next && *next) reason = next;
	else reason = "No reason.";

	src = botnet_lookup(word[0]);
	id = b64dec_int(word[2]);
	if (botnet_check_direction(bot, src)) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	p = partymember_lookup(word[1], src, id);
	if (p) partymember_delete(p, NULL, reason);
	egg_free_word_array(word, 3);
	return BIND_RET_BREAK;
}

/*!
 * \brief Private message for someone.
 *
 * A partymember or a bot have a private message for someone, local or remote.
 * In any case, the details are handled by insert_here(), we just pass it on.
 *
 * \param bot The bot the msg came from.
 * \param cmd "priv" or "p".
 * \param next The parameters.
 *
 * \format \b priv from@@bot dst message
 */

static int got_privmsg(botnet_bot_t *bot, const char *cmd, const char *next)
{
	int n;
	char *word[2];
	const char *original = next;
	botnet_entity_t src;
	partymember_t *dst;

	n = egg_get_word_array(next, &next, word, 2);
	if (n != 2) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}
	while (isspace(*next)) next++;
	if (!next || !*next) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	n = get_entity(bot, &src, word[0]);
	if (n < 0) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	dst = partymember_lookup(word[1], NULL, -1);

	if (!dst) {
		char *p = strchr(word[1], '@');
		if (p) {
			botnet_bot_t *dstbot = botnet_lookup(p + 1);
			if (dstbot || !strcmp(p + 1, botnet_get_name())) {
				botnet_extension(EXTENSION_ONE, &src, dstbot, NULL, "note", strchr(original, ' ') + 1, -1);
			}
		}
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	partymember_msg(dst, &src, next, -1);
	return BIND_RET_BREAK;
}

static int got_chat(botnet_bot_t *bot, const char *cmd, const char *next)
{
	char *word[2];
	int n;
	botnet_entity_t src;

	n = egg_get_word_array(next, &next, word, 2);
	if (n != 2 || !next) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	if (strchr(word[0], '@')) {
		partymember_t *p = partymember_lookup(word[0], NULL, -1);
		if (!p || botnet_check_direction(bot, p->bot)) {
			egg_free_word_array(word, 2);
			return BIND_RET_BREAK;
		}
		set_user_entity(&src, p);
	} else {
		botnet_bot_t *b = botnet_lookup(word[0]);
		if (!b || !botnet_check_direction(bot, b)) {
			egg_free_word_array(word, 2);
			return BIND_RET_BREAK;
		}
		set_bot_entity(&src, b);
	}

	while (isspace(*next)) next++;
	partychan_msg_name(assoc_get_name(b64dec_int(word[1])), &src, next, strlen(next));

	egg_free_word_array(word, 2);
	return BIND_RET_BREAK;
}

static void got_assoc(botnet_bot_t *src, const char *text)
{
	
}

/*!
 * \brief Handle a message from a bot for a bot.
 *
 * This is the old "zapf" message, whatever that may mean. It's called botmsg
 * in this version. It's some text from a script on a bot for another script
 * on another bot. If there's no script listening for the message on the
 * destination bot it'll be ignored.
 *
 * \param bot The bot the msg came from.
 * \param cmd "zapf" or "z".
 * \param next The parameters.
 *
 * \format \b zapf from_bot to_bot command [parameters]
 */

static int got_botmsg(botnet_bot_t *bot, const char *cmd, const char *next)
{
	int n;
	char *word[3];
	botnet_bot_t *src, *dst;

	n = egg_get_word_array(next, &next, word, 3);
	if (n != 3) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	src = botnet_lookup(word[0]);
	if (!src || botnet_check_direction(bot, src)) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	dst = botnet_lookup(word[1]);
	if (!dst && strcmp(word[1], botnet_get_name())) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	while (isspace(*next)) next++;
	if (!dst && !strcmp(word[2], "assoc") && next && *next) got_assoc(src, next);
	botnet_botmsg(src, dst, word[2], next, -1);
	egg_free_word_array(word, 3);
	return BIND_RET_BREAK;
}

static int got_link(botnet_bot_t *bot, const char *cmd, const char *next)
{
	int n;
	char *word[3];
	botnet_bot_t *dst;
	botnet_entity_t src;

	n = egg_get_word_array(next, &next, word, 3);
	if (n != 3) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	n = get_entity(bot, &src, word[0]);
	dst = botnet_lookup(word[1]);
	if (n < 0 || botnet_check_direction(bot, src.what == ENTITY_BOT ? src.bot : src.user->bot) || (!dst && strcmp(word[1], botnet_get_name()))) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	botnet_link(&src, dst, word[2]);

	return BIND_RET_BREAK;
}

/*!
 * \brief Handle a message from a bot to the entire net.
 *
 * This is the old "zapf-broad" message, whatever that may mean. It's called
 * botbroadcast in this version. It's some text from a script on a bot for
 * another script all other bots. If there's no script listening for the
 * message on any given bot it'll be ignored (but still passed on).
 *
 * \param bot The bot the msg came from.
 * \param cmd "zapf" or "z".
 * \param next The parameters.
 *
 * \format \b zapf from_bot command [parameters]
 */

static int got_botbroadcast(botnet_bot_t *bot, const char *cmd, const char *next)
{
	int n;
	char *word[2];
	botnet_bot_t *src;

	n = egg_get_word_array(next, &next, word, 2);
	if (n != 2) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	src = botnet_lookup(word[0]);
	if (botnet_check_direction(bot, src)) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	while (isspace(*next)) next++;
	botnet_botbroadcast(src, word[1], next, -1);
	egg_free_word_array(word, 2);
	return BIND_RET_BREAK;
}

static int got_who(botnet_bot_t *bot, const char *cmd, const char *next)
{
	int n;
	char *word[2];
	botnet_entity_t src;
	botnet_bot_t *dst;

	n = egg_get_word_array(next, &next, word, 2);
	if (n != 2) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	dst = botnet_lookup(word[1]);
	if (!dst && strcmp(word[1], botnet_get_name())) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	n = get_entity(bot, &src, word[0]);
	if (n < 0 || src.what == ENTITY_BOT || botnet_check_direction(bot, src.user->bot)) {
		egg_free_word_array(word, 2);
		return BIND_RET_BREAK;
	}

	while (isspace(*next)) next++;
	botnet_extension(EXTENSION_ONE, &src, dst, NULL, "who", next, -1);
	egg_free_word_array(word, 2);
	return BIND_RET_BREAK;
}

static int oldbotnet_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	oldbotnet_t *data = client_data;

	egg_iprintf(idx, "%s\n", data->name);

	return 0;
}

static int oldbotnet_on_read(void *client_data, int idx, char *data, int len)
{
	char *cmd;
	const char *next;
	oldbotnet_t *bot = client_data;

	bot->idle = 0;
	egg_get_word(data, &next, &cmd);
	if (!cmd) return(0);

	if (next) while (isspace(*next)) next++;

	if (!strcasecmp(cmd, "e") || !strcasecmp(cmd, "error") || (!bot->bot && !strcasecmp(cmd, "bye"))) {
		char buf[512];
		snprintf(buf, sizeof(buf), _("Botnet error from %s: %s"), bot->user->handle, next);
		if (bot->bot) {
			botnet_delete(bot->bot, buf);
		} else {
			botnet_link_failed(bot->user, buf);
			bot->user = NULL;
			sockbuf_delete(bot->idx);
		}
		free(cmd);
		return 0;
	}

	if (!bot->bot) {
		if (!strcasecmp(cmd, "passreq")) {
			got_passreq(bot, next);
		} else if (!strcasecmp(cmd, "badpass")) {
			botnet_link_failed(bot->user, _("Botnet error: Password was rejected."));
			bot->user = NULL;
			sockbuf_delete(bot->idx);
		} else if (!strcasecmp(cmd, "version") || !strcasecmp(cmd, "v")) {
			got_version(bot, next);
		}
		free(cmd);
		return 0;
	}

	if (!bind_check(BT_obot, NULL, cmd, bot->bot, cmd, next)) {
		putlog(LOG_MISC, "*", "%s said \"%s\" but noone cared :( (parameters: %s)", bot->bot->name, cmd, next);
	}

	free(cmd);
	return(0);
}

static int oldbotnet_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	oldbotnet_t *bot = client_data;

	if (!bot->bot) {
		if (bot->user->flags & USER_LINKING_BOT) botnet_link_failed(bot->user, errmsg ? errmsg : "no error");
		bot->user = NULL;   /* Might already be in the process of being reconnected, forget about it. */
		sockbuf_delete(idx);
	} else {
		putlog(LOG_MISC, "*", _("eof from %s (%s)."), bot->bot->name, errmsg ? errmsg : "no error");
		botnet_delete(bot->bot, errmsg ? errmsg : "eof");
	}
	return 0;
}

static int bot_on_delete(event_owner_t *owner, void *client_data)
{
	oldbotnet_t *bot = client_data;

	bot->bot = NULL;
	if (bot->idx >= 0) sockbuf_delete(bot->idx);

	return 0;
}

static int anonymous_on_delete(event_owner_t *owner, void *client_data)
{
	oldbotnet_t *bot = client_data;

	bot->anonymous = NULL;

	return 0;
}

static int oldbotnet_on_delete(void *client_data, int idx)
{
	oldbotnet_t *bot = client_data;

	bot->idx = -1;
	if (bot->bot) botnet_delete(bot->bot, _("Socket deleted."));
	else if (bot->user && bot->user->flags & USER_LINKING_BOT) botnet_link_failed(bot->user, _("Socket deleted."));

	if (bot->name) free(bot->name);
	if (bot->password) free(bot->password);
	free(bot);

	return 0;
}

static int oldbotnet_init()
{
	BT_obot = bind_table_add("obot", 3, "Bss", MATCH_MASK, BIND_STACKABLE);
	bind_add_list("obot", obot_binds);
	bind_add_list("party", party_binds);
	bind_add_simple(BTN_BOTNET_REQUEST_LINK, NULL, "old-eggdrop", do_link);
	return 0;
}

static int oldbotnet_close(int why)
{
	assoc_t *a, *next;

	for (a = assocs.next; a; a = next) {
		next = a->next;
		free(a->name);
		free(a);
	} 
	assocs.next = NULL;

	bind_table_del(BT_obot);
	bind_rem_list("obot", obot_binds);
	bind_rem_list("party", party_binds);
	bind_rem_simple(BTN_BOTNET_REQUEST_LINK, NULL, "old-eggdrop", do_link);

	return 0;
}

int oldbotnet_LTX_start(egg_module_t *modinfo)
{
	bot_owner.module = anonymous_owner.module = generic_owner.module = modinfo;
	modinfo->name = "oldbotnet";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "oldbotnet support for most things excluding shares";
	modinfo->close_func = oldbotnet_close;
	
	oldbotnet_init();

	return(0);
}
