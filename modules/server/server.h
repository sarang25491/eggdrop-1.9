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
	char *chantypes;
	char *strcmp;

	int raw_log;

	int ip_lookup;	/* 0 - normal, 1 - server */
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
	char *nick, *user, *host, *real_name;
	char *pass;

	/* Information about this server. */
	char *chantypes;
	int (*strcmp)(const char *s1, const char *s2);
} current_server_t;

extern server_config_t server_config;
extern current_server_t current_server;

#endif
