/*
 * tclmisc.c -- handles:
 *   Tcl stubs for file system commands
 *   Tcl stubs for everything else
 *
 * $Id: tclmisc.c,v 1.37 2002/01/04 04:49:49 ite Exp $
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
#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

extern struct dcc_t	*dcc;
extern char		 origbotname[], botnetnick[], quit_msg[];
extern struct userrec	*userlist;
extern time_t		 now;
extern module_entry	*module_list;

static int tcl_duration STDVAR
{
  char s[70];
  unsigned long sec, tmp;

  BADARGS(2, 2, " seconds");
  if (atol(argv[1]) <= 0) {
    Tcl_AppendResult(irp, "0 seconds", NULL);
    return TCL_OK;
  }
  sec = atol(argv[1]);
  s[0] = 0;
  if (sec >= 31536000) {
    tmp = (sec / 31536000);
    sprintf(s, "%lu year%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 31536000);
  }
  if (sec >= 604800) {
    tmp = (sec / 604800);
    sprintf(&s[strlen(s)], "%lu week%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 604800);
  }
  if (sec >= 86400) {
    tmp = (sec / 86400);
    sprintf(&s[strlen(s)], "%lu day%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 86400);
  }
  if (sec >= 3600) {
    tmp = (sec / 3600);
    sprintf(&s[strlen(s)], "%lu hour%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 3600);
  }
  if (sec >= 60) {
    tmp = (sec / 60);
    sprintf(&s[strlen(s)], "%lu minute%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 60);
  }
  if (sec > 0) {
    tmp = (sec);
    sprintf(&s[strlen(s)], "%lu second%s", tmp, (tmp == 1) ? "" : "s");
  }
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_unixtime STDVAR
{
  char s[11];

  BADARGS(1, 1, "");
  snprintf(s, sizeof s, "%lu", (unsigned long) now);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_ctime STDVAR
{
  time_t tt;
  char s[25];

  BADARGS(2, 2, " unixtime");
  tt = (time_t) atol(argv[1]);
  strncpyz(s, ctime(&tt), sizeof s);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_strftime STDVAR
{
  char buf[512];
  struct tm *tm1;
  time_t t;

  BADARGS(2, 3, " format ?time?");
  if (argc == 3)
    t = atol(argv[2]);
  else
    t = now;
    tm1 = localtime(&t);
  if (strftime(buf, sizeof(buf) - 1, argv[1], tm1)) {
    Tcl_AppendResult(irp, buf, NULL);
    return TCL_OK;
  }
  Tcl_AppendResult(irp, " error with strftime", NULL);
  return TCL_ERROR;
}

static int tcl_myip STDVAR
{
  char s[16];

  BADARGS(1, 1, "");
  snprintf(s, sizeof s, "%lu", iptolong(getmyip()));
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_myip6 STDVAR
{
    BADARGS(1, 1, "");
#ifdef IPV6
    Tcl_AppendResult(irp, "", NULL); /* FIXME!! */
#else
    Tcl_AppendResult(irp, "", NULL);
#endif
    return TCL_OK;
}

static int tcl_rand STDVAR
{
  unsigned long x;
  char s[11];

  BADARGS(2, 2, " limit");
  if (atol(argv[1]) <= 0) {
    Tcl_AppendResult(irp, "random limit must be greater than zero", NULL);
    return TCL_ERROR;
  }
  x = random() % (atol(argv[1]));
  snprintf(s, sizeof s, "%lu", x);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_sendnote STDVAR
{
  char s[5], from[NOTENAMELEN + 1], to[NOTENAMELEN + 1], msg[451];

  BADARGS(4, 4, " from to message");
  strncpyz(from, argv[1], sizeof from);
  strncpyz(to, argv[2], sizeof to);
  strncpyz(msg, argv[3], sizeof msg);
  snprintf(s, sizeof s, "%d", add_note(to, from, msg, -1, 0));
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_dumpfile STDVAR
{
  char nick[NICKLEN];
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  BADARGS(3, 3, " nickname filename");
  strncpyz(nick, argv[1], sizeof nick);
  get_user_flagrec(get_user_by_nick(nick), &fr, NULL);
  showhelp(argv[1], argv[2], &fr, HELP_TEXT);
  return TCL_OK;
}

static int tcl_dccdumpfile STDVAR
{
  int idx, i;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  BADARGS(3, 3, " idx filename");
  i = atoi(argv[1]);
  idx = findidx(i);
  if (idx < 0) {
    Tcl_AppendResult(irp, "illegal idx", NULL);
    return TCL_ERROR;
  }
  get_user_flagrec(get_user_by_handle(userlist, dcc[idx].nick), &fr, NULL);
  tellhelp(idx, argv[2], &fr, HELP_TEXT);
  return TCL_OK;
}

static int tcl_backup STDVAR
{
  BADARGS(1, 1, "");
  call_hook(HOOK_BACKUP);
  return TCL_OK;
}

static int tcl_die STDVAR
{
  char s[1024];

  BADARGS(1, 2, " ?reason?");
  if (argc == 2) {
    snprintf(s, sizeof s, "BOT SHUTDOWN (%s)", argv[1]);
    strncpyz(quit_msg, argv[1], 1024);
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

tcl_cmds tclmisc_cmds[] =
{
  {"unixtime",		tcl_unixtime},
  {"strftime",          tcl_strftime},
  {"ctime",		tcl_ctime},
  {"myip",		tcl_myip},
  {"rand",		tcl_rand},
  {"sendnote",		tcl_sendnote},
  {"dumpfile",		tcl_dumpfile},
  {"dccdumpfile",	tcl_dccdumpfile},
  {"backup",		tcl_backup},
  {"exit",		tcl_die},
  {"die",		tcl_die},
  {"unames",		tcl_unames},
  {"unloadmodule",	tcl_unloadmodule},
  {"loadmodule",	tcl_loadmodule},
  {"modules",		tcl_modules},
  {"loadhelp",		tcl_loadhelp},
  {"unloadhelp",	tcl_unloadhelp},
  {"reloadhelp",	tcl_reloadhelp},
  {"duration",		tcl_duration},
#if (TCL_MAJOR_VERSION < 8)
  {"md5",		tcl_md5},
#endif
  {"callevent",		tcl_callevent},
  {"myip6",		tcl_myip6},
  {NULL,		NULL}
};
