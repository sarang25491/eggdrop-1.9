#ifndef _MOD_DCCPARTY_H_
#define _MOD_DCCPARTY_H_

/* Possible states of the connection. */
#define STATE_RESOLVE	0
#define STATE_NICKNAME	1
#define STATE_PASSWORD	2
#define STATE_PARTYLINE	3

/* Flags for sessions. */
#define STEALTH_LOGIN	1

typedef struct {
	/* Pointer to our entry in the partyline. */
	partymember_t *party;

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
} dcc_session_t;

typedef struct {
	char *vhost;
	int port;
	int stealth;
	int max_retries;
} dcc_config_t;

extern dcc_config_t dcc_config;

#endif
