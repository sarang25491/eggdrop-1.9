/* scriptparty.c: partyline-related scripting commands
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
static const char rcsid[] = "$Id: scriptparty.c,v 1.5 2004/06/23 11:19:52 wingman Exp $";
#endif

#include <stdio.h>
#include <eggdrop/eggdrop.h>

script_command_t script_party_cmds[] = {
	/*{"", "party_write", partyline_write, NULL, 2, "is", "pid text", SCRIPT_INTEGER, 0},*/
	{0}
};
