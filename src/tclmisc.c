/*
 * tclmisc.c -- handles:
 *   Tcl stubs for file system commands
 *   Tcl stubs for everything else
 *
 * $Id: tclmisc.c,v 1.39 2002/01/16 18:09:22 stdarg Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001 Eggheads Development Team
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

#include <sys/stat.h>
#include "main.h"
#include "modules.h"
#include "core_binds.h"
#include "tandem.h"
#include "md5.h"
#include "script_api.h"
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

  tt = (time_t) sec;
  return ctime(&tt);
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

  strncpyz(trunc_from, from, sizeof(trunc_from));
  strncpyz(trunc_to, to, sizeof(trunc_to));
  strncpyz(trunc_msg, msg, sizeof(trunc_msg));
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
    strncpyz(quit_msg, reason, 1024);
  } else {
    strncpyz(s, "BOT SHUTDOWN (No reason)", sizeof s);
    quit_msg[0] = 0;
  }
  kill_bot(s, quit_msg[0] ? quit_msg : "EXIT");
  return TCL_OK;
}

static int tcl_loadmodule STDVAR
{
  const char *p;

  BADARGS(2, 2, " module-name");
  p = module_load(argv[1]);
  if (p && strcmp(p, _("Already loaded.")))
    putlog(LOG_MISC, "*", "%s %s: %s", _("Cant load modules"), argv[1], p);
  Tcl_AppendResult(irp, p, NULL);
  return TCL_OK;
}

static int tcl_unloadmodule STDVAR
{
  BADARGS(2, 2, " module-name");
  Tcl_AppendResult(irp, module_unload(argv[1], origbotname), NULL);
  return TCL_OK;
}

static int tcl_unames STDVAR
{
  char *unix_n, *vers_n;
#ifdef HAVE_UNAME
  struct utsname un;

  if (uname(&un) < 0) {
#endif
    unix_n = "*unkown*";
    vers_n = "";
#ifdef HAVE_UNAME
  } else {
    unix_n = un.sysname;
    vers_n = un.release;
  }
#endif
  Tcl_AppendResult(irp, unix_n, " ", vers_n, NULL);
  return TCL_OK;
}

static int tcl_modules STDVAR
{
  module_entry *current;
  dependancy *dep;
  char *list[100], *list2[2], *p;
  char s[24], s2[24];
  int i;

  BADARGS(1, 1, "");
  for (current = module_list; current; current = current->next) {
    list[0] = current->name;
    snprintf(s, sizeof s, "%d.%d", current->major, current->minor);
    list[1] = s;
    i = 2;
    for (dep = dependancy_list; dep && (i < 100); dep = dep->next) {
      if (dep->needing == current) {
	list2[0] = dep->needed->name;
	snprintf(s2, sizeof s2, "%d.%d", dep->major, dep->minor);
	list2[1] = s2;
	list[i] = Tcl_Merge(2, list2);
	i++;
      }
    }
    p = Tcl_Merge(i, list);
    Tcl_AppendElement(irp, p);
    Tcl_Free((char *) p);
    while (i > 2) {
      i--;
      Tcl_Free((char *) list[i]);
    }
  }
  return TCL_OK;
}

static int tcl_loadhelp STDVAR
{
  BADARGS(2, 2, " helpfile-name");
  add_help_reference(argv[1]);
  return TCL_OK;
}

static int tcl_unloadhelp STDVAR
{
  BADARGS(2, 2, " helpfile-name");
  rem_help_reference(argv[1]);
  return TCL_OK;
}

static int tcl_reloadhelp STDVAR
{
  BADARGS(1, 1, "");
  reload_help_data();
  return TCL_OK;
}

static int tcl_callevent STDVAR
{
  BADARGS(2, 2, " event");
  check_bind_event(argv[1]);
  return TCL_OK;
}

#if (TCL_MAJOR_VERSION >= 8)
static int tcl_md5(cd, irp, objc, objv)
ClientData cd;
Tcl_Interp *irp;
int objc;
Tcl_Obj *CONST objv[];
{
#else
static int tcl_md5 STDVAR
{
#endif
  MD5_CTX       md5context;
  char digest_string[33], *string;
  unsigned char digest[16];
  int i, len;

#if (TCL_MAJOR_VERSION >= 8)
  if (objc != 2) {
    Tcl_WrongNumArgs(irp, 1, objv, "string");
    return TCL_ERROR;
  }

#if (TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION < 1)
  string = Tcl_GetStringFromObj(objv[1], &len);
#else
  string = Tcl_GetByteArrayFromObj(objv[1], &len);
#endif

#else
  BADARGS(2, 2, " string");
  string = argv[1];
  len = strlen(argv[1]);
#endif

  MD5_Init(&md5context);
  MD5_Update(&md5context, (unsigned char *)string, len);
  MD5_Final(digest, &md5context);
  for(i=0; i<16; i++)
    sprintf(digest_string + (i*2), "%.2x", digest[i]);
  Tcl_AppendResult(irp, digest_string, NULL);
  return TCL_OK;
}

tcl_cmds tclmisc_objcmds[] =
{
#if (TCL_MAJOR_VERSION >= 8)
  {"md5",	tcl_md5},
#endif
  {NULL,	NULL}
};

script_command_t script_misc_cmds[] = {
	{"", "duration", (Function) script_duration, NULL, 1, "u", "seconds", SCRIPT_STRING|SCRIPT_FREE, 0},
	{"", "unixtime", (Function) script_unixtime, NULL, 0, "", "", SCRIPT_UNSIGNED, 0},
	{"", "ctime", (Function) script_ctime, NULL, 1, "u", "seconds", SCRIPT_STRING|SCRIPT_FREE, 0},
	{"", "strftime", (Function) script_strftime, NULL, 1, "su", "format ?seconds?", SCRIPT_STRING|SCRIPT_FREE, SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{"", "myip", (Function) script_myip, NULL, 0, "", "", SCRIPT_UNSIGNED, 0},
	{"", "myip6", (Function) script_myip6, NULL, 0, "", "", SCRIPT_STRING|SCRIPT_FREE, 0},
	{"", "rand", (Function) script_rand, NULL, 1, "ii", "?min? max", SCRIPT_INTEGER, SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{"", "sendnote", (Function) script_sendnote, NULL, 3, "sss", "from to msg", SCRIPT_INTEGER, 0},
	{"", "dumpfile", (Function) script_dumpfile, NULL, 2, "ss", "nick filename", SCRIPT_INTEGER, 0},
	{"", "dccdumpfile", (Function) script_dccdumpfile, NULL, 2, "is", "idx filename", SCRIPT_INTEGER, 0},
	{"", "backup", (Function) script_backup, NULL, 0, "", "", SCRIPT_INTEGER},
	{"", "die", (Function) script_die, NULL, 0, "s", "?reason?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{0}
};

tcl_cmds tclmisc_cmds[] =
{
  {"unames",		tcl_unames},
  {"unloadmodule",	tcl_unloadmodule},
  {"loadmodule",	tcl_loadmodule},
  {"modules",		tcl_modules},
  {"loadhelp",		tcl_loadhelp},
  {"unloadhelp",	tcl_unloadhelp},
  {"reloadhelp",	tcl_reloadhelp},
#if (TCL_MAJOR_VERSION < 8)
  {"md5",		tcl_md5},
#endif
  {"callevent",		tcl_callevent},
  {NULL,		NULL}
};
