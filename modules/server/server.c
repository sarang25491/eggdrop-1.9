/*
 * server.c implements the eggdrop module interface
 */

#define MODULE_NAME "server"
#define MAKING_SERVER
#include "lib/eggdrop/module.h"
#include "server.h"
#include "servsock.h"
#include "binds.h"

#define start server_LTX_start

eggdrop_t *egg = NULL;

current_server_t current_server = {0};
server_config_t server_config = {0};

int cycle_delay = 0;

/* From scriptcmds.c */
extern script_linked_var_t server_script_vars[];
extern script_command_t server_script_cmds[];

/* From servsock.c */
extern void connect_to_next_server();

/* From output.c */
extern void dequeue_messages();

/* From input.c */
extern bind_list_t my_new_raw_binds[];

/* From dcc_cmd.c */
extern bind_list_t server_party_commands[];

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

	bind_rem_list("newraw", my_new_raw_binds);

	server_binds_destroy();

	script_delete_commands(server_script_cmds);
	script_unlink_vars(server_script_vars);
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

	{"chantypes", &server_config.chantypes, CONFIG_STRING},
	{"strcmp", &server_config.strcmp, CONFIG_STRING},
	{0}
};

char *start(eggdrop_t *eggdrop)
{
	cmd_t *cmd;
	void *config_root;

	egg = eggdrop;

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
	server_config.chantypes = strdup("#&");
	server_config.strcmp = strdup("rfc1459");

	/* Link our config vars. */
	config_root = config_get_root("eggdrop");
	config_link_table(server_config_vars, config_root, "server", 0, NULL);

	memset(&current_server, 0, sizeof(current_server));
	current_server.idx = -1;
	cycle_delay = 0;

	module_register(MODULE_NAME, server_table, 1, 2);
	if (!module_depend(MODULE_NAME, "eggdrop", 107, 0)) {
		module_undepend(MODULE_NAME);
		return "This module requires eggdrop1.7.0 or later";
	}

	/* Create our bind tables. */
	server_binds_init();

	bind_add_list("newraw", my_new_raw_binds);
	bind_add_list("party", server_party_commands);

	script_create_commands(server_script_cmds);
	script_link_vars(server_script_vars);
	add_hook(HOOK_SECONDLY, (Function) server_secondly);

	return(NULL);
}
