/*
 * script.c
 *   stuff needed for scripting modules
 *
 * $Id: script.c,v 1.3 2002/05/01 02:30:55 stdarg Exp $
 */
/*
 * Copyright (C) 2001, 2002 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h> /* strlen() */
#include <stdlib.h> /* calloc(), malloc() */

#include "lib/egglib/mstack.h"
#include "eggdrop.h"
#include "script.h"

static Function load_script, link_var, unlink_var, create_cmd, delete_cmd;
static void *load_script_h, *link_var_h, *unlink_var_h, *create_cmd_h, *delete_cmd_h;

static mstack_t *script_events;

int script_load(char *fname);
int script_create_cmd_table(script_command_t *table);

static script_command_t my_script_cmds[] = {
	{"", "loadscript", script_load, NULL, 1, "s", "filename", SCRIPT_INTEGER, 0},
	{0}
};

typedef struct {
	int type;
	void *arg1, *arg2, *arg3;
} script_event_t;

static void add_event(int type, void *arg1, void *arg2, void *arg3)
{
	script_event_t *event = (script_event_t *)malloc(sizeof(*event));
	event->type = type;
	event->arg1 = arg1;
	event->arg2 = arg2;
	event->arg3 = arg3;
	mstack_push(script_events, event);
}

static int my_create_cmd(void *ignore, script_command_t *info)
{
	add_event(SCRIPT_EVENT_CREATE_CMD, info, 0, 0);
	return(0);
}

static int my_delete_cmd(void *ignore, script_command_t *info)
{
	add_event(SCRIPT_EVENT_DELETE_CMD, info, 0, 0);
	return(0);
}

static int my_link_var(void *ignore, script_linked_var_t *var)
{
	add_event(SCRIPT_EVENT_LINK_VAR, var, 0, 0);
	return(0);
}

static int my_unlink_var(void *ignore, script_linked_var_t *var)
{
	add_event(SCRIPT_EVENT_UNLINK_VAR, var, 0, 0);
	return(0);
}

static int my_playback(void *ignore, Function *table)
{
	script_event_t *event;
	int i, version, max;

	version = (int) *table;
	if (version != 1) return(0);
	table++;

	max = (int) *table;
	table++;
	for (i = 0; i < script_events->len; i++) {
		event = (script_event_t *)script_events->stack[i];
		if (event->type < max && table[event->type]) {
			(table[event->type])(NULL, event->arg1, event->arg2, event->arg3);
		}
	}
	return(0);
}

static registry_simple_chain_t my_functions[] = {
	{"script", NULL, 0},
	{"create cmd", my_create_cmd, 2},
	{"delete cmd", my_delete_cmd, 2},
	{"link var", my_link_var, 2},
	{"unlink var", my_unlink_var, 2},
	{"playback", my_playback, 2},
	{0}
};

int script_init()
{
	script_events = mstack_new(0);
	registry_add_simple_chains(my_functions);
	registry_lookup("script", "load script", &load_script, &load_script_h);
	registry_lookup("script", "link var", &link_var, &link_var_h);
	registry_lookup("script", "unlink var", &unlink_var, &unlink_var_h);
	registry_lookup("script", "create cmd", &create_cmd, &create_cmd_h);
	registry_lookup("script", "delete cmd", &delete_cmd, &delete_cmd_h);

	script_create_cmd_table(my_script_cmds);

	return(0);
}

int script_load(char *fname)
{
	load_script(load_script_h, fname);
	return(0);
}

int script_link_var_table(script_linked_var_t *table)
{
	while (table->class && table->name) {
		link_var(link_var_h, table);
		table++;
	}
	return(0);
}

int script_unlink_var_table(script_linked_var_t *table)
{
	while (table->class && table->name) {
		unlink_var(unlink_var_h, table);
		table++;
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

int script_create_simple_cmd_table(script_simple_command_t *table)
{
	script_command_t *cmd;
	char *class;

	/* First entry gives the class. */
	class = table->name;
	table++;

	while (table->name) {
		cmd = (script_command_t *)calloc(1, sizeof(*cmd));
		cmd->class = class;
		cmd->name = table->name;
		cmd->callback = table->callback;
		cmd->nargs = strlen(table->syntax);
		cmd->syntax = table->syntax;
		cmd->syntax_error = table->syntax_error;
		cmd->retval_type = table->retval_type;
		create_cmd(create_cmd_h, cmd);
		table++;
	}
	return(0);
}

script_var_t *script_string(char *str, int len)
{
	script_var_t *var = (script_var_t *)malloc(sizeof(*var));
	var->type = SCRIPT_STRING | SCRIPT_FREE_VAR;
	var->value = (void *)str;
	if (len < 0) len = strlen(str);
	var->len = len;
	return(var);
}

script_var_t *script_dynamic_string(char *str, int len)
{	script_var_t *var = script_string(str, len);
	var->type |= SCRIPT_FREE;
	return(var);
}

script_var_t *script_copy_string(char *str, int len)
{
	char *copy;

	if (len < 0) len = strlen(str);
	copy = (char *)malloc(len+1);
	memcpy(copy, str, len);
	copy[len] = 0;
	return script_dynamic_string(copy, len);
}

script_var_t *script_int(int val)
{
	script_var_t *var = (script_var_t *)malloc(sizeof(*var));
	var->type = SCRIPT_INTEGER | SCRIPT_FREE_VAR;
	var->value = (void *)val;
	return(var);
}

script_var_t *script_list(int nitems, ...)
{
	script_var_t *list;

	list = (script_var_t *)malloc(sizeof(*list));
	list->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR | SCRIPT_FREE_VAR;
	list->len = nitems;
	if (nitems > 0) {
		list->value = malloc(nitems * sizeof(script_var_t *));
		memmove(list->value, &nitems + 1, nitems * sizeof(script_var_t *));
	}
	else list->value = NULL;
	return(list);
}

int script_list_append(script_var_t *list, script_var_t *item)
{
	list->value = realloc(list->value, sizeof(item) * (list->len+1));
	((script_var_t **)list->value)[list->len] = item;
	list->len++;
	return(0);
}
