#include "lib/eggdrop/module.h"
#include <eggdrop/eggdrop.h>

#define MODULE_NAME "script"

static eggdrop_t *egg = NULL;
extern script_command_t script_bind_cmds[], script_net_cmds[],
	script_new_user_cmds[], script_party_cmds[], script_timer_cmds[];

static void script_report(int idx, int details)
{
}

EXPORT_SCOPE char *script_LTX_start();
static char *script_close();

static Function script_table[] = {
	(Function) script_LTX_start,
	(Function) script_close,
	(Function) 0,
	(Function) script_report
};

char *script_LTX_start(eggdrop_t *eggdrop)
{
	egg = eggdrop;

	module_register("script", script_table, 1, 2);
	if (!module_depend("script", "eggdrop", 107, 0)) {
		module_undepend("script");
		return "This module requires eggdrop1.7.0 or later";
	}

	script_create_commands(script_bind_cmds);
	script_create_commands(script_net_cmds);
	script_create_commands(script_new_user_cmds);
	script_create_commands(script_party_cmds);
	script_create_commands(script_timer_cmds);

	return(NULL);
}

static char *script_close()
{
	module_undepend("script");
	return(NULL);
}
