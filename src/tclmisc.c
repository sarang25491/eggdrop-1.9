/*
 * tclmisc.c --
 *
 *	Tcl stubs for everything else
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002 Eggheads Development Team
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
static const char rcsid[] = "$Id: tclmisc.c,v 1.56 2002/09/20 21:41:49 stdarg Exp $";
#endif

#include <sys/stat.h>
#include "main.h"
#include "modules.h"
#include "core_binds.h"
//#include "tandem.h"
#include "md5.h"
#include "logfile.h"
#include "misc.h"
#include "dccutil.h"		/* add_note				*/
#include "net.h"		/* getmyip				*/
#include "users.h"		/* get_user_by_nick, get_user_by_handle	*/
#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

extern struct dcc_t	*dcc;
extern char		 origbotname[], quit_msg[];
extern struct userrec	*userlist;
extern time_t		 now;
extern module_entry	*module_list;
extern int dcc_total;

static char *script_duration(unsigned int sec)
{
  char s[70];
  int tmp;

  s[0] = 0;
  if (sec >= 31536000) {
    tmp = (sec / 31536000);
    sprintf(s, "%d year%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 31536000);
  }
  if (sec >= 604800) {
    tmp = (sec / 604800);
    sprintf(&s[strlen(s)], "%d week%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 604800);
  }
  if (sec >= 86400) {
    tmp = (sec / 86400);
    sprintf(&s[strlen(s)], "%d day%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 86400);
  }
  if (sec >= 3600) {
    tmp = (sec / 3600);
    sprintf(&s[strlen(s)], "%d hour%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 3600);
  }
  if (sec >= 60) {
    tmp = (sec / 60);
    sprintf(&s[strlen(s)], "%d minute%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 60);
  }
  if (sec > 0) {
    tmp = (sec);
    sprintf(&s[strlen(s)], "%d second%s", tmp, (tmp == 1) ? "" : "s");
  }
  if (strlen(s) > 0 && s[strlen(s)-1] == ' ') s[strlen(s)-1] = 0;
  return strdup(s);
}

static unsigned int script_unixtime()
{
	return(now);
}

static char *script_ctime(unsigned int sec)
{
  time_t tt;
  char *str;

  tt = (time_t) sec;

  /* ctime() puts a '\n' on the end of the date string, let's remove it. */
  str = ctime(&tt);
  str[strlen(str)-1] = 0;
  return(str);
}

static char *script_strftime(int nargs, char *format, unsigned int sec)
{
  char buf[512];
  struct tm *tm1;
  time_t t;

  if (nargs > 1) t = sec;
  else t = now;

  tm1 = localtime(&t);
  if (strftime(buf, sizeof(buf), format, tm1)) return strdup(buf);
  return strdup("error with strftime");
}

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

static int script_rand(int nargs, int min, int max)
{
	if (nargs == 1) {
		max = min;
		min = 0;
	}
	if (max <= min) return(max);

	return(random() % (max - min) + min);
}

static int script_sendnote(char *from, char *to, char *msg)
{
  char trunc_from[NOTENAMELEN + 1], trunc_to[NOTENAMELEN + 1], trunc_msg[451];

  strlcpy(trunc_from, from, sizeof(trunc_from));
  strlcpy(trunc_to, to, sizeof(trunc_to));
  strlcpy(trunc_msg, msg, sizeof(trunc_msg));
  return add_note(trunc_to, trunc_from, trunc_msg, -1, 0);
}

static int script_dumpfile(char *nick, char *filename)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  get_user_flagrec(get_user_by_nick(nick), &fr, NULL);
  showhelp(nick, filename, &fr, HELP_TEXT);
  return TCL_OK;
}

static int script_dccdumpfile(int idx, char *filename)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  if (idx < 0 || idx >= dcc_total || !dcc[idx].type) return(1);
  get_user_flagrec(get_user_by_handle(userlist, dcc[idx].nick), &fr, NULL);
  tellhelp(idx, filename, &fr, HELP_TEXT);
  return(0);
}

static int script_backup()
{
  call_hook(HOOK_BACKUP);
  return(0);
}

static int script_die(char *reason)
{
  char s[1024];

  if (reason) {
    snprintf(s, sizeof s, "BOT SHUTDOWN (%s)", reason);
    strlcpy(quit_msg, reason, 1024);
  } else {
    strlcpy(s, "BOT SHUTDOWN (No reason)", sizeof s);
    quit_msg[0] = 0;
  }
  kill_bot(s, quit_msg[0] ? quit_msg : "EXIT");
  return TCL_OK;
}

static const char *script_loadmodule(char *modname)
{
  const char *p;

  p = module_load(modname);
  return(p);
}

static const char *script_unloadmodule(char *modname)
{
  return module_unload(modname, origbotname);
}

static char *script_unames()
{
  char *unix_n, *vers_n, *retval;
#ifdef HAVE_UNAME
  struct utsname un;

  if (uname(&un) < 0) {
#endif
    unix_n = "*unknown*";
    vers_n = "";
#ifdef HAVE_UNAME
  } else {
    unix_n = un.sysname;
    vers_n = un.release;
  }
#endif
  retval = msprintf("%s %s", unix_n, vers_n);
  return(retval);
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

static int script_callevent(char *event)
{
  check_bind_event(event);
  return(0);
}

static char *script_md5(char *data)
{
  MD5_CTX md5context;
  char *digest_string;
  unsigned char digest[16];
  int i;

  MD5_Init(&md5context);
  MD5_Update(&md5context, data, strlen(data));
  MD5_Final(digest, &md5context);
  digest_string = (char *)malloc(33);
  for (i = 0; i < 16; i++) sprintf(digest_string + (i*2), "%.2x", digest[i]);
  digest_string[32] = 0;
  return(digest_string);
}

int script_export(char *name, char *syntax, script_callback_t *callback)
{
	script_command_t new_command = {
		"", strdup(name), callback->callback, callback, strlen(syntax), strdup(syntax), strdup(syntax), SCRIPT_INTEGER, SCRIPT_PASS_CDATA
	};
	script_command_t *copy;

	callback->syntax = strdup(syntax);
	copy = calloc(2, sizeof(*copy));
	memcpy(copy, &new_command, sizeof(new_command));
	script_create_commands(copy);
	return(0);
}

script_command_t script_misc_cmds[] = {
	{"", "duration", (Function) script_duration, NULL, 1, "u", "seconds", SCRIPT_STRING|SCRIPT_FREE, 0},
	{"", "unixtime", (Function) script_unixtime, NULL, 0, "", "", SCRIPT_UNSIGNED, 0},
	{"", "ctime", (Function) script_ctime, NULL, 1, "u", "seconds", SCRIPT_STRING, 0},
	{"", "strftime", (Function) script_strftime, NULL, 1, "su", "format ?seconds?", SCRIPT_STRING|SCRIPT_FREE, SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{"", "myip", (Function) script_myip, NULL, 0, "", "", SCRIPT_UNSIGNED, 0},
	{"", "myip6", (Function) script_myip6, NULL, 0, "", "", SCRIPT_STRING|SCRIPT_FREE, 0},
	{"", "rand", (Function) script_rand, NULL, 1, "ii", "?min? max", SCRIPT_INTEGER, SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{"", "sendnote", (Function) script_sendnote, NULL, 3, "sss", "from to msg", SCRIPT_INTEGER, 0},
	{"", "dumpfile", (Function) script_dumpfile, NULL, 2, "ss", "nick filename", SCRIPT_INTEGER, 0},
	{"", "dccdumpfile", (Function) script_dccdumpfile, NULL, 2, "is", "idx filename", SCRIPT_INTEGER, 0},
	{"", "backup", (Function) script_backup, NULL, 0, "", "", SCRIPT_INTEGER},
	{"", "die", (Function) script_die, NULL, 0, "s", "?reason?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "unames", (Function) script_unames, NULL, 0, "", "", SCRIPT_STRING | SCRIPT_FREE, 0},
	{"", "modules", (Function) script_modules, NULL, 0, "", "", 0, SCRIPT_PASS_RETVAL},
	{"", "loadhelp", (Function) script_loadhelp, NULL, 1, "s", "filename", SCRIPT_INTEGER, 0},
	{"", "unloadhelp", (Function) script_unloadhelp, NULL, 1, "s", "filename", SCRIPT_INTEGER, 0},
	{"", "reloadhelp", (Function) script_reloadhelp, NULL, 0, "", "", SCRIPT_INTEGER, 0},
	{"", "loadmodule", (Function) script_loadmodule, NULL, 1, "s", "module-name", SCRIPT_STRING, 0},
	{"", "unloadmodule", (Function) script_unloadmodule, NULL, 1, "s", "module-name", SCRIPT_STRING, 0},
	{"", "md5", (Function) script_md5, NULL, 1, "s", "data", SCRIPT_STRING | SCRIPT_FREE, 0},
	{"", "callevent", (Function) script_callevent, NULL, 1, "s", "event", SCRIPT_INTEGER, 0},
	{"", "script_export", script_export, NULL, 3, "ssc", "export-name syntax callback", SCRIPT_INTEGER, 0},
	{0}
};
