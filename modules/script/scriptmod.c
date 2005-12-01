/* scriptmod.c: module-related scripting commands
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
static const char rcsid[] = "$Id: scriptmod.c,v 1.13 2005/12/01 21:22:11 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include "egg_script_internal.h"

extern script_command_t script_bind_cmds[], script_net_cmds[], script_new_user_cmds[], script_party_cmds[], script_timer_cmds[],
       script_log_cmds[], script_config_cmds[], script_misc_cmds[];

static int script_module_unload(char *name)
{
	return module_unload(name, MODULE_USER);
}

static script_command_t script_mod_cmds[] = {
	{"", "module_load", (Function) module_load, NULL, 1, "s", "name", SCRIPT_INTEGER, 0},	/* DDD */
	{"", "module_unload", (Function) script_module_unload, NULL, 1, "s", "name", SCRIPT_INTEGER, 0},	/* DDD */
	{"", "module_add_dir", (Function) module_add_dir, NULL, 1, "s", "dir", SCRIPT_INTEGER, 0},	/* DDD */
	{"", "module_loaded", (Function) module_loaded, NULL, 1, "s", "name", SCRIPT_INTEGER, 0},	/* DDD */
	{0}
};

EXPORT_SCOPE int script_LTX_start(egg_module_t *modinfo);

int script_LTX_start(egg_module_t *modinfo)
{
	modinfo->name = "script";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
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
