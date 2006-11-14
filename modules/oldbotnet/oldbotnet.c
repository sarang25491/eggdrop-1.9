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
static const char rcsid[] = "$Id: oldbotnet.c,v 1.13 2006/11/14 14:51:24 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "oldbotnet.h"

EXPORT_SCOPE int oldbotnet_LTX_start(egg_module_t *modinfo);
static int oldbotnet_close(int why);
static int do_link(user_t *u, const char *text);
static int bot_on_delete(event_owner_t *owner, void *client_data);
//static int sock_on_delete(event_owner_t *owner, void *client_data);

/* Partyline commands. */
static int party_plus_obot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text);
static int party_minus_obot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text);

/* Oldbotnet commands. */
static int got_handshake(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_ping(botnet_bot_t *bot, char *cmd, char *next);
static int got_join(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_part(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_chat(botnet_bot_t *bot, char *cmd, const char *next);
static int got_nlinked(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_unlinked(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_bye(botnet_bot_t *bot, const char *cmd, const char *text);
static int got_endlink(botnet_bot_t *bot, const char *cmd, const char *next);
static int got_thisbot(botnet_bot_t *bot, const char *cmd, const char *next);

/* Sockbuf handler. */
static int oldbotnet_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int oldbotnet_on_read(void *client_data, int idx, char *data, int len);
static int oldbotnet_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int oldbotnet_on_delete(void *client_data, int idx);

/* botnet callbacks */
static int on_quit(void *client_data, partymember_t *src, const char *text, int len);
static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking);
static int on_new_bot(void *client_data, botnet_bot_t *bot, int linking);
static int on_lost_bot(void *client_data, botnet_bot_t *bot, const char *reason);
static int on_unlink(void *client_data, partymember_t *from, botnet_bot_t *bot, const char *reason);

static event_owner_t bot_owner = {
	"oldbotnet", NULL,
	NULL, NULL,
	bot_on_delete
};

/*static event_owner_t sock_owner = {
	"oldbotnet", NULL,
	NULL, NULL,
	sock_on_delete
};*/

static botnet_handler_t handler = {
	NULL, on_quit,
	on_chanmsg, on_join, NULL,
	on_new_bot, on_lost_bot, on_unlink, NULL
};

typedef struct assoc {
	char *name;
	int id;
	struct assoc *next;
} assoc_t;

assoc_t assocs = {
	"*",
	0,
	NULL
};

static bind_list_t party_binds[] = {
	{"n", "+obot", party_plus_obot},	/* DDD	*/
	{"n", "-obot", party_minus_obot},	/* DDD	*/
	{0}
};

static bind_list_t obot_binds[] = {
	{NULL, "handshake", got_handshake},   /* DDD */
	{NULL, "h", got_handshake},           /* DDD */
	{NULL, "ping", got_ping},             /* DDD */
	{NULL, "pi", got_ping},               /* DDD */
	{NULL, "join", got_join},             /* DDD */
	{NULL, "j", got_join},                /* DDD */
	{NULL, "chat", got_chat},             /* DDD */
	{NULL, "c", got_chat},                /* DDD */
	{NULL, "n", got_nlinked},             /* DDD */
	{NULL, "nlinked", got_nlinked},       /* DDD */
	{NULL, "el", got_endlink},
	{NULL, "un", got_unlinked},
	{NULL, "unlinked", got_unlinked},
	{NULL, "bye", got_bye},
	{NULL, "tb", got_thisbot},
	{NULL, "thisbot", got_thisbot},
	{NULL, "part", got_part},
	{NULL, "pt", got_part},
	{0}
};

static partyline_event_t oldbotnet_party = {
	NULL, NULL, NULL,
	NULL, NULL, NULL
};

static sockbuf_handler_t oldbotnet_handler = {
	"oldbotnet",
	oldbotnet_on_connect, oldbotnet_on_eof, NULL,
	oldbotnet_on_read, NULL,
	oldbotnet_on_delete
};

static bind_table_t *BT_obot = NULL;

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
	a->next = assocs.next;
	assocs.next = a;
	return a->id;
}

const char *assoc_get_name(int id)
{
	assoc_t *a;

	for (a = &assocs; a; a = a->next) {
		if (a->id == id) break;
	}
	if (a) return a->name;
	a = malloc(sizeof(*a));
	a->name = egg_mprintf("%d", id);
	a->id = id;
	a->next = assocs.next;
	assocs.next = a;
	return a->name;
}

int oldbotnet_init()
{
	BT_obot = bind_table_add("obot", 3, "Bss", MATCH_MASK, BIND_STACKABLE);	/* DDD */
	bind_add_list("obot", obot_binds);
	bind_add_list("party", party_binds);
	oldbotnet_events_init();
	bind_add_simple(BTN_BOTNET_REQUEST_LINK, NULL, "old-eggdrop", do_link);
	return(0);
}

static int tobase64[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '[', ']'
};

static char base64to[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,


	0, 0, 0, 0, 0, 0, 0, 0, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0,
	
	0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 62, 0, 63, 0, 0, 0, 26, 27, 28,
	
	29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
	
	49, 50, 51, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*!
 * \brief Calculate the base64 value of an int.
 *
 * Uses the eggdrop1.6 style base64 algotithm to convert an int to a
 * base64 string.
 *
 * \param i The number to convert.
 *
 * \return A string containing the base64 value of \e i.
 *
 * \warning This function returns a pointer to a static buffer. Calling it
 *          again will overwrite it. \b Always \b remember!
 */

static const char *itob(int i)
{
	char *pos;
	static char ret[12];

	pos = ret + 11;
	*pos = 0;

	do {
		--pos;
		*pos = tobase64[i & 0x3F];
		i >>= 6;
	} while (i);

	return pos;
}

static int btoi(const char *b)
{
	int i = 0;

	while (*b) i = (i << 6) + base64to[(unsigned char) *b++];
	return(i);
}

static int on_quit(void *client_data, partymember_t *src, const char *text, int len)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "pt %s %s %s\n", src->bot ? src->bot->name : botnet_get_name(), src->nick, itob(src->id));

	return 0;
}

static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "c %s %s %s\n", src->full_name, itob(assoc_get_id(chan->name)), text);

	return 0;
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src, int linking)
{
	char *cid;
	oldbotnet_t *obot = client_data;

	cid = strdup(itob(assoc_get_id(chan->name)));
	egg_iprintf(obot->idx, "j %s%s %s %s %c%s %s@%s\n", linking ? "!" : "", src->bot ? src->bot->name : botnet_get_name(), src->nick, cid, '*', itob(src->id), src->ident, src->host);

	return 0;
}

static int on_new_bot(void *client_data, botnet_bot_t *bot, int linking)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "n %s %s %cEDNI\n", bot->name, bot->uplink ? bot->uplink->name : botnet_get_name(), linking ? '-' : '!');

	return 0;
}

static int on_lost_bot(void *client_data, botnet_bot_t *bot, const char *reason)
{
	oldbotnet_t *obot = client_data;

	egg_iprintf(obot->idx, "un %s %s\n", bot->name, reason);

	return 0;
}

static int on_unlink(void *client_data, partymember_t *from, struct botnet_bot *bot, const char *reason)
{
	oldbotnet_t *obot = client_data;

	if (from->nick) egg_iprintf(obot->idx, "ul %s %s %s %s\n", from->full_name, bot->uplink->name, bot->name, reason);
	else egg_iprintf(obot->idx, "ul %s %s %s %s\n", from->bot ? from->bot->name : botnet_get_name(), bot->uplink->name, bot->name, reason); 

	return 0;
}

/* +obot <obot> <host> <port> [username] [password] */
static int party_plus_obot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text)
{
	char *name, *host, *port, *username, *password;
	user_t *obot;
	int freeusername = 1;

	egg_get_words(text, NULL, &name, &host, &port, &username, &password, NULL);
	if (!port) {
		partymember_printf(p, _("Syntax: +obot <obot> <host> <port> [username] [password]"));
		goto done;
	}

	if (!username) {
		/* No username, so look up the botname. */
		void *config_root = config_get_root("eggdrop");
		config_get_str(&username, config_root, "eggdrop.botname");
		if (!username) {
			partymember_printf(p, _("Could not get local botname; please specify a username to log in as."));
			goto done;
		}
		partymember_printf(p, _("No username specified, using '%s' by default."), username);
		freeusername = 0;
	}

	obot = user_new(name);
	if (!obot) {
		partymember_printf(p, _("Could not create obot '%s'."), name);
		goto done;
	}

	user_set_setting(obot, "oldbotnet", "host", host);
	user_set_setting(obot, "oldbotnet", "port", port);
	user_set_setting(obot, "oldbotnet", "username", username);
	if (password) user_set_setting(obot, "oldbotnet", "password", password);

done:
	if (name) free(name);
	if (host) free(host);
	if (port) free(port);
	if (username && freeusername) free(username);
	if (password) free(password);
	return(0);
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
	char *host = NULL, *port = NULL, *username = NULL, *password = NULL;
	oldbotnet_t *data;

	user_get_setting(user, NULL, "botnet.host", &host);
	user_get_setting(user, NULL, "botnet.port", &port);
	user_get_setting(user, NULL, "botnet.username", &username);
	user_get_setting(user, NULL, "botnet.password", &password);
	if (!host || !port || !username) {
		putlog(LOG_MISC, "*", _("Error linking %s: Invalid telnet address:port stored."), user->handle);
		return BIND_RET_BREAK;
	}

	data = malloc(sizeof(*data));
	data->bot = NULL;
	data->user = user;
	data->host = strdup(host);
	data->port = atoi(port);
	data->name = strdup(username);
	if (password) data->password = strdup(password);
	else data->password = NULL;
	data->idx = egg_connect(data->host, data->port, -1);
	data->connected = 0;

	sockbuf_set_handler(data->idx, &oldbotnet_handler, data);
	linemode_on(data->idx);

	putlog(LOG_MISC, "*", _("Linking to %s (%s %d) on idx %d as %s."), user->handle, data->host, data->port, data->idx, data->name);
	return BIND_RET_BREAK;
}

static void got_passreq(oldbotnet_t *bot, const char *next)
{
	if (!bot->password) {
		putlog(LOG_MISC, "*", _("oldbotnet error: password on %s needs to be reset."), bot->user->handle);
		sockbuf_delete(bot->idx);
		return;
	}

	if (next && *next == '<') {
		MD5_CTX md5;
		char hash[16], hex[64];
		int i;

		MD5_Init(&md5);
		MD5_Update(&md5, next, strlen(next));
		MD5_Update(&md5, bot->password, strlen(bot->password));
		MD5_Final(hash, &md5);
		for (i = 0; i < 16; i++) sprintf(hex+2*i, "%.2x", (unsigned char) hash[i]);
		hex[33] = 0;
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

static void got_version(oldbotnet_t *bot, const char *next)
{
	/* Get their version and handlen. */
	sscanf(next, "%d %d", &bot->version, &bot->handlen);

	/* Send back our version reply with their handlen. */
	egg_iprintf(bot->idx, "version 1090000 %d eggdrop v1.9 <alrighty.then>\n", bot->handlen);
	egg_iprintf(bot->idx, "tb %s\n", bot->name);

	/* And now we're connected. */
	bot->connected = 1;
	bot->linking = 1;
	bot->bot = botnet_new(bot->user->handle, bot->user, NULL, NULL, &handler, bot, &bot_owner, 0);
	botnet_replay_net(bot->bot);
	egg_iprintf(bot->idx, "el\n");
}

static int got_ping(botnet_bot_t *bot, char *cmd, char *next)
{
	oldbotnet_t *obot = bot->client_data;

	egg_iprintf(obot->idx, "po\n");
	return BIND_RET_BREAK;
}

static int got_endlink(botnet_bot_t *bot, const char *cmd, const char *next)
{
	oldbotnet_t *obot = bot->client_data;

	obot->linking = 0;
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

static int got_nlinked(botnet_bot_t *bot, const char *cmd, const char *next)
{
	char *word[3];
	int n, linking = 1;
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

	if (word[2][0] == '!') linking = 0;
	new = botnet_new(word[0], NULL, src, bot, NULL, NULL, NULL, linking);
	if (!new) {
		/* Invalid botname ... should probably do some really clever name mangleing here ... */
		char obuf[512];
		snprintf(obuf, sizeof(obuf), "Botname incompatiblity: %s linked from %s", word[0], src->name);
		botnet_delete(bot, obuf);
		/* or just be lazy and kill the bot -_- */
	}
	return BIND_RET_BREAK;
}

static int got_bye(botnet_bot_t *bot, const char *cmd, const char *text)
{
	botnet_delete(bot, text);
	return BIND_RET_BREAK;
}

static int got_unlinked(botnet_bot_t *bot, const char *cmd, const char *text)
{
	char *lostname;
	botnet_bot_t *lost;

	if (egg_get_word(text, &text, &lostname)) return BIND_RET_BREAK;

	lost = botnet_lookup(lostname);
	if (botnet_check_direction(bot, lost)) return BIND_RET_BREAK;

	while (isspace(*text)) ++text;
	botnet_delete(lost, text);
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

	id = btoi(word[3] + 1);

	p = partymember_lookup(NULL, frombot, id);
	if (p && strcmp(p->nick, word[1])) {
		partymember_delete(p, NULL, "Botnet desync");
		p = NULL;
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

		p = partymember_new(id, NULL, frombot, word[1], ident, host, &oldbotnet_party, NULL);
	}
	partychan_join_name(assoc_get_name(btoi(word[2])), p, linking);
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
	if (next && *next) reason = next;
	else reason = "No reason.";

	src = botnet_lookup(word[0]);
	id = btoi(word[2]);
	if (botnet_check_direction(bot, src)) {
		egg_free_word_array(word, 3);
		return BIND_RET_BREAK;
	}

	p = partymember_lookup(word[1], src, id);
	if (p) partymember_delete(p, NULL, reason);
	egg_free_word_array(word, 3);
	return BIND_RET_BREAK;
}

static int got_chat(botnet_bot_t *bot, char *cmd, const char *next)
{
	char *word[2];
	partymember_t *p;
	int n;

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

	while (isspace(*next)) next++;
	partychan_msg_name(assoc_get_name(btoi(word[1])), p, next, strlen(next));

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

	egg_get_word(data, &next, &cmd);
	if (!cmd) return(0);

	if (next) while (isspace(*next)) next++;

	if (!strcasecmp(cmd, "error") || (!bot->connected && !strcasecmp(cmd, "bye"))) {
		putlog(LOG_MISC, "*", _("Botnet error from %s: %s"), bot->user->handle, next);
		if (bot->connected) botnet_delete(bot->bot, next);
		else sockbuf_delete(bot->idx);
		free(cmd);
		return 0;
	}

	if (!bot->connected) {
		if (!strcasecmp(cmd, "passreq")) {
			got_passreq(bot, next);
		} else if (!strcasecmp(cmd, "badpass")) {
			putlog(LOG_MISC, "*", _("Botnet error: Password was rejected."));
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

	if (!bot->connected) {
		putlog(LOG_MISC, "*", _("eof while trying to link %s (%s)."), bot->user->handle, errmsg ? errmsg : "no error");
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

	sockbuf_delete(bot->idx);

	return 0;
}

static int oldbotnet_on_delete(void *client_data, int idx)
{
	oldbotnet_t *bot = client_data;

	if (bot->host) free(bot->host);
	if (bot->name) free(bot->name);
	if (bot->password) free(bot->password);
	free(bot);
	return 0;
}

static int oldbotnet_close(int why)
{
	return(0);
}

int oldbotnet_LTX_start(egg_module_t *modinfo)
{
	bot_owner.module = /*sock_owner.module = */modinfo;
	modinfo->name = "oldbotnet";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "oldbotnet chat support for the partyline";
	modinfo->close_func = oldbotnet_close;

	oldbotnet_init();
	return(0);
}
