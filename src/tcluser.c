/*
 * tcluser.c -- handles:
 *   Tcl stubs for the user-record-oriented commands
 *
 * $Id: tcluser.c,v 1.34 2002/01/11 20:06:29 stdarg Exp $
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
#include "script.h"

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

static int script_matchattr(struct userrec *u, char *flags, char *chan)
{
	struct flag_record plus, minus, user;
	int has_minus_flags;

	if (!u) return(0);
	user.match = FR_GLOBAL | FR_BOT;
	if (chan) user.match |= FR_CHAN;
	get_user_flagrec(u, &user, chan);
	plus.match = user.match;
	break_down_flags(chan, &plus, &minus);
	has_minus_flags = (minus.global || minus.udef_global || minus.chan || minus.udef_chan || minus.bot);
	if (flagrec_eq(&plus, &user)) {
		if (!has_minus_flags) return(1);
		else {
			minus.match = plus.match ^ (FR_AND | FR_OR);
			if (!flagrec_eq(&minus, &user)) return(1);
		}
	}
	return(0);
}

static int script_adduser(char *handle, char *hostmask)
{
	char realhandle[HANDLEN];

	strncpy(realhandle, handle, HANDLEN);
	realhandle[HANDLEN-1] = 0;

	if (realhandle[0] == '*' || get_user_by_handle(userlist, realhandle)) return(0);

	userlist = adduser(userlist, handle, hostmask, "-", default_flags);
	return(1);
}

static int script_addbot(char *handle, char *address)
{
	struct bot_addr *bi;
	char *p, *q;
	char *orig, *addr, realhandle[HANDLEN];
	int addrlen = 0;

	strncpy(realhandle, handle, HANDLEN);
	realhandle[HANDLEN-1] = 0;
	if (get_user_by_handle(userlist, realhandle)) return(0);
  	if (realhandle[0] == '*') return(0);
	userlist = adduser(userlist, realhandle, "none", "-", USER_BOT);

	bi = (struct bot_addr *)malloc(sizeof(*bi));
	addr = strdup(address);
	orig = addr;
	if (*addr == '[') {
		addr++;
		if ((q = strchr(addr, ']'))) {
			addrlen = q - addr;
			q++;
			if (*q != ':') q = 0;
			else addrlen = strlen(addr);
		}
	}
	else if ((q = strchr(addr, ':'))) addrlen = q - addr;
	else addrlen = strlen(addr);
	if (!q) {
		malloc_strcpy(bi->address, addr);
		bi->telnet_port = 3333;
		bi->relay_port = 3333;
	}
	else {
		bi->address = malloc(q - addr + 1);
		strncpyz(bi->address, addr, addrlen + 1);
		p = q + 1;
		bi->telnet_port = atoi(p);
		q = strchr(p, '/');
		if (!q) bi->relay_port = bi->telnet_port;
		else bi->relay_port = atoi(q + 1);
	}
	free(orig);
	set_user(&USERENTRY_BOTADDR, get_user_by_handle(userlist, realhandle), bi);
	return(1);
}

static int script_deluser(struct userrec *u)
{
	if (!u) return(0);
	return deluser(u->handle);
}

static int script_delhost(char *handle, char *hostmask)
{
	return delhost_by_handle(handle, hostmask);
}

static int script_userlist(script_var_t *retval, char *flags, char *chan)
{
  struct userrec *u;
  struct flag_record user, plus, minus;
  int ok = 1, f = 0;

	retval->type = SCRIPT_ARRAY | SCRIPT_VAR | SCRIPT_FREE;
	retval->len = 0;
	retval->value = NULL;

  if (chan && !findchan_by_dname(chan)) return(0);
  if (flags) {
    plus.match = FR_GLOBAL | FR_CHAN | FR_BOT;
    break_down_flags(flags, &plus, &minus);
    f = (minus.global || minus.udef_global || minus.chan ||
	 minus.udef_chan || minus.bot);
  }
  minus.match = plus.match ^ (FR_AND | FR_OR);
  for (u = userlist; u; u = u->next) {
    if (flags) {
      user.match = FR_GLOBAL | FR_CHAN | FR_BOT | (chan ? 0 : FR_ANYWH);
      get_user_flagrec(u, &user, chan);
      if (flagrec_eq(&plus, &user) && !(f && flagrec_eq(&minus, &user)))
	ok = 1;
      else
	ok = 0;
    }
    if (ok) script_list_append(retval, script_string(u->handle, -1));
  }
  return(0);
}

static int script_save()
{
  write_userfile(-1);
  return(0);
}

static int script_reload()
{
  reload();
  return(0);
}

static int script_chhandle(struct userrec *u, char *desired_handle)
{
  char newhand[HANDLEN + 1];
  int i;

	if (!u) return(0);
	strncpyz(newhand, desired_handle, sizeof(newhand));
    for (i = 0; i < strlen(newhand); i++)
      if ((newhand[i] <= 32) || (newhand[i] >= 127) || (newhand[i] == '@'))
	newhand[i] = '?';
    if (strchr(BADHANDCHARS, newhand[0]) != NULL) return(0);
    if (strlen(newhand) < 1) return(0);
    if (get_user_by_handle(userlist, newhand)) return(0);
    if (!strcasecmp(botnetnick, newhand) &&
             (!(u->flags & USER_BOT) || nextbot(desired_handle) != -1)) return(0);
    if (newhand[0] == '*') return(0);

	return change_handle(u, newhand);
}

static int script_getting_users()
{
  int i;

  for (i = 0; i < dcc_total; i++) {
    if (dcc[i].type == &DCC_BOT && dcc[i].status & STAT_GETTING) {
	return(1);
    }
  }
  return(0);
}

static int script_isignore(char *nick_user_host)
{
  return match_ignore(nick_user_host);
}

static int script_newignore(int nargs, char *hostmask, char *creator, char *comment, int lifetime)
{
  time_t expire_time;
  char ign[UHOSTLEN], cmt[66], from[HANDLEN + 1];

  //BADARGS(4, 5, " hostmask creator comment ?lifetime?");
  strncpyz(ign, hostmask, sizeof ign);
  strncpyz(from, creator, sizeof from);
  strncpyz(cmt, comment, sizeof cmt);
  if (lifetime < 0) lifetime = 0;
  if (nargs == 3) expire_time = now + (60 * ignore_time);
  else {
    if (!lifetime) expire_time = 0L;
    else expire_time = now + (60 * lifetime);
  }
  addignore(ign, from, cmt, expire_time);
  return TCL_OK;
}

static int script_killignore(char *hostmask)
{
  return delignore(hostmask);
}

/* { hostmask note expire-time create-time creator }
 */
static int script_ignorelist(script_var_t *retval)
{
  script_var_t *sublist;
  struct igrec *i;

	retval->type = SCRIPT_ARRAY | SCRIPT_VAR | SCRIPT_FREE;
	retval->len = 0;
	retval->value = NULL;

  for (i = global_ign; i; i = i->next) {
	sublist = script_list(5, script_string(i->igmask, -1),
		script_string(i->msg, -1),
		script_int(i->expire),
		script_int(i->added),
		script_string(i->user, -1));
	script_list_append(retval, sublist);
  }
  return(0);
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
	{"", "matchattr", (Function) script_matchattr, NULL, 2, "Uss", "handle flags ?channel?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "adduser", (Function) script_adduser, NULL, 1, "ss", "handle ?hostmask?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "addbot", (Function) script_addbot, NULL, 2, "ss", "handle address", SCRIPT_INTEGER, 0},
	{"", "deluser", (Function) script_deluser, NULL, 1, "U", "handle", SCRIPT_INTEGER, 0},
	{"", "delhost", (Function) script_delhost, NULL, 2, "ss", "handle hostmask", SCRIPT_INTEGER, 0},
	{"", "userlist", (Function) script_userlist, NULL, 0, "ss", "?flags ?channel??", 0, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS},
	{"", "save", (Function) script_save, NULL, 0, "", "", SCRIPT_INTEGER, 0},
	{"", "reload", (Function) script_reload, NULL, 0, "", "", SCRIPT_INTEGER, 0},
	{"", "chhandle", (Function) script_chhandle, NULL, 2, "Us", "handle new-handle", SCRIPT_INTEGER, 0},
	{"", "getting_users", (Function) script_getting_users, NULL, 0, "", "", SCRIPT_INTEGER, 0},
	{"", "isignore", (Function) script_isignore, NULL, 1, "s", "nick!user@host", SCRIPT_INTEGER, 0},
	{"", "newignore", (Function) script_newignore, NULL, 3, "sssi", "hostmask creator comment ?minutes?", SCRIPT_INTEGER, SCRIPT_PASS_COUNT | SCRIPT_VAR_ARGS},
	{"", "killignore", (Function) script_killignore, NULL, 1, "s", "hostmask", SCRIPT_INTEGER, 0},
	{"", "ignorelist", (Function) script_ignorelist, NULL, 0, "", "", 0, SCRIPT_PASS_RETVAL},
	{0}
};

tcl_cmds tcluser_cmds[] =
{
  {"getuser",		tcl_getuser},
  {"setuser",		tcl_setuser},
  {NULL,		NULL}
};
