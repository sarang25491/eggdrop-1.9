/*
 * modules.c --
 *
 *	support for modules in eggdrop
 *
 * by Darrin Smith (beldin@light.iinet.net.au)
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
static const char rcsid[] = "$Id: modules.c,v 1.136 2003/05/12 14:11:42 wingman Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include "main.h"		/* NOTE: when removing this, include config.h */
#include "modules.h"
#include "core_binds.h"
#include "logfile.h"
#include "misc.h"
#include "cmdt.h"		/* cmd_t				*/
#include "core_binds.h"
/*#include "cmds.h"*/		/* check_dcc_attrs, check_dcc_chanattrs	*/
#include <ctype.h>
#include <ltdl.h>

extern struct dcc_t	*dcc;

extern struct userrec	*userlist, *lastuser;
extern char myname[], *botname, natip[], origbotname[], botuser[],
            admin[], userfile[], ver[], notify_new[], helpdir[], version[],
            quit_msg[];
extern int dcc_total, egg_numver, userfile_perm, ignore_time, learn_users,
           raw_log, make_userfile, default_flags, max_dcc, password_timeout,
           use_invites, use_exempts, force_expire, do_restart,
           protect_readonly, reserved_port_min, reserved_port_max, copy_to_tmp,
           quiet_reject;
extern time_t now, online_since;
extern egg_timeval_t egg_timeval_now;
extern struct chanset_t *chanset;
extern sock_list *socklist;

#ifndef MAKING_MODS
extern struct dcc_table DCC_CHAT_PASS, DCC_LOST, DCC_DNSWAIT,
			DCC_CHAT;
#endif /* MAKING_MODS   */

int xtra_kill();
int xtra_unpack();
static int module_rename(char *name, char *newname);


/* Directory to look for modules */
/* FIXME: the default should be the value of $pkglibdir in configure. */
/* FIXME: support multiple entries. */
char moddir[121] = "modules/";


/* The null functions */
void null_func()
{
}
char *charp_func()
{
  return NULL;
}
int minus_func()
{
  return -1;
}
int false_func()
{
  return 0;
}


/*
 *     Various hooks & things
 */

/* The REAL hooks, when these are called, a return of 0 indicates unhandled
 * 1 is handled
 */
struct hook_entry *hook_list[REAL_HOOKS];

void (*encrypt_pass) (char *, char *) = 0;
char *(*encrypt_string) (char *, char *) = 0;
char *(*decrypt_string) (char *, char *) = 0;
void (*qserver) (int, char *, int) = (void (*)(int, char *, int)) null_func;
void (*add_mode) () = null_func;
int (*match_noterej) (struct userrec *, char *) = (int (*)(struct userrec *, char *)) false_func;
int (*storenote)(char *from, char *to, char *msg, int idx, char *who) = (int (*)(char *from, char *to, char *msg, int idx, char *who)) minus_func;

module_entry *module_list;
dependancy *dependancy_list = NULL;

/* The horrible global lookup table for functions
 * BUT it makes the whole thing *much* more portable than letting each
 * OS screw up the symbols their own special way :/
 */

Function global_table[] =
{
  (Function) module_rename,
  (Function) module_register,
  (Function) module_find,
  (Function) module_depend,
  (Function) module_undepend,
  (Function) &egg_timeval_now,
  owner_check
};

static bind_table_t *BT_load, *BT_unload;

void modules_init()
{
  int i;
  char wbuf[1024];

  /* FIXME: keep this hack until the global table mess is gone */
  egg->global = global_table;

  BT_load = bind_table_add("load", 1, "s", MATCH_MASK, 0);
  BT_unload = bind_table_add("unload", 1, "s", MATCH_MASK, 0);

  module_list = malloc(sizeof(module_entry));
  module_list->name = strdup("eggdrop");
  module_list->major = (egg_numver) / 10000;
  module_list->minor = ((egg_numver) / 100) % 100;
  module_list->hand = NULL;
  module_list->next = NULL;
  module_list->funcs = NULL;

  LTDL_SET_PRELOADED_SYMBOLS();
  if (lt_dlinit() != 0) {
    snprintf(wbuf, sizeof(wbuf),
		    _("error during libtdl initialization: %s\n"),
		    lt_dlerror());
    fatal(wbuf, 0);
  }

  if (moddir[0] != '/') {
    if (getcwd(wbuf, sizeof(wbuf)) == NULL)
      fatal(_("Cant determine current directory."), 0);
    sprintf(&(wbuf[strlen(wbuf)]), "/%s", moddir);
    if (lt_dladdsearchdir(wbuf)) fatal(_("Invalid module's search path."), 0);
  } else
    if (lt_dladdsearchdir(moddir)) fatal(_("Invalid module's search path."), 0);

  for (i = 0; i < REAL_HOOKS; i++)
    hook_list[i] = NULL;
}

int module_register(char *name, Function * funcs,
		    int major, int minor)
{
  module_entry *p;

  for (p = module_list; p && p->name; p = p->next)
    if (!strcasecmp(name, p->name)) {
      p->major = major;
      p->minor = minor;
      p->funcs = funcs;
      return 1;
    }
  return 0;
}

const char *module_load(char *name)
{
  module_entry *p;
  char *e;
  Function f;
  lt_dlhandle hand;

  if (module_find(name, 0, 0) != NULL)
    return _("Already loaded.");
  hand = lt_dlopenext(name);
  if (!hand) {
    const char *err = lt_dlerror();
    putlog(LOG_MISC, "*", "Error loading module %s: %s", name, err);
    return err;
  }

  f = (Function) lt_dlsym(hand, "start");
    if (f == NULL) {
    lt_dlclose(hand);
      return _("No start function defined.");
    }
  p = malloc(sizeof(module_entry));
  if (p == NULL)
    return _("Malloc error");
  p->name = strdup(name);
  p->major = 0;
  p->minor = 0;
  p->hand = hand;
  p->funcs = 0;
  p->next = module_list;
  module_list = p;
  e = (((char *(*)()) f) (egg));
  if (e) {
    module_list = module_list->next;
    free(p->name);
    free(p);
    return e;
  }
  bind_check(BT_load, name, name);
  putlog(LOG_MISC, "*", _("Module loaded: %-16s"), name);
  return NULL;
}

char *module_unload(char *name, char *user)
{
  module_entry *p = module_list, *o = NULL;
  char *e;
  Function *f;

  while (p) {
    if ((p->name != NULL) && (!strcmp(name, p->name))) {
      dependancy *d;

      for (d = dependancy_list; d; d = d->next)
	if (d->needed == p)
	  return _("Needed by another module");

      f = p->funcs;
      if (f && !f[MODCALL_CLOSE])
	return _("No close function");
      if (f) {
	bind_check(BT_unload, name, name);
	e = (((char *(*)()) f[MODCALL_CLOSE]) (user));
	if (e != NULL)
	  return e;
	lt_dlclose(p->hand);
      }
      free(p->name);
      if (o == NULL) {
	module_list = p->next;
      } else {
	o->next = p->next;
      }
      free(p);
      putlog(LOG_MISC, "*", _("Module unloaded: %s"), name);
      return NULL;
    }
    o = p;
    p = p->next;
  }
  return _("No such module");
}

module_entry *module_find(char *name, int major, int minor)
{
  module_entry *p;

  for (p = module_list; p && p->name; p = p->next)
    if ((major == p->major || !major) && minor <= p->minor &&
	!strcasecmp(name, p->name))
      return p;
  return NULL;
}

static int module_rename(char *name, char *newname)
{
  module_entry *p;

  for (p = module_list; p; p = p->next)
    if (!strcasecmp(newname, p->name))
      return 0;

  for (p = module_list; p && p->name; p = p->next)
    if (!strcasecmp(name, p->name)) {
      free(p->name);
      p->name = strdup(newname);
      return 1;
    }
  return 0;
}

Function *module_depend(char *name1, char *name2, int major, int minor)
{
  module_entry *p = module_find(name2, major, minor);
  module_entry *o = module_find(name1, 0, 0);
  dependancy *d;

  if (!p) {
    if (module_load(name2))
      return 0;
    p = module_find(name2, major, minor);
  }
  if (!p || !o)
    return 0;
  d = malloc(sizeof(dependancy));

  d->needed = p;
  d->needing = o;
  d->next = dependancy_list;
  d->major = major;
  d->minor = minor;
  dependancy_list = d;
  return p->funcs ? p->funcs : (Function *) 1;
}

int module_undepend(char *name1)
{
  int ok = 0;
  module_entry *p = module_find(name1, 0, 0);
  dependancy *d = dependancy_list, *o = NULL;

  if (p == NULL)
    return 0;
  while (d != NULL) {
    if (d->needing == p) {
      if (o == NULL) {
	dependancy_list = d->next;
      } else {
	o->next = d->next;
      }
      free(d);
      if (o == NULL)
	d = dependancy_list;
      else
	d = o->next;
      ok++;
    } else {
      o = d;
      d = d->next;
    }
  }
  return ok;
}

/* Hooks, various tables of functions to call on ceratin events
 */
void add_hook(int hook_num, Function func)
{
  if (hook_num < REAL_HOOKS) {
    struct hook_entry *p;

    for (p = hook_list[hook_num]; p; p = p->next)
      if (p->func == func)
	return;			/* Don't add it if it's already there */
    p = malloc(sizeof(struct hook_entry));

    p->next = hook_list[hook_num];
    hook_list[hook_num] = p;
    p->func = func;
  } else
    switch (hook_num) {
    case HOOK_ENCRYPT_PASS:
      encrypt_pass = (void (*)(char *, char *)) func;
      break;
    case HOOK_ENCRYPT_STRING:
      encrypt_string = (char *(*)(char *, char *)) func;
      break;
    case HOOK_DECRYPT_STRING:
      decrypt_string = (char *(*)(char *, char *)) func;
      break;
    case HOOK_QSERV:
      if (qserver == (void (*)(int, char *, int)) null_func)
	qserver = (void (*)(int, char *, int)) func;
      break;
    case HOOK_ADD_MODE:
      if (add_mode == (void (*)()) null_func)
	add_mode = (void (*)()) func;
      break;
    case HOOK_MATCH_NOTEREJ:
      if (match_noterej == (int (*)(struct userrec *, char *))false_func)
	match_noterej = func;
      break;
    case HOOK_STORENOTE:
	if (func == NULL) storenote = (int (*)(char *from, char *to, char *msg, int idx, char *who)) minus_func;
	else storenote = func;
	break;
    }
}

void del_hook(int hook_num, Function func)
{
  if (hook_num < REAL_HOOKS) {
    struct hook_entry *p = hook_list[hook_num], *o = NULL;

    while (p) {
      if (p->func == func) {
	if (o == NULL)
	  hook_list[hook_num] = p->next;
	else
	  o->next = p->next;
	free(p);
	break;
      }
      o = p;
      p = p->next;
    }
  } else
    switch (hook_num) {
    case HOOK_ENCRYPT_PASS:
      if (encrypt_pass == (void (*)(char *, char *)) func)
	encrypt_pass = (void (*)(char *, char *)) null_func;
      break;
    case HOOK_ENCRYPT_STRING:
      if (encrypt_string == (char *(*)(char *, char *)) func)
        encrypt_string = (char *(*)(char *, char *)) null_func;
      break;
    case HOOK_DECRYPT_STRING:
      if (decrypt_string == (char *(*)(char *, char *)) func)
        decrypt_string = (char *(*)(char *, char *)) null_func;
      break;
    case HOOK_QSERV:
      if (qserver == (void (*)(int, char *, int)) func)
	qserver = null_func;
      break;
    case HOOK_ADD_MODE:
      if (add_mode == (void (*)()) func)
	add_mode = null_func;
      break;
    case HOOK_MATCH_NOTEREJ:
      if (match_noterej == (int (*)(struct userrec *, char *))func)
	match_noterej = false_func;
      break;
    case HOOK_STORENOTE:
	if (storenote == func) storenote = (int (*)(char *from, char *to, char *msg, int idx, char *who)) minus_func;
	break;
    }
}

int call_hook_cccc(int hooknum, char *a, char *b, char *c, char *d)
{
  struct hook_entry *p, *pn;
  int f = 0;

  if (hooknum >= REAL_HOOKS)
    return 0;
  p = hook_list[hooknum];
  for (p = hook_list[hooknum]; p && !f; p = pn) {
    pn = p->next;
    f = p->func(a, b, c, d);
  }
  return f;
}

void do_module_report(int idx, int details, char *which)
{
}
