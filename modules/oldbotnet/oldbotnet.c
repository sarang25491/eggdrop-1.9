#include <eggdrop/eggdrop.h>
#include <ctype.h>
#include "oldbotnet.h"

EXPORT_SCOPE int oldbotnet_LTX_start(egg_module_t *modinfo);
static int oldbotnet_close(int why);

static int party_plusobot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text);
static int party_olink(partymember_t *p, char *nick, user_t *u, char *cmd, char *text);

static int oldbotnet_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int oldbotnet_on_read(void *client_data, int idx, char *data, int len);
static int oldbotnet_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int oldbotnet_on_delete(void *client_data, int idx);

static bind_list_t party_binds[] = {
	{"n", "+obot", party_plusobot},
	{"n", "olink", party_olink},
	{0}
};

static sockbuf_handler_t oldbotnet_handler = {
	"oldbotnet",
	oldbotnet_on_connect, oldbotnet_on_eof, NULL,
	oldbotnet_on_read, NULL,
	oldbotnet_on_delete
};

oldbotnet_t oldbotnet = {0};

int oldbotnet_init()
{
	bind_add_list("party", party_binds);
	oldbotnet_events_init();
	memset(&oldbotnet, 0, sizeof(oldbotnet));
	return(0);
}

static void kill_session(oldbotnet_session_t *session)
{
	if (session->host) free(session->host);
	if (session->username) free(session->username);
	if (session->password) free(session->password);
	free(session);
}

static int party_plusobot(partymember_t *p, char *nick, user_t *u, char *cmd, char *text)
{
	char *name, *host, *port, *username, *password;
	user_t *obot;
	int freeusername = 1;

	egg_get_words(text, NULL, &name, &host, &port, &username, &password, NULL);
	if (!port) {
		partymember_printf(p, "Syntax: <obot> <host> <port> [username] [password]");
		goto done;
	}

	if (!username) {
		/* No username, so look up the botname. */
		void *config_root = config_get_root("eggdrop");
		config_get_str(&username, config_root, "botname");
		if (!username) {
			partymember_printf(p, "Could not get local botname, please specify a username to log in as.");
			goto done;
		}
		partymember_printf(p, "No username specified, using '%s' by default.", username);
		freeusername = 0;
	}

	obot = user_new(name);
	if (!obot) {
		partymember_printf(p, "Could not create obot '%s'", name);
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
	oldbotnet_session_t *session;
	user_t *obot = NULL;

	if (text) obot = user_lookup_by_handle(text);
	if (!obot) {
		partymember_printf(p, "Syntax: olink <obot>");
		return(0);
	}

	user_get_setting(obot, "oldbotnet", "host", &host);
	user_get_setting(obot, "oldbotnet", "port", &port);
	user_get_setting(obot, "oldbotnet", "username", &username);
	user_get_setting(obot, "oldbotnet", "password", &password);
	if (!host || !port || !username) {
		partymember_printf(p, "Error: '%s' is not an obot", text);
		return(0);
	}
	session = calloc(1, sizeof(*session));
	session->obot = obot;
	session->host = strdup(host);
	session->port = atoi(port);
	session->username = strdup(username);
	if (password) session->password = strdup(password);
	session->idx = egg_connect(host, session->port, -1);

	sockbuf_set_handler(session->idx, &oldbotnet_handler, session);
	linemode_on(session->idx);

	partymember_printf(p, "Linking to %s on idx %d", text, session->idx);
	return(0);
}

static int oldbotnet_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	oldbotnet_session_t *session = client_data;

	egg_iprintf(idx, "%s\n", session->username);
	if (session->password) egg_iprintf(idx, "%s\n", session->password);

	str_redup(&oldbotnet.name, session->username);
	oldbotnet.idx = idx;
	/* Wait to set the 'connected' flag when we get the "el" command. */

	return(0);
}

static int oldbotnet_on_read(void *client_data, int idx, char *data, int len)
{
	oldbotnet_session_t *session = client_data;
	char *cmd;
	const char *next;

	putlog(LOG_MISC, "oldbotnet", "got on %d: %s", idx, data);
	egg_get_word(data, &next, &cmd);
	if (!cmd) return(0);
	if (!strcasecmp(cmd, "pi") || !strcasecmp(cmd, "ping")) {
		putlog(LOG_MISC, "oldbotnet", "sent: pong");
		egg_iprintf(idx, "pong\n");
	}
	else if (!strcasecmp(cmd, "h") || !strcasecmp(cmd, "handshake")) {
		char *pass;

		egg_get_word(next, NULL, &pass);
		putlog(LOG_MISC, "oldbotnet", "Saving password '%s'", pass);
		user_set_setting(session->obot, "oldbotnet", "password", pass);
		free(pass);
	}
	else if (!strcasecmp(cmd, "version")) {
		/* We don't respond on the *hello!, because we want to wait
		 * and see what handlen the other bot has. It is sent in the
		 * version info:
		 * version 1061603 9 eggdrop v1.6.16 <my.network> */
		int numversion, handlen;

		sscanf(next, "%d %d", &numversion, &handlen);
		/* Send back our version reply with their handlen. */
		egg_iprintf(idx, "version 1070000 %d eggdrop v1.7 <alrighty.then>\n", handlen);
		egg_iprintf(idx, "thisbot %s\n", session->username);
	}
	else if (!strcasecmp(cmd, "el")) {
		/* Start processing events! */
		oldbotnet.connected = 1;
	}
	free(cmd);
	return(0);
}

static int oldbotnet_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	oldbotnet_session_t *session = client_data;

	putlog(LOG_MISC, "oldbotnet", "eof from %s:%d (%s).", session->host, session->port, errmsg ? errmsg : "no error");
	sockbuf_delete(idx);
	return(0);
}

static int oldbotnet_on_delete(void *client_data, int idx)
{
	oldbotnet_session_t *session = client_data;

	kill_session(session);
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
