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
static const char rcsid[] = "$Id: oldbotnet.c,v 1.8 2004/06/20 21:33:28 wingman Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "oldbotnet.h"

EXPORT_SCOPE int oldbotnet_LTX_start(egg_module_t *modinfo);
static int oldbotnet_close(int why);

/* Partyline commands. */
static int party_plusobot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text);
static int party_olink(partymember_t *p, char *nick, user_t *u, char *cmd, char *text);

/* Oldbotnet commands. */
static int got_error(char *cmd, char *next);
static int got_passreq(char *cmd, char *next);
static int got_badpass(char *cmd, char *next);
static int got_handshake(char *cmd, char *next);
static int got_version(char *cmd, char *next);
static int got_ping(char *cmd, char *next);
static int got_join(char *cmd, char *next);
static int got_chat(char *cmd, const char *next);

/* Sockbuf handler. */
static int oldbotnet_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int oldbotnet_on_read(void *client_data, int idx, char *data, int len);
static int oldbotnet_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int oldbotnet_on_delete(void *client_data, int idx);

static bind_list_t party_binds[] = {
	{"n", "+obot", party_plusobot},		/* DDD	*/
	{"n", "olink", party_olink},		/* DDD	*/
	{0}
};

static bind_list_t obot_binds[] = {
	{NULL, "error", got_error},		/* DDD	*/
	{NULL, "passreq", got_passreq},		/* DDD	*/
	{NULL, "badpass", got_badpass},		/* DDD	*/
	{NULL, "handshake", got_handshake},	/* DDD	*/
	{NULL, "h", got_handshake},		/* DDD	*/
	{NULL, "version", got_version},		/* DDD	*/
	{NULL, "v", got_version},		/* DDD	*/
	{NULL, "ping", got_ping},		/* DDD	*/
	{NULL, "pi", got_ping},			/* DDD	*/
	{NULL, "join", got_join},		/* DDD	*/
	{NULL, "j", got_join},			/* DDD	*/
	{NULL, "chat", got_chat},		/* DDD	*/
	{NULL, "c", got_chat},			/* DDD */
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
oldbotnet_t oldbotnet = {0};

int oldbotnet_init()
{
	BT_obot = bind_table_add("obot", 2, "ss", MATCH_MASK, BIND_STACKABLE);	/* DDD */
	bind_add_list("obot", obot_binds);
	bind_add_list("party", party_binds);
	oldbotnet_events_init();
	memset(&oldbotnet, 0, sizeof(oldbotnet));
	oldbotnet.idx = -1;
	return(0);
}

void oldbotnet_reset()
{
	if (oldbotnet.idx != -1) {
		int idx = oldbotnet.idx;
		putlog(LOG_MISC, "oldbotnet", _("*** Disconnecting"));
		oldbotnet.idx = -1;
		sockbuf_delete(idx);
	}
	if (oldbotnet.host) free(oldbotnet.host);
	if (oldbotnet.name) free(oldbotnet.name);
	if (oldbotnet.password) free(oldbotnet.password);
	memset(&oldbotnet, 0, sizeof(oldbotnet));
	oldbotnet.idx = -1;
}

int oldbotnet_bot_id(const char *bot)
{
	return(1);
}

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

static int btoi(const char *b)
{
	int i = 0;

	while (*b) i = (i << 6) + base64to[(unsigned char) *b++];
	return(i);
}

static int party_plusobot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text)
{
	char *name, *host, *port, *username, *password;
	user_t *obot;
	int freeusername = 1;

	egg_get_words(text, NULL, &name, &host, &port, &username, &password, NULL);
	if (!port) {
		partymember_printf(p, _("Syntax: <obot> <host> <port> [username] [password]"));
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

/* Partyline command to link to a 1.6 bot. */
static int party_olink(partymember_t *p, char *nick, user_t *u, char *cmd, char *text)
{
	char *host, *port, *username, *password;
	user_t *obot = NULL;

	if (text) obot = user_lookup_by_handle(text);
	if (!obot) {
		partymember_printf(p, _("Syntax: olink <obot>"));
		return(0);
	}

	user_get_setting(obot, "oldbotnet", "host", &host);
	user_get_setting(obot, "oldbotnet", "port", &port);
	user_get_setting(obot, "oldbotnet", "username", &username);
	user_get_setting(obot, "oldbotnet", "password", &password);
	if (!host || !port || !username) {
		partymember_printf(p, _("Error: '%s' is not an obot."), text);
		return(0);
	}

	oldbotnet_reset();

	oldbotnet.obot = obot;
	oldbotnet.host = strdup(host);
	oldbotnet.port = atoi(port);
	oldbotnet.name = strdup(username);
	if (password) oldbotnet.password = strdup(password);
	oldbotnet.idx = egg_connect(host, oldbotnet.port, -1);

	sockbuf_set_handler(oldbotnet.idx, &oldbotnet_handler, NULL);
	linemode_on(oldbotnet.idx);

	partymember_printf(p, _("Linking to %s (%s %d) on idx %d as %s."), obot->handle, oldbotnet.host, oldbotnet.port, oldbotnet.idx, oldbotnet.name);
	return(0);
}

static int got_error(char *cmd, char *next)
{
	putlog(LOG_MISC, "*", "oldbotnet error: %s", next);
	return(0);
}

static int got_passreq(char *cmd, char *next)
{
	if (oldbotnet.connected) return(0);

	if (!oldbotnet.password) {
		putlog(LOG_MISC, "*", _("oldbotnet error: password on %s needs to be reset."), oldbotnet.obot->handle);
		return(0);
	}

	if (next && *next == '<') {
		MD5_CTX md5;
		char hash[16], hex[64];
		int i;

		putlog(LOG_MISC, "*", _("Calculating md5 of '%s' + '%s'."), next, oldbotnet.password);

		MD5_Init(&md5);
		MD5_Update(&md5, next, strlen(next));
		MD5_Update(&md5, oldbotnet.password, strlen(oldbotnet.password));
		MD5_Final(hash, &md5);
		for (i = 0; i < 16; i++) sprintf(hex+2*i, "%.2x", (unsigned char) hash[i]);
		hex[33] = 0;
		putlog(LOG_MISC, "*", _("Result: '%s'"), hex);
		egg_iprintf(oldbotnet.idx, "digest %s\n", hex);
	}
	else {
		egg_iprintf(oldbotnet.idx, "%s\n", oldbotnet.password);
	}

	return(0);
}

static int got_badpass(char *cmd, char *next)
{
	putlog(LOG_MISC, "*", _("oldbotnet error: password was rejected."));
	return(0);
}

static int got_handshake(char *cmd, char *next)
{
	if (!next) return(0);
	putlog(LOG_MISC, "oldbotnet", "Saving password '%s'", next);
	user_set_setting(oldbotnet.obot, "oldbotnet", "password", next);
	return(0);
}

static int got_version(char *cmd, char *next)
{
	if (oldbotnet.connected || !next) return(0);

	/* Get their version and handlen. */
	sscanf(next, "%d %d", &oldbotnet.oversion, &oldbotnet.handlen);

	/* Send back our version reply with their handlen. */
	egg_iprintf(oldbotnet.idx, "version 1090000 %d eggdrop v1.9 <alrighty.then>\n", oldbotnet.handlen);

	egg_iprintf(oldbotnet.idx, "el\n");

	/* And now we're connected. */
	oldbotnet.connected = 1;
	return(0);
}

static int got_ping(char *cmd, char *next)
{
	egg_iprintf(oldbotnet.idx, "po\n");
	return(0);
}

static int got_join(char *cmd, char *next)
{
	char *word[5];
	partymember_t *p;
	char chan_name[64];
	int n, pid;

	n = egg_get_word_array(next, NULL, word, 5);
	if (n != 5) {
		egg_free_word_array(word, 5);
		return(0);
	}

	/* The pid is a combo of the obot's id and his sock# on the obot. */
	pid = oldbotnet_bot_id(word[0]) * BOT_PID_MULT + btoi(word[3]+1);
	p = partymember_lookup_pid(pid);
	if (p) {
		/* We already know about him. */
		partychan_t *chan;
		chan = partychan_get_default(p);
		partychan_part(chan, p, "joining another channel");
	}
	else {
		/* Nope, he's new. */
		char *ident, *sep, *host, *nick, *bot;
		ident = word[4];
		sep = strchr(ident,'@');
		if (sep) {
			*sep = 0;
			host = sep+1;
		}
		else host = word[0];

		bot = word[0];
		if (*bot == '!') bot++;
		nick = egg_mprintf("%s@%s", word[1], bot);
		p = partymember_new(pid, NULL, nick, ident, host, &oldbotnet_party, NULL);
		free(nick);
	}
	sprintf(chan_name, "%d", btoi(word[2]));
	partychan_join_name(chan_name, p);
	egg_free_word_array(word, 5);
	return(0);
}

static int got_chat(char *cmd, const char *next)
{
	char *word[2];
	partychan_t *chan;
	partymember_t *p;
	int n, pid;

	n = egg_get_word_array(next, &next, word, 2);
	if (n != 2 || !next) {
		egg_free_word_array(word, 2);
		return(0);
	}

	p = partymember_lookup_nick(word[0]);
	if (p) {
		chan = partychan_get_default(p);
		if (next) while (isspace(*next)) next++;
		else next = "";
		if (chan) partychan_msg(chan, p, next, strlen(next));
	}
	egg_free_word_array(word, 2);
	return(0);
}

static int oldbotnet_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	/* Start logging in. */
	egg_iprintf(idx, "%s\n", oldbotnet.name);

	return(0);
}

static int oldbotnet_on_read(void *client_data, int idx, char *data, int len)
{
	char *cmd;
	const char *next;

	putlog(LOG_MISC, "oldbotnet", "got on %d: %s", idx, data);
	egg_get_word(data, &next, &cmd);
	if (!cmd) return(0);

	if (next) while (isspace(*next)) next++;

	bind_check(BT_obot, NULL, cmd, cmd, next);

	free(cmd);
	return(0);
}

static int oldbotnet_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	putlog(LOG_MISC, "oldbotnet", _("eof from %s (%s)."), oldbotnet.obot->handle, errmsg ? errmsg : "no error");
	sockbuf_delete(idx);
	return(0);
}

static int oldbotnet_on_delete(void *client_data, int idx)
{
	if (oldbotnet.idx != -1) {
		oldbotnet.idx = -1;
		oldbotnet_reset();
	}
	return(0);
}

static int oldbotnet_close(int why)
{
	return(0);
}

int oldbotnet_LTX_start(egg_module_t *modinfo)
{
	modinfo->name = "oldbotnet";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "oldbotnet chat support for the partyline";
	modinfo->close_func = oldbotnet_close;

	oldbotnet_init();
	return(0);
}
