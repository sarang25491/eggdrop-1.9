#include <eggdrop/eggdrop.h>

#define MODULE_NAME "script"

extern script_command_t script_bind_cmds[], script_net_cmds[],
	script_new_user_cmds[], script_party_cmds[], script_timer_cmds[],
	script_log_cmds[], script_config_cmds[], script_misc_cmds[];

static int script_module_unload(char *name)
{
	return module_unload(name, EGGMOD_USER);
}

static script_command_t script_mod_cmds[] = {
	{"", "module_load", (Function) module_load, NULL, 1, "s", "name", SCRIPT_INTEGER, 0},
	{"", "module_unload", (Function) script_module_unload, NULL, 1, "s", "name", SCRIPT_INTEGER, 0},
	{"", "module_add_dir", (Function) module_add_dir, NULL, 1, "s", "dir", SCRIPT_INTEGER, 0},
	{0}
};

static void script_report(int idx, int details)
{
}

EXPORT_SCOPE int script_LTX_start(egg_module_t *modinfo);

int script_LTX_start(egg_module_t *modinfo)
{
	modinfo->name = "script";
	modinfo->author = "eggdev";
	modinfo->version = "1.7.0";
	modinfo->description = "provides core scripting functions";

	script_create_commands(script_config_cmds);
	script_create_commands(script_log_cmds);
	script_create_commands(script_bind_cmds);
	script_create_commands(script_net_cmds);
	script_create_commands(script_new_user_cmds);
	script_create_commands(script_party_cmds);
	script_create_commands(script_timer_cmds);
	script_create_commands(script_misc_cmds);
	script_create_commands(script_mod_cmds);

	return(0);
}
