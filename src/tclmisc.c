/*
 * tclmisc.c --
 *
 *	Tcl stubs for everything else
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Eggheads Development Team
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
static const char rcsid[] = "$Id: tclmisc.c,v 1.62 2003/02/14 20:55:02 stdarg Exp $";
#endif

#include "main.h"
#include "modules.h"
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <sys/stat.h>
#include "core_binds.h"
#include "logfile.h"
#include "misc.h"
#include "dccutil.h"		/* add_note				*/
#include "net.h"		/* getmyip				*/
#include "users.h"		/* get_user_by_nick, get_user_by_handle	*/
#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

extern struct dcc_t	*dcc;
extern char		 quit_msg[];
extern struct userrec	*userlist;
extern time_t		 now;
extern module_entry	*module_list;
extern int dcc_total;

static unsigned int script_myip()
{
  return iptolong(getmyip());
}

static char *script_myip6()
{
#ifdef IPV6
	struct in6_addr ip;
	char buf[512], *ptr;

	ip = getmyip6();
	ptr = (char *)inet_ntop(AF_INET6, &ip, buf, sizeof(buf));
	if (!ptr) {
		/* Error with conversion -- return "". */
		buf[0] = 0;
	}
	return(strdup(buf));
#else
	/* Empty string if there's no IPv6 support. */
	return(strdup(""));
#endif
}

static int script_sendnote(char *from, char *to, char *msg)
{
  char trunc_from[NOTENAMELEN + 1], trunc_to[NOTENAMELEN + 1], trunc_msg[451];

  strlcpy(trunc_from, from, sizeof(trunc_from));
  strlcpy(trunc_to, to, sizeof(trunc_to));
  strlcpy(trunc_msg, msg, sizeof(trunc_msg));
  return add_note(trunc_to, trunc_from, trunc_msg, -1, 0);
}

static int script_modules(script_var_t *retval)
{
	module_entry *current;
	dependancy *dep;
	script_var_t *name, *version, *entry, *deplist;

	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;

	for (current = module_list; current; current = current->next) {
		name = script_string(current->name, -1);
		version = script_dynamic_string(msprintf("%d.%d", current->major, current->minor), -1);
		deplist = script_list(0);
		for (dep = dependancy_list; dep; dep = dep->next) {
			script_var_t *depname, *depver;

			depname = script_string(dep->needed->name, -1);
			depver = script_copy_string(msprintf("%d.%d", dep->needed->major, dep->needed->minor), -1);

			script_list_append(deplist, script_list(2, depname, depver));
		}
		entry = script_list(3, name, version, deplist);
		script_list_append(retval, entry);
	}
	return(0);
}

static int script_loadmod(char *modname)
{
	module_load(modname);
	return(0);
}

static int script_unloadmod(char *modname)
{
	module_unload(modname, NULL);
	return(0);
}

static int script_loadhelp(char *helpfile)
{
  add_help_reference(helpfile);
  return(0);
}

static int script_unloadhelp(char *helpfile)
{
  rem_help_reference(helpfile);
  return(0);
}

static int script_reloadhelp()
{
  reload_help_data();
  return(0);
}

script_command_t script_misc_cmds[] = {
	{"", "myip", (Function) script_myip, NULL, 0, "", "", SCRIPT_UNSIGNED, 0},
	{"", "myip6", (Function) script_myip6, NULL, 0, "", "", SCRIPT_STRING|SCRIPT_FREE, 0},
	{"", "sendnote", (Function) script_sendnote, NULL, 3, "sss", "from to msg", SCRIPT_INTEGER, 0},
	{"", "modules", (Function) script_modules, NULL, 0, "", "", 0, SCRIPT_PASS_RETVAL},
	{"", "loadmodule", script_loadmod, NULL, 1, "s", "module", SCRIPT_INTEGER, 0},
	{"", "unloadmodule", script_loadmod, NULL, 1, "s", "module", SCRIPT_INTEGER, 0},
	{"", "loadhelp", (Function) script_loadhelp, NULL, 1, "s", "filename", SCRIPT_INTEGER, 0},
	{"", "unloadhelp", (Function) script_unloadhelp, NULL, 1, "s", "filename", SCRIPT_INTEGER, 0},
	{"", "reloadhelp", (Function) script_reloadhelp, NULL, 0, "", "", SCRIPT_INTEGER, 0},
	{0}
};
