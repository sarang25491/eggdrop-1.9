#ifndef _MOD_TELNET_H_
#define _MOD_TELNET_H_

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
} telnet_session_t;

typedef struct {
	char *vhost;
	int port;
	int stealth;
	int max_retries;
} telnet_config_t;

extern telnet_config_t telnet_config;

#endif
