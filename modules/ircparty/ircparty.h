#ifndef _MOD_IRCPARTY_H_
#define _MOD_IRCPARTY_H_

/* Possible states of the connection. */
#define STATE_RESOLVE	0
#define STATE_UNREGISTERED	1
#define STATE_PARTYLINE	2

/* Flags for sessions. */
#define STEALTH_LOGIN	1

typedef struct {
	/* Pointer to our entry in the partyline. */
	partymember_t *party;

	/* Who we're connected to. */
	user_t *user;
	char *nick, *ident, *host, *ip, *pass;
	int port;
	int idx;
	int pid;

	/* Flags for this connection. */
	int flags;

	/* Connection state we're in. */
	int state, count;
} irc_session_t;

typedef struct {
	char *vhost;
	int port;
	int stealth;
} irc_config_t;

extern irc_config_t irc_config;

#endif
