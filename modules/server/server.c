/*
 * server.c implements the eggdrop module interface
 */

#define MODULE_NAME "server"
#define MAKING_SERVER
#include "lib/eggdrop/module.h"
#include "server.h"
#include "servsock.h"
#include "serverlist.h"
#include "nicklist.h"
#include "binds.h"
#include "channels.h"

#define start server_LTX_start

eggdrop_t *egg = NULL;

current_server_t current_server = {0};
server_config_t server_config = {0};

int cycle_delay = 0;

/* From scriptcmds.c */
extern int server_script_init();
extern int server_script_destroy();

/* From servsock.c */
extern void connect_to_next_server();

/* From output.c */
extern void dequeue_messages();

/* From input.c */
extern bind_list_t server_raw_binds[];

/* From dcc_cmd.c */
extern bind_list_t server_party_commands[];

/* Look up the information we get from 005. */
char *server_support(const char *name)
{
	int i;

	for (i = 0; i < current_server.nsupport; i++) {
		if (!strcasecmp(name, current_server.support[i].name)) {
			return(current_server.support[i].value);
		}
	}
	return(NULL);
}

/* Every second, we want to
 * 1. See if we're ready to connect to the next server (cycle_delay == 0)
 * 2. Dequeue some messages if we're connected. */

static void server_secondly()
{
	if (current_server.idx < 0 && cycle_delay >= 0) {
		/* If there's no idx, see if it's time to jump to the next
		 * server. */
		cycle_delay--;
		if (cycle_delay <= 0) {
			cycle_delay = -1;
			connect_to_next_server();
		}
		return;
	}

	/* Try to dequeue some stuff. */
	dequeue_messages();
}

/* A report on the module status.  */
static void server_report(int idx, int details)
{
	if (!current_server.connected) {
		if (current_server.idx >= 0) {
			dprintf(idx, "   Connecting to server %s:%d\n", current_server.server_host, current_server.port);
		}
		else {
			dprintf(idx, "   Connecting to next server in %d seconds\n", cycle_delay);
		}
	}
	else {
		/* First line, who we've connected to. */
		dprintf(idx, "   Connected to %s:%d\n", current_server.server_self ? current_server.server_self : current_server.server_host, current_server.port);

		/* Second line, who we're connected as. */
		if (current_server.registered) {
			if (current_server.user) {
				dprintf(idx, "    Online as %s!%s@%s (%s)\n", current_server.nick, current_server.user, current_server.host, current_server.real_name);
			}
			else {
				dprintf(idx, "    Online as %s (still waiting for WHOIS result)\n", current_server.nick);
			}
		}
		else {
			dprintf(idx, "   Still logging in\n");
		}
	}
}

static char *server_close()
{
	kill_server("Server module unloading");
	cycle_delay = 100;

	bind_rem_list("raw", server_raw_binds);

	server_binds_destroy();

	server_channel_destroy();

	server_script_destroy();

	del_hook(HOOK_SECONDLY, (Function) server_secondly);
	return(NULL);
}

EXPORT_SCOPE char *start();

static Function server_table[] =
{
  (Function) start,
  (Function) server_close,
  (Function) 0,
  (Function) server_report,
};

static config_var_t server_config_vars[] = {
	/* Registration information. */
	{"user", &server_config.user, CONFIG_STRING},
	{"realname", &server_config.realname, CONFIG_STRING},

	/* Timeouts. */
	{"connect_timeout", &server_config.connect_timeout, CONFIG_INT},
	{"ping_timeout", &server_config.ping_timeout, CONFIG_INT},
	{"dcc_timeout", &server_config.dcc_timeout, CONFIG_INT},

	{"trigger_on_ignore", &server_config.trigger_on_ignore, CONFIG_INT},
	{"keepnick", &server_config.keepnick, CONFIG_INT},
	{"cycle_delay", &server_config.cycle_delay, CONFIG_INT},
	{"default_port", &server_config.default_port, CONFIG_INT},

	{"fake005", &server_config.fake005, CONFIG_STRING},

	{"raw_log", &server_config.raw_log, CONFIG_INT},

	{"ip_lookup", &server_config.ip_lookup, CONFIG_INT},
	{0}
};

static void server_config_init()
{
	int i;
	char *host, *pass, *nickstr;
	int port;
	void *config_root, *server, *nick;

	/* Set default values. */
	server_config.user = strdup("user");
	server_config.realname = strdup("real name");
	server_config.connect_timeout = 30;
	server_config.ping_timeout = 30;
	server_config.dcc_timeout = 30;
	server_config.trigger_on_ignore = 0;
	server_config.keepnick = 0;
	server_config.cycle_delay = 10;
	server_config.default_port = 6667;
	server_config.fake005 = NULL;

	/* Link our config vars. */
	config_root = config_get_root("eggdrop");
	config_link_table(server_config_vars, config_root, "server", 0, NULL);
	config_update_table(server_config_vars, config_root, "server", 0, NULL);

	/* Get the server list. */
	for (i = 0; (server = config_exists(config_root, "server", 0, "server", i, NULL)); i++) {
		host = pass = NULL;
		port = 0;
		config_get_str(&host, server, "host", 0, NULL);
		config_get_str(&pass, server, "pass", 0, NULL);
		config_get_int(&port, server, "port", 0, NULL);
		if (host) server_add(host, port, pass);
	}

	/* And the nick list. */
	for (i = 0; (nick = config_exists(config_root, "server", 0, "nick", i, NULL)); i++) {
		nickstr = NULL;
		config_get_str(&nickstr, nick, NULL);
		if (nickstr) nick_add(nickstr);
	}
}

char *start(eggdrop_t *eggdrop)
{
	egg = eggdrop;

	memset(&current_server, 0, sizeof(current_server));
	current_server.idx = -1;
	cycle_delay = 0;

	module_register(MODULE_NAME, server_table, 1, 2);
	if (!module_depend(MODULE_NAME, "eggdrop", 107, 0)) {
		module_undepend(MODULE_NAME);
		return "This module requires eggdrop1.7.0 or later";
	}

	server_config_init();

	/* Create our bind tables. */
	server_binds_init();
	bind_add_list("raw", server_raw_binds);
	bind_add_list("party", server_party_commands);

	/* Initialize channels. */
	server_channel_init();

	/* Initialize script interface. */
	server_script_init();

	add_hook(HOOK_SECONDLY, (Function) server_secondly);

	return(NULL);
}
