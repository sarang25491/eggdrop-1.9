/* scriptdns.c: dns-related scripting commands
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
static const char rcsid[] = "$Id: scriptdns.c,v 1.3 2008/10/17 15:57:43 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include "egg_script_internal.h"

static int script_dns_lookup(const char *host, script_callback_t *callback, char *text, int len)
{
	return script_dns_query(egg_dns_lookup, host, callback, text, len);
}

static int script_dns_reverse(const char *ip, script_callback_t *callback, char *text, int len)
{
	return script_dns_query(egg_dns_reverse, ip, callback, text, len);
}

script_command_t script_dns_cmds[] = {
	{"dns", "lookup", script_dns_lookup, NULL, 2, "scsi", "host callback ?callbackdata? ?len?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},	/* DDD */
	{"dns", "reverse", script_dns_reverse, NULL, 2, "scsi", "ip  callback ?callbackdata? ?len?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},	/* DDD */
	{"dns", "cancel", egg_dns_cancel, NULL, 1, "ii", "id ?docallback?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},	/* DDD */
	{0}
};
