/* scriptlog.c: logging-related scripting commands
 *
 * Copyright (C) 2003, 2004 Eggheads Development Team
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
static const char rcsid[] = "$Id: scriptlog.c,v 1.5 2005/12/01 21:22:11 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include "egg_script_internal.h"

static int script_putlog(char *text)
{
	return putlog(-1, "*", "%s", text);
}

static int script_putloglev(char *level, char *chan, char *text)
{
	return putlog(-1, chan, "%s", text);
}

script_command_t script_log_cmds[] = {
	{"", "putlog", script_putlog, NULL, 1, "s", "text", SCRIPT_INTEGER, 0}, /* DDD */
	{"", "putloglev", script_putloglev, NULL, 3, "sss", "level channel text", SCRIPT_INTEGER, 0}, /* DDD */
	{0}
};
