#include <eggdrop/eggdrop.h>
#include "core_config.h"

/* Possible states of the connection. */
#define STATE_RESOLVE	0
#define STATE_NICKNAME	1
#define STATE_PASSWORD	2
#define STATE_PARTYLINE	3

/* Telnet strings for controlling echo behavior. */
#define TELNET_ECHO	1
#define TELNET_AYT	246
#define TELNET_WILL	251
#define TELNET_WONT	252
#define TELNET_DO	253
#define TELNET_DONT	254
#define TELNET_CMD	255

#define TELNET_ECHO_OFF	"\377\373\001"
#define TELNET_ECHO_ON	"\377\374\001"

#define TFLAG_ECHO	1

/* Flags for sessions. */
#define STEALTH_LOGIN	1

typedef struct {
	/* Who we're connected to. */
	user_t *user;
	char *nick, *ident, *host, *ip;
	int port;
	int idx;
	int pid;

	/* Flags for this connection. */
	int flags;

	/* Connection state we're in. */
	int state, count;
} telnet_session_t;

static int telnet_idx = -1;
static int telnet_port = 0;

static void kill_session(telnet_session_t *session);

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
	telnet_idx = egg_server(core_config.telnet_vhost, core_config.telnet_port, &telnet_port);
	sockbuf_set_handler(telnet_idx, &server_handler, NULL);
	return(0);
}

static void kill_session(telnet_session_t *session)
{
	sockbuf_delete(session->idx);
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
	if (core_config.telnet_stealth) {
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
		partyline_update_info(session->pid, session->ident, session->host);
		return(0);
	}
	if (session->flags & STEALTH_LOGIN) {
		fakehost = msprintf("-telnet!%s@%s", session->ident, session->host);
		if (!user_lookup_by_irchost(fakehost)) {
			kill_session(session);
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
			partyline_on_input(session->pid, data, len);
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
				if (session->count > core_config.telnet_max_retries) {
					kill_session(session);
					break;
				}
				free(session->nick);
				session->nick = NULL;
				sockbuf_write(session->idx, "Please enter your nickname.\r\n", -1);
				session->state = STATE_NICKNAME;
			}
			else {
				session->pid = partyline_connect(session->idx, -1, session->user, session->nick, session->ident ? session->ident : "~telnet", session->host ? session->host : session->ip);
				session->state = STATE_PARTYLINE;
				egg_iprintf(idx, "\r\nWelcome to the telnet partyline interface!\r\n");
				if (session->ident) egg_iprintf(idx, "Your ident is: %s\r\n", session->ident);
				if (session->host) egg_iprintf(idx, "Your hostname is: %s\r\n", session->host);
			}
			break;
	}
	return(0);
}

static int telnet_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	telnet_session_t *session = client_data;

	if (session->state == STATE_PARTYLINE) partyline_disconnect(session->pid, err ? errmsg : NULL);
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
