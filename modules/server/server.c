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

static server_config_t default_config = {
	0,	/* trigger on ignore */
	0,	/* keepnick */
	30,	/* connect_timeout */
	10,	/* cycle delay */
	6667,	/* default port */
	30,	/* ping timeout */
	30,	/* dcc timeout */
	NULL,	/* user */
	NULL,	/* real name */
};

current_server_t current_server = {0};
server_config_t config = {0};

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

char *start(eggdrop_t *eggdrop)
{
	cmd_t *cmd;
	egg = eggdrop;

	/* Let's wait for the new config system before we redo all this mess. */
	memcpy(&config, &default_config, sizeof(config));
	config.user = strdup("user");
	config.realname = strdup("real name");

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
