#include <eggdrop/eggdrop.h>
#include "telnetparty.h"

telnet_config_t telnet_config = {0};

static config_var_t telnet_config_vars[] = {
	{"vhost", &telnet_config.vhost, CONFIG_STRING},
	{"port", &telnet_config.port, CONFIG_INT},
	{"stealth", &telnet_config.stealth, CONFIG_INT},
	{"max_retries", &telnet_config.max_retries, CONFIG_INT},

	{0}
};

EXPORT_SCOPE int telnetparty_LTX_start(egg_module_t *modinfo);
static int telnetparty_close(int why);

/* From events.c */
extern partyline_event_t telnet_party_handler;

static int telnet_idx = -1;
static int telnet_port = 0;

static int telnet_on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port);
static int telnet_on_read(void *client_data, int idx, char *data, int len);
static int telnet_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int telnet_filter_read(void *client_data, int idx, char *data, int len);
static int telnet_filter_write(void *client_data, int idx, const char *data, int len);
static int telnet_filter_delete(void *client_data, int idx);

static int ident_result(void *client_data, const char *ip, int port, const char *reply);
static int dns_result(void *client_data, const char *ip, const char *host);
static int process_results(telnet_session_t *session);

static sockbuf_handler_t server_handler = {
	"telnet server",
	NULL, NULL, telnet_on_newclient,
	NULL, NULL
};

#define TELNET_FILTER_LEVEL	SOCKBUF_LEVEL_TEXT_ALTERATION

static sockbuf_filter_t telnet_filter = {
	"telnet filter", TELNET_FILTER_LEVEL,
	NULL, NULL, NULL,
	telnet_filter_read, telnet_filter_write, NULL,
	NULL, telnet_filter_delete
};

static sockbuf_handler_t client_handler = {
	"telnet",
	NULL, telnet_on_eof, NULL,
	telnet_on_read, NULL
};

int telnet_init()
{
	/* Open our listening socket. */
	telnet_idx = egg_server(telnet_config.vhost, telnet_config.port, &telnet_port);
	sockbuf_set_handler(telnet_idx, &server_handler, NULL);
	return(0);
}

static void kill_session(telnet_session_t *session)
{
	if (session->ip) free(session->ip);
	if (session->host) free(session->host);
	if (session->ident) free(session->ident);
	if (session->nick) free(session->nick);
	free(session);
}

static int telnet_on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port)
{
	telnet_session_t *session;
	int *flags;

	session = calloc(1, sizeof(*session));
	session->ip = strdup(peer_ip);
	session->port = peer_port;
	session->idx = newidx;

	sockbuf_set_handler(newidx, &client_handler, session);
	flags = calloc(1, sizeof(*flags));
	sockbuf_attach_filter(newidx, &telnet_filter, flags);
	linemode_on(newidx);

	egg_iprintf(newidx, "Hello %s/%d!\r\n", peer_ip, peer_port);
	/* Stealth logins are where we don't say anything until we know they
	 * are a valid user. */
	if (telnet_config.stealth) {
		session->state = STATE_RESOLVE;
	}
	else {
		sockbuf_write(newidx, "\r\nPlease enter your nickname.\r\n", -1);
		session->state = STATE_NICKNAME;
		session->count = 0;
	}

	/* Start lookups. */
	egg_ident_lookup(peer_ip, peer_port, telnet_port, -1, ident_result, session);
	egg_dns_reverse(peer_ip, -1, dns_result, session);

	return(0);
}

static int ident_result(void *client_data, const char *ip, int port, const char *reply)
{
	telnet_session_t *session = client_data;

	if (reply) session->ident = strdup(reply);
	else session->ident = strdup("~telnet");
	process_results(session);
	return(0);
}

static int dns_result(void *client_data, const char *ip, const char *host)
{
	telnet_session_t *session = client_data;

	if (!host) host = ip;
	session->host = strdup(host);
	process_results(session);
	return(0);
}

static int process_results(telnet_session_t *session)
{
	char *fakehost;

	if (!session->ident || !session->host) return(0);

	if (session->state == STATE_PARTYLINE) {
		partyline_update_info(session->party, session->ident, session->host);
		return(0);
	}
	if (session->flags & STEALTH_LOGIN) {
		fakehost = msprintf("-telnet!%s@%s", session->ident, session->host);
		if (!user_lookup_by_irchost(fakehost)) {
			sockbuf_delete(session->idx);
			return(0);
		}
		sockbuf_write(session->idx, "\r\nPlease enter your nickname.\r\n", -1);
		session->state = STATE_NICKNAME;
	}
	return(0);
}

static int telnet_on_read(void *client_data, int idx, char *data, int len)
{
	telnet_session_t *session = client_data;

	switch (session->state) {
		case STATE_PARTYLINE:
			partyline_on_input(NULL, session->party, data, len);
			break;
		case STATE_NICKNAME:
			session->nick = strdup(data);
			session->state = STATE_PASSWORD;
			sockbuf_write(session->idx, TELNET_ECHO_OFF, -1);
			sockbuf_write(session->idx, "Please enter your password.\r\n", -1);
			break;
		case STATE_PASSWORD:
			sockbuf_write(session->idx, TELNET_ECHO_ON, -1);
			session->user = user_lookup_authed(session->nick, data);
			if (!session->user) {
				sockbuf_write(session->idx, "Invalid username/password.\r\n\r\n", -1);
				session->count++;
				if (session->count > telnet_config.max_retries) {
					sockbuf_delete(session->idx);
					break;
				}
				free(session->nick);
				session->nick = NULL;
				sockbuf_write(session->idx, "Please enter your nickname.\r\n", -1);
				session->state = STATE_NICKNAME;
			}
			else {
				session->party = partymember_new(-1, session->user, session->nick, session->ident ? session->ident : "~telnet", session->host ? session->host : session->ip, &telnet_party_handler, session);
				session->state = STATE_PARTYLINE;
				egg_iprintf(idx, "\r\nWelcome to the telnet partyline interface!\r\n");
				if (session->ident) egg_iprintf(idx, "Your ident is: %s\r\n", session->ident);
				if (session->host) egg_iprintf(idx, "Your hostname is: %s\r\n", session->host);
				/* Put them on the main channel. */
				partychan_join_name("*", session->party);
			}
			break;
	}
	return(0);
}

static int telnet_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	telnet_session_t *session = client_data;

	if (session->party) {
		if (!err) errmsg = "Client disconnected";
		else if (!errmsg) errmsg = "Unknown error";
		partymember_delete(session->party, errmsg);
		session->party = NULL;
	}
	sockbuf_delete(idx);
	return(0);
}

static int telnet_on_delete(void *client_data)
{
	telnet_session_t *session = client_data;

	if (session->party) {
		partymember_delete(session->party, "Deleted!");
		session->party = NULL;
	}
	kill_session(session);
	return(0);
}

static int telnet_filter_read(void *client_data, int idx, char *data, int len)
{
	char *cmd;
	int type, arg, remove, flags;

	flags = *(int *)client_data;
	cmd = data;
	while ((cmd = memchr(cmd, TELNET_CMD, len))) {
		type = *(unsigned char *)(cmd+1);
		if (type == TELNET_CMD) remove = 1;
		else if (type == TELNET_DO) {
			arg = *(cmd+2);
			if (arg == TELNET_ECHO) flags |= TFLAG_ECHO;
			remove = 3;
		}
		else if (type == TELNET_DONT) {
			arg = *(cmd+2);
			if (arg == TELNET_ECHO) flags &= ~TFLAG_ECHO;
			remove = 3;
		}
		else if (type == TELNET_WILL || type == TELNET_WONT) {
			remove = 3;
		}
		else {
			remove = 2;
		}
		memmove(cmd, cmd+remove, len - (cmd - data) - remove + 1);
		len -= remove;
	}
	if (len) {
		if (flags & TFLAG_ECHO) sockbuf_on_write(idx, TELNET_FILTER_LEVEL, data, len);
		sockbuf_on_read(idx, TELNET_FILTER_LEVEL, data, len);
	}
	return(0);
}

/* We replace \n with \r\n and \255 with \255\255. */
static int telnet_filter_write(void *client_data, int idx, const char *data, int len)
{
	const char *newline;
	int left, linelen, r, r2;

	newline = data;
	r = 0;
	left = len;
	while ((newline = memchr(newline, '\n', left))) {
		linelen = newline - data;
		if (linelen > 0  && newline[-1] == '\r') {
			newline++;
			left = len - linelen - 1;
			continue;
		}

		r2 = sockbuf_on_write(idx, TELNET_FILTER_LEVEL, data, linelen);
		if (r2 < 0) return(r2);
		r += r2;
		r2 = sockbuf_on_write(idx, TELNET_FILTER_LEVEL, "\r\n", 2);
		if (r2 < 0) return(r2);
		r += r2;
		data = newline+1;
		newline = data;
		len -= linelen+1;
		left = len;
	}
	if (len > 0) {
		r2 = sockbuf_on_write(idx, TELNET_FILTER_LEVEL, data, len);
		if (r2 < 0) return(r2);
		r += r2;
	}
	return(r);
}

static int telnet_filter_delete(void *client_data, int idx)
{
	free(client_data);
	return(0);
}

static int telnetparty_close(int why)
{
	sockbuf_delete(telnet_idx);
	return(0);
}

int telnetparty_LTX_start(egg_module_t *modinfo)
{
	void *config_root;

	modinfo->name = "telnetparty";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "telnet support for the partyline";
	modinfo->close_func = telnetparty_close;

	/* Set defaults. */
	memset(&telnet_config, 0, sizeof(telnet_config));
	telnet_config.max_retries = 3;

	/* Link our vars in. */
	config_root = config_get_root("eggdrop");
	config_link_table(telnet_config_vars, config_root, "telnetparty", 0, NULL);
	config_update_table(telnet_config_vars, config_root, "telnetparty", 0, NULL);

	telnet_init();
	return(0);
}
