#include <eggdrop/eggdrop.h>
#include <ctype.h>
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

static int irc_on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port);
static int irc_on_read(void *client_data, int idx, char *data, int len);
static int irc_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int irc_on_delete(void *client_data, int idx);

static int ident_result(void *client_data, const char *ip, int port, const char *reply);
static int dns_result(void *client_data, const char *ip, char **hosts);
static int process_results(irc_session_t *session);

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
		egg_iprintf(newidx, "NOTICE *** Hello %s/%d!\r\n", peer_ip, peer_port);
		sockbuf_write(newidx, "NOTICE *** Don't forget to use the /PASS command to send your password in addition to normal registration!\r\n", -1);
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
	char *fakehost;

	if (!session->ident || !session->host) return(0);

	if (session->state == STATE_PARTYLINE) {
		partyline_update_info(session->party, session->ident, session->host);
		return(0);
	}
	if (session->flags & STEALTH_LOGIN) {
		fakehost = msprintf("-ircparty!%s@%s", session->ident, session->host);
		if (!user_lookup_by_irchost(fakehost)) {
			sockbuf_delete(session->idx);
			return(0);
		}
		sockbuf_write(session->idx, "NOTICE *** Don't forget to use the /PASS command to send your password in addition to normal registration!\r\n", -1);
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
			if (!strcasecmp(msg.cmd, "join")) {
				if (msg.nargs != 1 || strlen(msg.args[0]) < 2) egg_iprintf(session->idx, ":eggdrop.bot 461 %s JOIN :Not enough parameters.\r\n", session->nick);
				else partychan_join_name(msg.args[0]+1, session->party);
			}
			else if (!strcasecmp(msg.cmd, "part")) {
				if (msg.nargs != 1 || strlen(msg.args[0]) < 2) egg_iprintf(session->idx, ":eggdrop.bot 461 %s PART :Not enough parameters.\r\n", session->nick);
				else partychan_part_name(msg.args[0]+1, session->party, msg.args[msg.nargs-1]);
			}
			else if (!strcasecmp(msg.cmd, "privmsg")) {
				if (msg.nargs != 2) egg_iprintf(session->idx, ":eggdrop.bot 461 %s PRIVMSG :Not enough parameters.\r\n", session->nick);
				else partyline_on_input(partychan_lookup_name(msg.args[0]+1), session->party, msg.args[msg.nargs-1], -1);
			}
			else if (!strcasecmp(msg.cmd, "mode")) {
				if (msg.nargs == 1) egg_iprintf(session->idx, ":eggdrop.bot 324 MODE %s +\r\n", msg.args[0]);
			}
			else if (!strcasecmp(msg.cmd, "who")) {
				if (msg.nargs == 1 && strlen(msg.args[0]) > 1) {
					partychan_t *chan = partychan_lookup_name(msg.args[0]+1);
					partychan_member_t *m;
					partymember_t *p;

					if (chan) {
						for (i = 0; i < chan->nmembers; i++) {
							m = chan->members+i;
							if (chan->members[i].flags & PARTY_DELETED) continue;
							p = m->p;
							egg_iprintf(session->idx, ":eggdrop.bot 352 %s %s %s %s eggdrop.bot %s H :0 real name\r\n", session->nick, msg.args[0], p->ident, p->host, p->nick);
							putlog(LOG_MISC, "*", ":eggdrop.bot 352 %s %s %s %s eggdrop.bot %s H :0 real name\r\n", session->nick, msg.args[0], p->ident, p->host, p->nick);
						}
						egg_iprintf(session->idx, ":eggdrop.bot 315 %s %s :End of /WHO list.\r\n", session->nick, msg.args[0]);
					}
				}
			}
			else if (!strcasecmp(msg.cmd, "quit")) {
				sockbuf_delete(session->idx);
			}
			break;
		case STATE_UNREGISTERED:
			if (!strcasecmp(msg.cmd, "nick")) {
				if (session->nick) egg_iprintf(session->idx, ":%s NICK %s\r\n", session->nick, msg.args[0]);
				str_redup(&session->nick, msg.args[0]);
			}
			else if (!strcasecmp(msg.cmd, "pass")) {
				str_redup(&session->pass, msg.args[0]);
			}
			if (session->nick && session->pass) {
				session->user = user_lookup_authed(session->nick, session->pass);
				if (!session->user) {
					sockbuf_write(session->idx, "NOTICE *** Invalid username/password.\r\n", -1);
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
