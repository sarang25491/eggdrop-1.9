/*
 * tcluser.c -- handles:
 *   Tcl stubs for the user-record-oriented commands
 *
 * $Id: tcluser.c,v 1.33 2002/01/09 12:11:14 stdarg Exp $
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

#include "main.h"
#include "users.h"
#include "chan.h"
#include "tandem.h"
#include "script_api.h"

extern Tcl_Interp	*interp;
extern struct userrec	*userlist;
extern int		 default_flags, dcc_total, ignore_time;
extern struct dcc_t	*dcc;
extern char		 origbotname[], botnetnick[];
extern time_t		 now;


static int script_countusers()
{
	return count_users(userlist);
}

static int script_validuser(struct userrec *u)
{
	if (u) return(1);
	return(0);
}

static struct userrec *script_finduser(char *nick_user_host)
{
	return get_user_by_host(nick_user_host);
}

static int script_passwd_ok(struct userrec *u, char *trypass)
{
	if (u && u_pass_match(u, trypass)) return(1);
	return(0);
}

/* client_data will be NULL for chattr, non-NULL for botattr. */
/* I combined them since there are only minor differences. */
static char *script_chattr_botattr(void *client_data, struct userrec *u, char *changes, char *chan)
{
	static char work[128]; /* Static so it can be a return value. */
	struct flag_record pls, mns, user;
	int desired_flags;

	if (!u || (client_data && !(u->flags & USER_BOT))) return("*");

	if (client_data) desired_flags = FR_BOT;
	else desired_flags = FR_GLOBAL;

	if (chan) {
		user.match = desired_flags | FR_CHAN;
	}
	else if (changes) {
		if ((strchr(CHANMETA, changes[0]) != NULL)) {
			/* Determine if 'changes' is actually a channel name. */
			if (findchan_by_dname(changes)) {
				/* Yup. */
				chan = changes;
				changes = NULL;
				user.match = desired_flags | FR_CHAN;
			}
			else if (changes[0] != '+' && changes[1] != '-') {
				/* Invalid channel. */
				return("*");
			}
			else {
				/* Nope, they really are changes. */
				user.match = desired_flags;
			}
		}
		else {
			/* Ditto... */
			user.match = desired_flags;
		}
	}
	else {
		/* And again... */
		user.match = desired_flags;
	}
	if (chan && !findchan_by_dname(chan)) {
		return("*");
	}
	get_user_flagrec(u, &user, chan);
	/* Make changes */
	if (changes) {
		pls.match = user.match;
		break_down_flags(changes, &pls, &mns);
		/* Nobody can change these flags on-the-fly */
		pls.global &= ~(USER_BOT);
		mns.global &= ~(USER_BOT);
		if (chan) {
			pls.chan &= ~(BOT_SHARE);
			mns.chan &= ~(BOT_SHARE);
		}
		user.global = sanity_check((user.global|pls.global) & (~mns.global));
		user.bot = sanity_check((user.bot|pls.bot) & (~mns.bot));
		user.udef_global = (user.udef_global | pls.udef_global) & (~mns.udef_global);
		if (chan) {
			if (client_data) user.chan = (user.chan | pls.chan) & (~mns.chan);
			else user.chan = chan_sanity_check((user.chan | pls.chan) & (~mns.chan), user.global);

			user.udef_chan = (user.udef_chan | pls.udef_chan) & (~mns.udef_chan);
		}
		set_user_flagrec(u, &user, chan);
	}
	/* Retrieve current flags and return them */
	build_flags(work, &user, NULL);
	return(work);
}

static int tcl_matchattr STDVAR
{
  struct userrec *u;
  struct flag_record plus, minus, user;
  int ok = 0, f;

  BADARGS(3, 4, " handle flags ?channel?");
  if ((u = get_user_by_handle(userlist, argv[1]))) {
    user.match = FR_GLOBAL | (argc == 4 ? FR_CHAN : 0) | FR_BOT;
    get_user_flagrec(u, &user, argv[3]);
    plus.match = user.match;
    break_down_flags(argv[2], &plus, &minus);
    f = (minus.global || minus.udef_global || minus.chan ||
	 minus.udef_chan || minus.bot);
    if (flagrec_eq(&plus, &user)) {
      if (!f)
	ok = 1;
      else {
	minus.match = plus.match ^ (FR_AND | FR_OR);
	if (!flagrec_eq(&minus, &user))
	  ok = 1;
      }
    }
  }
  Tcl_AppendResult(irp, ok ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_adduser STDVAR
{
  BADARGS(2, 3, " handle ?hostmask?");
  if (strlen(argv[1]) > HANDLEN)
    argv[1][HANDLEN] = 0;
  if ((argv[1][0] == '*') || get_user_by_handle(userlist, argv[1]))
    Tcl_AppendResult(irp, "0", NULL);
  else {
    userlist = adduser(userlist, argv[1], argv[2], "-", default_flags);
    Tcl_AppendResult(irp, "1", NULL);
  }
  return TCL_OK;
}

static int tcl_addbot STDVAR
{
  struct bot_addr *bi;
  char *p, *q;
  char *addr;
  int addrlen;

  BADARGS(3, 3, " handle address");
  if (strlen(argv[1]) > HANDLEN)
     argv[1][HANDLEN] = 0;
  if (get_user_by_handle(userlist, argv[1]))
     Tcl_AppendResult(irp, "0", NULL);
  else if (argv[1][0] == '*')
     Tcl_AppendResult(irp, "0", NULL);
  else {
    userlist = adduser(userlist, argv[1], "none", "-", USER_BOT);
    bi = malloc(sizeof(struct bot_addr));
    addr = argv[2];
    if (*addr == '[') {
	addr++;
	if ((q = strchr(addr, ']'))) {
          addrlen = q - addr;
          q++;
          if (*q != ':')
            q = 0;
        } else
          addrlen = strlen(addr);
    } else {
        if ((q = strchr(addr, ':')))
	  addrlen = q - addr;
        else
	  addrlen = strlen(addr);
    }
    if (!q) {
      malloc_strcpy(bi->address, addr);
      bi->telnet_port = 3333;
      bi->relay_port = 3333;
    } else {
      bi->address = malloc(q - argv[2] + 1);
      strncpyz(bi->address, addr, addrlen + 1);
      p = q + 1;
      bi->telnet_port = atoi(p);
      q = strchr(p, '/');
      if (!q)
	bi->relay_port = bi->telnet_port;
      else
	bi->relay_port = atoi(q + 1);
    }
    set_user(&USERENTRY_BOTADDR, get_user_by_handle(userlist, argv[1]), bi);
    Tcl_AppendResult(irp, "1", NULL);
  }
  return TCL_OK;
}

static int tcl_deluser STDVAR
{
  BADARGS(2, 2, " handle");
  Tcl_AppendResult(irp, (argv[1][0] == '*') ? "0" :
		   int_to_base10(deluser(argv[1])), NULL);
  return TCL_OK;
}

static int tcl_delhost STDVAR
{
  BADARGS(3, 3, " handle hostmask");
  if ((!get_user_by_handle(userlist, argv[1])) || (argv[1][0] == '*')) {
    Tcl_AppendResult(irp, "non-existent user", NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, delhost_by_handle(argv[1], argv[2]) ? "1" : "0",
		   NULL);
  return TCL_OK;
}

static int tcl_userlist STDVAR
{
  struct userrec *u;
  struct flag_record user, plus, minus;
  int ok = 1, f = 0;

  BADARGS(1, 3, " ?flags ?channel??");
  if (argc == 3 && !findchan_by_dname(argv[2])) {
    Tcl_AppendResult(irp, "Invalid channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (argc >= 2) {
    plus.match = FR_GLOBAL | FR_CHAN | FR_BOT;
    break_down_flags(argv[1], &plus, &minus);
    f = (minus.global || minus.udef_global || minus.chan ||
	 minus.udef_chan || minus.bot);
  }
  minus.match = plus.match ^ (FR_AND | FR_OR);
  for (u = userlist; u; u = u->next) {
    if (argc >= 2) {
      user.match = FR_GLOBAL | FR_CHAN | FR_BOT | (argc == 3 ? 0 : FR_ANYWH);
      if (argc == 3) 
        get_user_flagrec(u, &user, argv[2]);
      else 
        get_user_flagrec(u, &user, NULL);
      if (flagrec_eq(&plus, &user) && !(f && flagrec_eq(&minus, &user)))
	ok = 1;
      else
	ok = 0;
    }
    if (ok)
      Tcl_AppendElement(interp, u->handle);
  }
  return TCL_OK;
}

static int tcl_save STDVAR
{
  write_userfile(-1);
  return TCL_OK;
}

static int tcl_reload STDVAR
{
  reload();
  return TCL_OK;
}

static int tcl_chhandle STDVAR
{
  struct userrec *u;
  char newhand[HANDLEN + 1];
  int x = 1, i;

  BADARGS(3, 3, " oldnick newnick");
  u = get_user_by_handle(userlist, argv[1]);
  if (!u)
     x = 0;
  else {
    strncpyz(newhand, argv[2], sizeof newhand);
    for (i = 0; i < strlen(newhand); i++)
      if ((newhand[i] <= 32) || (newhand[i] >= 127) || (newhand[i] == '@'))
	newhand[i] = '?';
    if (strchr(BADHANDCHARS, newhand[0]) != NULL)
      x = 0;
    else if (strlen(newhand) < 1)
      x = 0;
    else if (get_user_by_handle(userlist, newhand))
      x = 0;
    else if (!strcasecmp(botnetnick, newhand) &&
             (!(u->flags & USER_BOT) || nextbot (argv [1]) != -1))
      x = 0;
    else if (newhand[0] == '*')
      x = 0;
  }
  if (x)
     x = change_handle(u, newhand);
  Tcl_AppendResult(irp, x ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_getting_users STDVAR
{
  int i;

  BADARGS(1, 1, "");
  for (i = 0; i < dcc_total; i++) {
    if (dcc[i].type == &DCC_BOT && dcc[i].status & STAT_GETTING) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isignore STDVAR
{
  BADARGS(2, 2, " nick!user@host");
  Tcl_AppendResult(irp, match_ignore(argv[1]) ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_newignore STDVAR
{
  time_t expire_time;
  char ign[UHOSTLEN], cmt[66], from[HANDLEN + 1];

  BADARGS(4, 5, " hostmask creator comment ?lifetime?");
  strncpyz(ign, argv[1], sizeof ign);
  strncpyz(from, argv[2], sizeof from);
  strncpyz(cmt, argv[3], sizeof cmt);
  if (argc == 4)
     expire_time = now + (60 * ignore_time);
  else {
    if (argc == 5 && atol(argv[4]) == 0)
      expire_time = 0L;
    else
      expire_time = now + (60 * atol(argv[4])); /* This is a potential crash. FIXME  -poptix */
  }
  addignore(ign, from, cmt, expire_time);
  return TCL_OK;
}

static int tcl_killignore STDVAR
{
  BADARGS(2, 2, " hostmask");
  Tcl_AppendResult(irp, delignore(argv[1]) ? "1" : "0", NULL);
  return TCL_OK;
}

/* { hostmask note expire-time create-time creator }
 */
static int tcl_ignorelist STDVAR
{
  struct igrec *i;
  char expire[11], added[11], *list[5], *p;

  BADARGS(1, 1, "");
  for (i = global_ign; i; i = i->next) {
    list[0] = i->igmask;
    list[1] = i->msg;
    snprintf(expire, sizeof expire, "%lu", i->expire);
    list[2] = expire;
    snprintf(added, sizeof added, "%lu", i->added);
    list[3] = added;
    list[4] = i->user;
    p = Tcl_Merge(5, list);
    Tcl_AppendElement(irp, p);
    Tcl_Free((char *) p);
  }
  return TCL_OK;
}

static int tcl_getuser STDVAR
{
  struct user_entry_type *et;
  struct userrec *u;
  struct user_entry *e;

  BADARGS(3, 999, " handle type");
  if (!(et = find_entry_type(argv[2])) && strcasecmp(argv[2], "HANDLE")) {
    Tcl_AppendResult(irp, "No such info type: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (!(u = get_user_by_handle(userlist, argv[1]))) {
    if (argv[1][0] != '*') {
      Tcl_AppendResult(irp, "No such user.", NULL);
      return TCL_ERROR;
    } else
      return TCL_OK;		/* silently ignore user * */
  }
  if (!strcasecmp(argv[2], "HANDLE")) {
    Tcl_AppendResult(irp,u->handle, NULL);
  } else {
  e = find_user_entry(et, u);
  if (e)
    return et->tcl_get(irp, u, e, argc, argv);
  }
  return TCL_OK;
}

static int tcl_setuser STDVAR
{
  struct user_entry_type *et;
  struct userrec *u;
  struct user_entry *e;
  int r;

  BADARGS(3, 999, " handle type ?setting....?");
  if (!(et = find_entry_type(argv[2]))) {
    Tcl_AppendResult(irp, "No such info type: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (!(u = get_user_by_handle(userlist, argv[1]))) {
    if (argv[1][0] != '*') {
      Tcl_AppendResult(irp, "No such user.", NULL);
      return TCL_ERROR;
    } else
      return TCL_OK;		/* Silently ignore user * */
  }
  if (!(e = find_user_entry(et, u))) {
    e = malloc(sizeof(struct user_entry));
    e->type = et;
    e->name = NULL;
    e->u.list = NULL;
    list_insert((&(u->entries)), e);
  }
  r = et->tcl_set(irp, u, e, argc, argv);
  /* Yeah... e is freed, and we read it... (tcl: setuser hand HOSTS none) */
  if (!e->u.list) {
    if (list_delete((struct list_type **) &(u->entries),
		    (struct list_type *) e))
      free(e);
    /* else maybe already freed... (entry_type==HOSTS) <drummer> */
  }
  return r;
}

script_command_t script_user_cmds[] = {
	{"", "countusers", script_countusers, NULL, 0, "", "", SCRIPT_INTEGER, 0},
	{"", "validuser", script_validuser, NULL, 1, "U", "handle", SCRIPT_INTEGER, 0},
	{"", "finduser", (Function) script_finduser, NULL, 1, "s", "nick!user@host", SCRIPT_USER, 0},
	{"", "passwdok", script_passwd_ok, NULL, 2, "Us", "handle password", SCRIPT_INTEGER, 0},
	{"", "chattr", (Function) script_chattr_botattr, NULL, 1, "Uss", "handle ?changes ?channel??", SCRIPT_STRING, SCRIPT_PASS_CDATA|SCRIPT_VAR_ARGS},
	{"", "botattr", (Function) script_chattr_botattr, (void *)1, 1, "Uss", "bot ?changes ?channel??", SCRIPT_STRING, SCRIPT_PASS_CDATA|SCRIPT_VAR_ARGS},
	{0}
};

tcl_cmds tcluser_cmds[] =
{
  {"matchattr",		tcl_matchattr},
  {"adduser",		tcl_adduser},
  {"addbot",		tcl_addbot},
  {"deluser",		tcl_deluser},
  {"delhost",		tcl_delhost},
  {"userlist",		tcl_userlist},
  {"save",		tcl_save},
  {"reload",		tcl_reload},
  {"chhandle",		tcl_chhandle},
  {"chnick",		tcl_chhandle},
  {"getting-users",	tcl_getting_users},
  {"isignore",		tcl_isignore},
  {"newignore",		tcl_newignore},
  {"killignore",	tcl_killignore},
  {"ignorelist",	tcl_ignorelist},
  {"getuser",		tcl_getuser},
  {"setuser",		tcl_setuser},
  {NULL,		NULL}
};
