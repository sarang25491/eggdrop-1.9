typedef int (*Function)();
#include "registry.h"
#include "script_api.h"

static Function link_int, unlink_int, link_str, unlink_str, create_cmd, delete_cmd;
void *link_int_h, *unlink_int_h, *link_str_h, *unlink_str_h, *create_cmd_h, *delete_cmd_h;

int script_init()
{
	registry_lookup("script", "link int", &link_int, &link_int_h);
	registry_lookup("script", "unlink int", &unlink_int, &unlink_int_h);
	registry_lookup("script", "link str", &link_str, &link_str_h);
	registry_lookup("script", "unlink str", &unlink_str, &unlink_str_h);
	registry_lookup("script", "create cmd", &create_cmd, &create_cmd_h);
	registry_lookup("script", "delete cmd", &delete_cmd, &delete_cmd_h);
	return(0);
}

int script_link_int_table(script_int_t *table)
{
	script_int_t *intval;

	for (intval = table; intval->class && intval->name; intval++) {
		link_int(link_int_h, intval, 0);
	}
	return(0);

}

int script_unlink_int_table(script_int_t *table)
{
	script_int_t *intval;

	for (intval = table; intval->class && intval->name; intval++) {
		unlink_int(unlink_int_h, intval);
	}
	return(0);

}

int script_link_str_table(script_str_t *table)
{
	script_str_t *str;

	for (str = table; str->class && str->name; str++) {
		link_str(link_str_h, str, 0);
	}
	return(0);

}

int script_unlink_str_table(script_str_t *table)
{
	script_str_t *str;

	for (str = table; str->class && str->name; str++) {
		unlink_str(unlink_str_h, str);
	}
	return(0);

}

int script_create_cmd_table(script_command_t *table)
{
	script_command_t *cmd;

	for (cmd = table; cmd->class && cmd->name; cmd++) {
		create_cmd(create_cmd_h, cmd);
	}
	return(0);
}

int script_delete_cmd_table(script_command_t *table)
{

	script_command_t *cmd;

	for (cmd = table; cmd->class && cmd->name; cmd++) {
		delete_cmd(delete_cmd_h, cmd);
	}
	return(0);
}
