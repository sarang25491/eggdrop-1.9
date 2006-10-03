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
static const char rcsid[] = "$Id: scriptbind.c,v 1.18 2006/10/03 04:02:13 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include "egg_script_internal.h"

/* Prototypes for the commands we create in this file. */
static int script_bind(char *table_name, char *flags, char *mask, script_callback_t *callback);
static int script_unbind(char *table_name, char *mask, char *name);
static int script_rebind(char *table_name, char *flags, char *mask, char *command, char *newflags, char *newmask);

static int script_bind(char *table_name, char *flags, char *mask, script_callback_t *callback)
{
	bind_table_t *table;

	table = bind_table_lookup(table_name);
	if (!table) {
		putlog (LOG_MISC, "*", _("Script '%s' accessed non-existing bind table '%s'."),
				NULL, table_name);
		return(1);
	}

	if (table->syntax) callback->syntax = strdup(table->syntax);
	else callback->syntax = NULL;

	return bind_entry_add(table, flags, mask, callback->name, BIND_WANTS_CD, callback->callback, callback, callback->owner);
}

static int script_unbind(char *table_name, char *mask, char *name)
{
	bind_table_t *table;
	int retval;

	table = bind_table_lookup(table_name);
	if (!table) return(1);

	retval = bind_entry_del(table, mask, name, NULL);
	return(retval);
}

static int script_rebind(char *table_name, char *flags, char *mask, char *command, char *newflags, char *newmask)
{
	bind_table_t *table;

	table = bind_table_lookup(table_name);
	if (!table) return(-1);
	return bind_entry_modify(table, mask, command, newflags, newmask);
}

script_command_t script_bind_cmds[] = {
	{"", "bind", script_bind, NULL, 4, "sssc", "table flags mask command", SCRIPT_INTEGER, 0},	/* DDD	*/
	{"", "unbind", script_unbind, NULL, 3, "sss", "table mask command", SCRIPT_INTEGER, 0},		/* DDD	*/
	{"", "rebind", script_rebind, NULL, 6, "ssssss", "table flags mask command newflags newmask", SCRIPT_INTEGER, 0},
	{0}
};
