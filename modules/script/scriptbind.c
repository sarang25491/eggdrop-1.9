#include <eggdrop/eggdrop.h>

/* Prototypes for the commands we create in this file. */
static int script_bind(char *table_name, char *mask, script_callback_t *callback);
static int script_unbind(char *table_name, char *flags, char *mask, char *name);
//static int script_rebind(char *table_name, char *mask, char *command, char *newflags, char *newmask);

script_command_t script_bind_cmds[] = {
	{"", "bind", script_bind, NULL, 3, "ssc", "table mask command", SCRIPT_INTEGER, 0},
	{"", "unbind", script_unbind, NULL, 3, "sss", "table mask command", SCRIPT_INTEGER, 0},
//	{"", "rebind", script_rebind, NULL, 6, "ssssss", "table flags mask command newflags newmask", SCRIPT_INTEGER, 0},
	{0}
};

int script_bind_init()
{
}

static int script_bind(char *table_name, char *mask, script_callback_t *callback)
{
	bind_table_t *table;
	int retval;

	table = bind_table_lookup(table_name);
	if (!table) return(1);

	callback->syntax = strdup(table->syntax);
	retval = bind_entry_add(table, mask, callback->name, BIND_WANTS_CD, callback->callback, callback);
	return(retval);
}

static int script_unbind(char *table_name, char *flags, char *mask, char *name)
{
	bind_table_t *table;
	script_callback_t *callback;
	int retval;

	table = bind_table_lookup(table_name);
	if (!table) return(1);

	retval = bind_entry_del(table, -1, mask, name, &callback);
	if (callback) callback->del(callback);
	return(retval);
}

/*
static int script_rebind(char *table_name, char *flags, char *mask, char *command, char *newflags, char *newmask)
{
	bind_table_t *table;

	table = bind_table_lookup(table_name);
	if (!table) return(-1);
	return bind_entry_modify(table, -1, mask, command, newflags, newmask);
}
*/
