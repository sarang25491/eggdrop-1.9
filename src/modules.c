/* 
 * modules.c --
 *
 *	support for modules in eggdrop
 * 
 * by Darrin Smith (beldin@light.iinet.net.au)
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
static const char rcsid[] = "$Id: modules.c,v 1.119 2002/09/20 21:41:49 stdarg Exp $";
#endif

#include "main.h"		/* NOTE: when removing this, include config.h */
#include "modules.h"
#include "core_binds.h"
#include "logfile.h"
#include "misc.h"
#include "dns.h"
#include "cmdt.h"		/* cmd_t				*/
#include "tclhash.h"
#include "core_binds.h"
#include "chanprog.h"		/* clear_chanlist, reaffirm_owners, 
				   logmodes, masktype, isowner, 
				   clear_chanlist_member		*/
#include "cmds.h"		/* check_dcc_attrs, check_dcc_chanattrs	*/
#include "dccutil.h"		/* chatout, chanout_but, new_dcc,
				   lostdcc, makepass, flush_lines, 
				   changeover_dcc			*/
#include "net.h"		/* getlocaladdr, flush_inbuf, 
				   sockoptions, open_address_listen,
				   sock_has_data, iptostr, allocsock,
				   open_telnet, open_telnet_dcc, 
				   killsock, open_listen, getsock,
				   tputs, neterror, getmyip, answer	*/
#include "tcl.h"		/* findanyidx, do_tcl, readtclprog	*/
#include "users.h"		/* list_type				*/
#include "userent.h"		/* list_type_kill, xtra_set		*/
#include "userrec.h"		/* fixfrom, user_del_chan, touch_laston,
				   count_users, clear_userlist,
				   write_user, change_handle, 
				   write_userfile, u_pass_match, deluser,
				   addhost_by_handle, delhost_by_handle	*/
#include "irccmp.h"		/* _irccmp, _ircncmp, _irctolower, 
				   _irctoupper				*/
#include "match.h"		/* wild_match				*/


#include <ctype.h>
#include <ltdl.h>

extern struct dcc_t	*dcc;

extern Tcl_Interp	*interp;
extern struct userrec	*userlist, *lastuser;
extern char		 tempdir[], botnetnick[], *botname, natip[],
			 origbotname[], botuser[], admin[],
			 userfile[], ver[], notify_new[], helpdir[],
			 version[], quit_msg[];
extern int	 noshare, dcc_total, egg_numver, userfile_perm,
			 ignore_time, learn_users,
			 debug_output, make_userfile, default_flags,
			 max_dcc, share_greet, password_timeout,
			 use_invites, use_exempts, force_expire, do_restart,
			 protect_readonly, reserved_port_min, reserved_port_max;
extern time_t now, online_since;
extern egg_timeval_t egg_timeval_now;
extern struct chanset_t *chanset;
extern sock_list        *socklist;

#ifndef MAKING_MODS
extern struct dcc_table DCC_CHAT_PASS, DCC_LOST, DCC_DNSWAIT,
			DCC_CHAT; 
#endif /* MAKING_MODS   */

int cmd_die();
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

static void null_share(int idx, char *x)
{
  if ((x[0] == 'u') && (x[1] == 'n')) {
    putlog(LOG_BOTS, "*", _("User file rejected by %s: %s"),
	   dcc[idx].nick, x + 3);
    dcc[idx].status &= ~STAT_OFFERED;
    if (!(dcc[idx].status & STAT_GETTING)) {
      dcc[idx].status &= ~STAT_SHARE;
    }
  } else if ((x[0] != 'v') && (x[0] != 'e'))
    dprintf(idx, "s un Not sharing userfile.\n");
}

void (*encrypt_pass) (char *, char *) = 0;
char *(*encrypt_string) (char *, char *) = 0;
char *(*decrypt_string) (char *, char *) = 0;
void (*shareout) () = null_func;
void (*sharein) (int, char *) = null_share;
void (*qserver) (int, char *, int) = (void (*)(int, char *, int)) null_func;
void (*add_mode) () = null_func;
int (*irccmp) (const char *, const char *) = _irccmp;
int (*ircncmp) (const char *, const char *, int) = _ircncmp;
int (*irctolower) (int) = _irctolower;
int (*irctoupper) (int) = _irctoupper;
int (*match_noterej) (struct userrec *, char *) = (int (*)(struct userrec *, char *)) false_func;
int (*storenote)(char *from, char *to, char *msg, int idx, char *who, int bufsize) = (int (*)(char *from, char *to, char *msg, int idx, char *who, int bufsize)) minus_func;

module_entry *module_list;
dependancy *dependancy_list = NULL;

/* The horrible global lookup table for functions
 * BUT it makes the whole thing *much* more portable than letting each
 * OS screw up the symbols their own special way :/
 */

Function global_table[] =
{
  /* 0 - 3 */
  (Function) 0,
  (Function) 0,
  (Function) 0,
  (Function) module_rename,
  /* 4 - 7 */
  (Function) module_register,
  (Function) module_find,
  (Function) module_depend,
  (Function) module_undepend,
  /* 8 - 11 */
  (Function) 0,
  (Function) 0,
  (Function) 0,
  (Function) 0,
  /* 12 - 15 */
  (Function) 0,
  (Function) 0,
  (Function) add_tcl_commands,
  (Function) rem_tcl_commands,
  /* 16 - 19 */
  (Function) add_tcl_ints,
  (Function) rem_tcl_ints,
  (Function) add_tcl_strings,
  (Function) rem_tcl_strings,
  /* 20 - 23 */
  0,
  0,
  0,
  0,
  /* 24 - 27 */
  (Function) 0, /* botnet_send_zapf, */
  (Function) 0, /* botnet_send_zapf_broad, */
  (Function) 0, /* botnet_send_unlinked, */
  (Function) 0, /* botnet_send_bye, */
  /* 28 - 31 */
  (Function) 0, /* botnet_send_chat, */
  (Function) 0, /* botnet_send_filereject, */
  (Function) 0, /* botnet_send_filesend, */
  (Function) 0, /* botnet_send_filereq, */
  /* 32 - 35 */
  (Function) 0, /* botnet_send_join_idx, */
  (Function) 0, /* botnet_send_part_idx, */
  (Function) 0, /* updatebot, */
  (Function) 0, /* nextbot, */
  /* 36 - 39 */
  (Function) 0, /* zapfbot, */
  (Function) 0,
  (Function) u_pass_match,
  (Function) 0,
  /* 40 - 43 */
  (Function) get_user,
  (Function) set_user,
  (Function) add_entry_type,
  (Function) del_entry_type,
  /* 44 - 47 */
  (Function) get_user_flagrec,
  (Function) set_user_flagrec,
  (Function) get_user_by_host,
  (Function) get_user_by_handle,
  /* 48 - 51 */
  (Function) find_entry_type,
  (Function) find_user_entry,
  (Function) adduser,
  (Function) deluser,
  /* 52 - 55 */
  (Function) addhost_by_handle,
  (Function) delhost_by_handle,
  (Function) readuserfile,
  (Function) write_userfile,
  /* 56 - 59 */
  (Function) geticon,
  (Function) clear_chanlist,
  (Function) reaffirm_owners,
  (Function) change_handle,
  /* 60 - 63 */
  (Function) write_user,
  (Function) clear_userlist,
  (Function) count_users,
  (Function) sanity_check,
  /* 64 - 67 */
  (Function) break_down_flags,
  (Function) build_flags,
  (Function) flagrec_eq,
  (Function) flagrec_ok,
  /* 68 - 71 */
  (Function) 0, /* & shareout, */
  (Function) dprintf,
  (Function) chatout,
  (Function) chanout_but,
  /* 72 - 75 */
  (Function) 0,
  (Function) list_delete,
  (Function) list_append,
  (Function) list_contains,
  /* 76 - 79 */
  (Function) answer,
  (Function) getmyip,
  (Function) neterror,
  (Function) tputs,
  /* 80 - 83 */
  (Function) new_dcc,
  (Function) lostdcc,
  (Function) getsock,
  (Function) killsock,
  /* 84 - 87 */
  (Function) open_listen,
  (Function) open_telnet_dcc,
  (Function) 0,
  (Function) open_telnet,
  /* 88 - 91 */
  (Function) check_bind_event,
  (Function) 0,			/* memcpy				*/
  (Function) 0,
  (Function) 0,
  /* 92 - 95 */
  (Function) & dcc,		 /* struct dcc_t *			*/
  (Function) & chanset,		 /* struct chanset_t *			*/
  (Function) & userlist,	 /* struct userrec *			*/
  (Function) & lastuser,	 /* struct userrec *			*/
  /* 96 - 99 */
  (Function) & global_bans,	 /* struct banrec *			*/
  (Function) & global_ign,	 /* struct igrec *			*/
  (Function) & password_timeout, /* int					*/
  (Function) & share_greet,	 /* int					*/
  /* 100 - 103 */
  (Function) & max_dcc,		 /* int					*/
  (Function) 0,
  (Function) & ignore_time,	 /* int					*/
  (Function) 0,
  /* 104 - 107 */
  (Function) & reserved_port_min,
  (Function) & reserved_port_max,
  (Function) & debug_output,	 /* int					*/
  (Function) & noshare,		 /* int					*/
  /* 108 - 111 */
  (Function) 0, /* gban_total -- UNUSED! (Eule)				*/
  (Function) & make_userfile,	 /* int					*/
  (Function) & default_flags,	 /* int					*/
  (Function) & dcc_total,	 /* int					*/
  /* 112 - 115 */
  (Function) tempdir,		 /* char *				*/
  (Function) natip,		 /* char *				*/
  (Function) & learn_users,	 /* int *				*/
  (Function) origbotname,	 /* char *				*/
  /* 116 - 119 */
  (Function) botuser,		 /* char *				*/
  (Function) admin,		 /* char *				*/
  (Function) userfile,		 /* char *				*/
  (Function) ver,		 /* char *				*/
  /* 120 - 123 */
  (Function) notify_new,	 /* char *				*/
  (Function) helpdir,		 /* char *				*/
  (Function) version,		 /* char *				*/
  (Function) botnetnick,	 /* char *				*/
  /* 124 - 127 */
  (Function) & DCC_CHAT_PASS,	 /* struct dcc_table *			*/
  (Function) 0, /* & DCC_BOT,		 / struct dcc_table *			*/
  (Function) & DCC_LOST,	 /* struct dcc_table *			*/
  (Function) & DCC_CHAT,	 /* struct dcc_table *			*/
  /* 128 - 131 */
  (Function) & interp,		 /* Tcl_Interp *			*/
  (Function) & now,		 /* time_t				*/
  (Function) findanyidx,
  (Function) findchan,
  /* 132 - 135 */
  (Function) cmd_die,
  (Function) days,
  (Function) daysago,
  (Function) daysdur,
  /* 136 - 139 */
  (Function) ismember,
  (Function) 0,
  (Function) 0,
  (Function) 0,
  /* 140 - 143 */
  (Function) addignore,
  (Function) match_ignore,
  (Function) delignore,
  (Function) fatal,
  /* 144 - 147 */
  (Function) xtra_kill,
  (Function) xtra_unpack,
  (Function) 0,
  (Function) 0,
  /* 148 - 151 */
  (Function) do_tcl,
  (Function) readtclprog,
  (Function) 0,
  (Function) def_get,
  /* 152 - 155 */
  (Function) makepass,
  (Function) wild_match,
  (Function) maskhost,
  (Function) show_motd,
  /* 156 - 159 */
  (Function) tellhelp,
  (Function) showhelp,
  (Function) add_help_reference,
  (Function) rem_help_reference,
  /* 160 - 163 */
  (Function) touch_laston,
  (Function) & add_mode,	/* Function *				*/
  (Function) 0,
  (Function) 0, /* in_chain, */
  /* 164 - 167 */
  (Function) add_note,
  (Function) 0,
  (Function) detect_dcc_flood,
  (Function) flush_lines,
  /* 168 - 171 */
  (Function) 0,
  (Function) 0,
  (Function) & do_restart,	/* int					*/
  (Function) check_bind_filt,
  /* 172 - 175 */
  (Function) add_hook,
  (Function) del_hook,
  (Function) 0,
  (Function) 0,		/* p_tcl_bind_list *			*/
  /* 176 - 179 */
  (Function) 0,		/* p_tcl_bind_list *			*/
  (Function) 0,		/* p_tcl_bind_list *			*/
  (Function) 0,		/* p_tcl_bind_list *			*/
  (Function) 0,		/* p_tcl_bind_list *			*/
  /* 180 - 183 */
  (Function) 0,		/* p_tcl_bind_list *			*/
  (Function) 0,		/* p_tcl_bind_list *			*/
  (Function) 0,		/* p_tcl_bind_list *			*/
  (Function) 0,		/* p_tcl_bind_list *			*/
  /* 184 - 187 */
  (Function) 0,		/* p_tcl_bind_list *			*/
  (Function) 0,		/* p_tcl_bind_list *			*/
  (Function) 0,		/* p_tcl_bind_list *			*/
  (Function) 0,		/* p_tcl_bind_list *			*/
  /* 188 - 191 */
  (Function) & USERENTRY_BOTADDR,	/* struct user_entry_type *	*/
  (Function) & USERENTRY_BOTFL,		/* struct user_entry_type *	*/
  (Function) & USERENTRY_HOSTS,		/* struct user_entry_type *	*/
  (Function) & USERENTRY_PASS,		/* struct user_entry_type *	*/
  /* 192 - 195 */
  (Function) & USERENTRY_XTRA,		/* struct user_entry_type *	*/
  (Function) user_del_chan,
  (Function) & USERENTRY_INFO,		/* struct user_entry_type *	*/
  (Function) & USERENTRY_COMMENT,	/* struct user_entry_type *	*/
  /* 196 - 199 */
  (Function) & USERENTRY_LASTON,	/* struct user_entry_type *	*/
  (Function) putlog,
  (Function) 0, /* botnet_send_chan, */
  (Function) list_type_kill,
  /* 200 - 203 */
  (Function) logmodes,
  (Function) masktype,
  (Function) 0,
  (Function) 0,
  /* 204 - 207 */
  (Function) sub_lang,
  (Function) & online_since,	/* time_t *				*/
  (Function) 0,
  (Function) check_dcc_attrs,
  /* 208 - 211 */
  (Function) check_dcc_chanattrs,
  (Function) add_tcl_coups,
  (Function) rem_tcl_coups,
  (Function) &botname,
  /* 212 - 215 */
  (Function) 0,			/* remove_gunk() -- UNUSED! (drummer)	*/
  (Function) check_bind_chjn,
  (Function) 0,
  (Function) isowner,
  /* 216 - 219 */
  (Function) 0,			/* min_dcc_port -- UNUSED! (guppy)	*/
  (Function) 0,			/* max_dcc_port -- UNUSED! (guppy)	*/
  (Function) & irccmp,		/* Function *				*/
  (Function) & ircncmp,		/* Function *				*/
  /* 220 - 223 */
  (Function) & global_exempts,	/* struct exemptrec *			*/
  (Function) & global_invites,	/* struct inviterec *			*/
  (Function) 0, /* ginvite_total -- UNUSED! (Eule)			*/
  (Function) 0, /* gexempt_total -- UNUSED! (Eule)			*/
  /* 224 - 227 */
  (Function) 0,			/* p_tcl_bind_list *			*/
  (Function) & use_exempts,	/* int					*/
  (Function) & use_invites,	/* int					*/
  (Function) & force_expire,	/* int					*/
  /* 228 - 231 */
  (Function) 0,
  (Function) 0,
  (Function) 0,
  (Function) xtra_set,
  /* 232 - 235 */
  (Function) 0,
  (Function) 0,
  (Function) allocsock,
  (Function) call_hostbyip,
  /* 236 - 239 */
  (Function) call_ipbyhost,
  (Function) iptostr,
  (Function) & DCC_DNSWAIT,	 /* struct dcc_table *			*/
  (Function) 0,
  /* 240 - 243 */
  (Function) dcc_dnsipbyhost,
  (Function) dcc_dnshostbyip,
  (Function) changeover_dcc,  
  (Function) make_rand_str,
  /* 244 - 247 */
  (Function) & protect_readonly, /* int					*/
  (Function) findchan_by_dname,
  (Function) 0,
  (Function) & userfile_perm,	 /* int					*/
  /* 248 - 251 */
  (Function) sock_has_data,
  (Function) 0, /* bots_in_subtree, */
  (Function) 0, /* users_in_subtree, */
  (Function) 0,			/* inet_aton				*/
  /* 252 - 255 */
  (Function) 0,			/* snprintf				*/
  (Function) 0,			/* vsnprintf				*/
  (Function) 0,			/* memset				*/
  (Function) 0,			/* strcasecmp				*/
  /* 256 - 259 */
  (Function) fixfrom,
  (Function) 0,
  (Function) 0,
  (Function) 0, /* & tandbot,		/ tand_t *				*/
  /* 260 - 263 */
  (Function) 0, /* & party,		/ party_t *				*/
  (Function) open_address_listen,
  (Function) 0,
  (Function) 0,
  /* 264 - 267 */
  (Function) 0,
  (Function) 0,
  (Function) clear_chanlist_member,
  (Function) 0,
  /* 268 - 271 */
  (Function) & socklist,              /* sock_list *                      */
  (Function) sockoptions,
  (Function) flush_inbuf,
#ifdef IPV6
  (Function) getmyip6,
#else
  (Function) 0,
#endif
  /* 272 - 275 */ 
  (Function) getlocaladdr,
  (Function) kill_bot,
  (Function) quit_msg,                /* char *				  */
  (Function) bind_table_add,
  /* 276 - 279 */
  (Function) bind_table_del,
  (Function) add_builtins,
  (Function) rem_builtins,
  (Function) bind_table_lookup,
  /* 280 - 283 */
  (Function) check_bind,
  (Function) &egg_timeval_now,
  0,
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
  check_bind(BT_load, name, NULL, name);
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
	check_bind(BT_unload, name, NULL, name);
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
    /* special hook <drummer> */
    case HOOK_IRCCMP:
      if (func == NULL) {
	irccmp = strcasecmp;
	ircncmp = (int (*)(const char *, const char *, int)) strncasecmp;
	irctolower = tolower;
	irctoupper = toupper;
      } else {
	irccmp = _irccmp;
	ircncmp = _ircncmp;
	irctolower = _irctolower;
	irctoupper = _irctoupper;
      }
      break;
    case HOOK_MATCH_NOTEREJ:
      if (match_noterej == (int (*)(struct userrec *, char *))false_func)
	match_noterej = func;
      break;
    case HOOK_STORENOTE:
	if (func == NULL) storenote = (int (*)(char *from, char *to, char *msg, int idx, char *who, int bufsize)) minus_func;
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
    case HOOK_SHAREOUT:
      if (shareout == (void (*)()) func)
	shareout = null_func;
      break;
    case HOOK_SHAREIN:
      if (sharein == (void (*)(int, char *)) func)
	sharein = null_share;
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
	if (storenote == func) storenote = (int (*)(char *from, char *to, char *msg, int idx, char *who, int bufsize)) minus_func;
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
  module_entry *p = module_list;

  if (p && !which && details)
    dprintf(idx, _("MODULES LOADED:\n"));
  for (; p; p = p->next) {
    if (!which || !strcasecmp(which, p->name)) {
      dependancy *d;

      if (details)
	dprintf(idx, "Module: %s, v %d.%d\n", p->name ? p->name : "CORE",
		p->major, p->minor);
      if (details > 1) {
	for (d = dependancy_list; d; d = d->next) 
	  if (d->needing == p)
	    dprintf(idx, "    requires: %s, v %d.%d\n", d->needed->name,
		    d->major, d->minor);
      }
      if (p->funcs) {
	Function f = p->funcs[MODCALL_REPORT];

	if (f != NULL)
	  f(idx, details);
      }
      if (which)
	return;
    }
  }
  if (which)
    dprintf(idx, _("No such module.\n"));
}
