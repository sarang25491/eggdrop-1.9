/* scriptbind.c: bind-related scripting commands
 *
 * Copyright (C) 2002, 2003, 2004 Eggheads Development Team
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

#ifndef lint
static const char rcsid[] = "$Id: scriptbind.c,v 1.6 2003/12/18 06:50:47 wcc Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <string.h>
#include <stdlib.h>

/* Prototypes for the commands we create in this file. */
static int script_bind(char *table_name, char *flags, char *mask, script_callback_t *callback);
static int script_unbind(char *table_name, char *mask, char *name);
/*static int script_rebind(char *table_name, char *mask, char *command, char *newflags, char *newmask);*/

typedef struct {
	int bind_id;
	bind_table_t *table;
	script_callback_t *callback;
} fake_bind_placeholder_t;

static int fake_bind_placeholder(void *client_data, ...)
{
	fake_bind_placeholder_t *fake = client_data;
	script_callback_t *callback;
	void *args[11];
	int i;
	va_list ap;

	callback = fake->callback;
	callback->syntax = strdup(fake->table->syntax);
	bind_entry_overwrite(fake->table, fake->bind_id, NULL, NULL, callback->callback, callback);

	args[0] = callback;
	va_start(ap, client_data);
	for (i = 1; i <= fake->table->nargs; i++) {
		args[i] = va_arg(ap, void *);
	}
	va_end(ap);
	free(fake);
	return callback->callback(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10]);
}

static int script_bind(char *table_name, char *flags, char *mask, script_callback_t *callback)
{
	bind_table_t *table;
	int retval;

	table = bind_table_lookup_or_fake(table_name);
	if (!table) return(1);

	if (table->syntax) {
		callback->syntax = strdup(table->syntax);
		retval = bind_entry_add(table, flags, mask, callback->name, BIND_WANTS_CD, callback->callback, callback);
	}
	else {
		fake_bind_placeholder_t *fake;

		fake = calloc(1, sizeof(*fake));
		fake->table = table;
		fake->callback = callback;
		fake->bind_id = bind_entry_add(table, flags, mask, callback->name, BIND_WANTS_CD, (Function) fake_bind_placeholder, fake);
		retval = fake->bind_id;
	}
	return(retval);
}

static int script_unbind(char *table_name, char *mask, char *name)
{
	bind_table_t *table;
	script_callback_t *callback;
	int retval;

	table = bind_table_lookup(table_name);
	if (!table) return(1);

	retval = bind_entry_del(table, -1, mask, name, NULL, &callback);
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

script_command_t script_bind_cmds[] = {
	{"", "bind", script_bind, NULL, 4, "sssc", "table flags mask command", SCRIPT_INTEGER, 0},
	{"", "unbind", script_unbind, NULL, 3, "sss", "table mask command", SCRIPT_INTEGER, 0},
/*	{"", "rebind", script_rebind, NULL, 6, "ssssss", "table flags mask command newflags newmask", SCRIPT_INTEGER, 0},*/
	{0}
};
