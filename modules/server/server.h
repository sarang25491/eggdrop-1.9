#ifndef _SERVER_H_
#define _SERVER_H_

#define match_my_nick(test) (!((current_server.strcmp)(current_server.nick, test)))

/* Configuration data for this module. */
typedef struct {
	int trigger_on_ignore;
	int keepnick;
	int connect_timeout;
	int cycle_delay;
	int default_port;
	int ping_timeout;
	int dcc_timeout;
	char *user;
	char *realname;

	/* Override the 005 sent by the server. */
	char *fake005;

	int raw_log;

	int ip_lookup;	/* For dcc stuff, 0 - normal, 1 - server */
} server_config_t;

/* All the stuff we need to know about the currently connected server. */
typedef struct {
	/* Connection details. */
	int idx;	/* Sockbuf idx of this connection. */
	char *server_host;	/* The hostname we connected to. */
	char *server_self;	/* What the server calls itself (from 001). */
	int port;	/* The port we connected to. */
	int connected;	/* Are we connected yet? */

	/* Our info on the server. */
	int registered;	/* Has the server accepted our registration? */
	int got005;	/* Did we get the 005 message? */
	char *nick, *user, *host, *real_name;
	char *pass;

	/* Information about this server. */
	struct {
		char *name;
		char *value;
	} *support;
	int nsupport;
	char *chantypes;
	int (*strcmp)(const char *s1, const char *s2);
	char *type1modes, *type2modes, *type3modes, *type4modes;
	char *modeprefix, *whoprefix;

	/* Our dcc information for this server. */
	char *myip;
	unsigned int mylongip;
} current_server_t;

extern server_config_t server_config;
extern current_server_t current_server;

/* And one lonely function, because he had nowhere else to fit. */
extern char *server_support(const char *name);

#endif
