/*
 * chanprog.c -- handles:
 *   telling the current programmed settings
 *   initializing a lot of stuff and loading the tcl scripts
 *
 * $Id: chanprog.c,v 1.42 2002/02/04 07:27:02 wcc Exp $
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
#if HAVE_GETRUSAGE
#include <sys/resource.h>
#if HAVE_SYS_RUSAGE_H
#include <sys/rusage.h>
#endif
#endif
#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif
#include "modules.h"
#include "logfile.h"
#include "misc.h"

extern struct userrec	*userlist;
extern Tcl_Interp	*interp;
extern char		 ver[], botnetnick[], firewall[],
			 motdfile[], userfile[], helpdir[], tempdir[],
			 moddir[], notify_new[], owner[], configfile[];
extern time_t		 now, online_since;
extern int		 backgrd, term_z, con_chan, cache_hit, cache_miss,
			 firewallport, default_flags, conmask,
			 protect_readonly, make_userfile, noshare,
			 ignore_time;

struct chanset_t *chanset = NULL;	/* Channel list			*/
char		  admin[121] = "";	/* Admin info			*/
char		  origbotname[NICKLEN + 1];
char		  botname[NICKLEN + 1];	/* Primary botname		*/


/* Returns memberfields if the nick is in the member list.
 */
memberlist *ismember(struct chanset_t *chan, char *nick)
{
  register memberlist	*x;

  for (x = chan->channel.member; x && x->nick[0]; x = x->next)
    if (!irccmp(x->nick, nick))
      return x;
  return NULL;
}

/* Find a chanset by channel name as the server knows it (ie !ABCDEchannel)
 */
struct chanset_t *findchan(const char *name)
{
  register struct chanset_t	*chan;

  for (chan = chanset; chan; chan = chan->next)
    if (!irccmp(chan->name, name))
      return chan;
  return NULL;
}

/* Find a chanset by display name (ie !channel)
 */
struct chanset_t *findchan_by_dname(const char *name)
{
  register struct chanset_t	*chan;

  for (chan = chanset; chan; chan = chan->next)
    if (!irccmp(chan->dname, name))
      return chan;
  return NULL;
}

/*
 *    "caching" functions
 */

/* Shortcut for get_user_by_host -- might have user record in one
 * of the channel caches.
 */
struct userrec *check_chanlist(const char *host)
{
  char				*nick, *uhost, buf[UHOSTLEN];
  register memberlist		*m;
  register struct chanset_t	*chan;

  strncpyz(buf, host, sizeof buf);
  nick = strtok(buf, "!");
  uhost = strtok(NULL, "!");
  for (chan = chanset; chan; chan = chan->next)
    for (m = chan->channel.member; m && m->nick[0]; m = m->next)
      if (!irccmp(nick, m->nick) && !strcasecmp(uhost, m->userhost))
	return m->user;
  return NULL;
}

/* Shortcut for get_user_by_handle -- might have user record in channels
 */
struct userrec *check_chanlist_hand(const char *hand)
{
  register struct chanset_t	*chan;
  register memberlist		*m;

  for (chan = chanset; chan; chan = chan->next)
    for (m = chan->channel.member; m && m->nick[0]; m = m->next)
      if (m->user && !strcasecmp(m->user->handle, hand))
	return m->user;
  return NULL;
}

/* Clear the user pointers in the chanlists.
 *
 * Necessary when a hostmask is added/removed, a user is added or a new
 * userfile is loaded.
 */
void clear_chanlist(void)
{
  register memberlist		*m;
  register struct chanset_t	*chan;

  for (chan = chanset; chan; chan = chan->next)
    for (m = chan->channel.member; m && m->nick[0]; m = m->next)
      m->user = NULL;
}

/* Clear the user pointer of a specific nick in the chanlists.
 *
 * Necessary when a hostmask is added/removed, a nick changes, etc.
 * Does not completely invalidate the channel cache like clear_chanlist().
 */
void clear_chanlist_member(const char *nick)
{
  register memberlist		*m;
  register struct chanset_t	*chan;

  for (chan = chanset; chan; chan = chan->next)
    for (m = chan->channel.member; m && m->nick[0]; m = m->next)
      if (!irccmp(m->nick, nick)) {
	m->user = NULL;
	break;
      }
}

/* If this user@host is in a channel, set it (it was null)
 */
void set_chanlist(const char *host, struct userrec *rec)
{
  char				*nick, *uhost, buf[UHOSTLEN];
  register memberlist		*m;
  register struct chanset_t	*chan;

  strncpyz(buf, host, sizeof buf);
  nick = strtok(buf, "!");
  uhost = strtok(NULL, "!");
  for (chan = chanset; chan; chan = chan->next)
    for (m = chan->channel.member; m && m->nick[0]; m = m->next)
      if (!irccmp(nick, m->nick) && !strcasecmp(uhost, m->userhost))
	m->user = rec;
}

/* Dump status info out to dcc
 */
void tell_verbose_status(int idx)
{
  char s[256], s1[121], s2[81];
  char *vers_t, *uni_t;
  int i;
  time_t now2, hr, min;
#if HAVE_GETRUSAGE
  struct rusage ru;
#else
# if HAVE_CLOCK
  clock_t cl;
# endif
#endif
#ifdef HAVE_UNAME
  struct utsname un;

  if (!uname(&un) < 0) {
#endif
    vers_t = " ";
    uni_t = _("*unknown*");
#ifdef HAVE_UNAME
  } else {
    vers_t = un.release;
    uni_t = un.sysname;
  }
#endif

  i = count_users(userlist);
  /* FIXME PLURAL: handle it properly */
  dprintf(idx, _("I am %1$s, running %2$s:  %3$d user%4$s\n"),
	  botnetnick, ver, i, i == 1 ? "" : "s");
  dprintf(idx, _("Running on %1$s %2$s\n"), uni_t, vers_t);
  if (admin[0])
    dprintf(idx, _("Admin: %s\n"), admin);
  now2 = now - online_since;
  s[0] = 0;
  if (now2 > 86400) {
    /* days */
    /* FIXME PLURAL: handle it properly */
    sprintf(s, _("%d day"), (int) (now2 / 86400));
    if ((int) (now2 / 86400) >= 2)
      strcat(s, "s");
    strcat(s, ", ");
    now2 -= (((int) (now2 / 86400)) * 86400);
  }
  hr = (time_t) ((int) now2 / 3600);
  now2 -= (hr * 3600);
  min = (time_t) ((int) now2 / 60);
  sprintf(&s[strlen(s)], "%02d:%02d", (int) hr, (int) min);
  s1[0] = 0;
  if (backgrd)
    strcpy(s1, _("background"));
  else {
    if (term_z)
      strcpy(s1, _("terminal mode"));
    else if (con_chan)
      strcpy(s1, _("status mode"));
    else
      strcpy(s1, _("log dump mode"));
  }
#if HAVE_GETRUSAGE
  getrusage(RUSAGE_SELF, &ru);
  hr = (int) ((ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) / 60);
  min = (int) ((ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) - (hr * 60));
  sprintf(s2, "CPU %02d:%02d", (int) hr, (int) min);	/* Actally min/sec */
#else
# if HAVE_CLOCK
  cl = (clock() / CLOCKS_PER_SEC);
  hr = (int) (cl / 60);
  min = (int) (cl - (hr * 60));
  sprintf(s2, "CPU %02d:%02d", (int) hr, (int) min);	/* Actually min/sec */
# else
  sprintf(s2, "CPU ???");
# endif
#endif
  dprintf(idx, "%s %s  (%s)  %s  %s %4.1f%%\n", _("Online for"),
	  s, s1, s2, _("cache hit"),
	  100.0 * ((float) cache_hit) / ((float) (cache_hit + cache_miss)));

  /* info library */
  dprintf(idx, "%s %s\n", _("Tcl library:"),
	  ((interp) && (Tcl_Eval(interp, "info library") == TCL_OK)) ?
	  interp->result : _("*unknown*"));

  /* info tclversion/patchlevel */
  dprintf(idx, "%s %s (%s %s)\n", _("Tcl version:"),
	  ((interp) && (Tcl_Eval(interp, "info patchlevel") == TCL_OK)) ?
	  interp->result : (Tcl_Eval(interp, "info tclversion") == TCL_OK) ?
	  interp->result : _("*unknown*"), _("header version"),
	  TCL_PATCH_LEVEL ? TCL_PATCH_LEVEL : _("*unknown*"));

#if HAVE_TCL_THREADS
  dprintf(idx, _("Tcl is threaded\n"));
#endif  
	  
}

/* Show all internal state variables
 */
void tell_settings(int idx)
{
  char s[1024];
  struct flag_record fr = {FR_GLOBAL, 0, 0, 0, 0, 0};

  dprintf(idx, _("Botnet Nickname: %s\n"), botnetnick);
  if (firewall[0])
    dprintf(idx, _("Firewall: %1$s, port %2$d\n"), firewall, firewallport);
  dprintf(idx, _("Userfile: %1$s   Motd: %2$s\n"), userfile, motdfile);
  dprintf(idx, _("Directories:\n"));
  dprintf(idx, _("  Help    : %s\n"), helpdir);
  dprintf(idx, _("  Temp    : %s\n"), tempdir);
#ifndef STATIC
  dprintf(idx, _("  Modules : %s\n"), moddir);
#endif
  fr.global = default_flags;

  build_flags(s, &fr, NULL);
  dprintf(idx, "%s [%s], %s: %s\n", _("New users get flags"), s,
	  _("notify"), notify_new);
  /* FIXME PLURAL: handle it properly */
  if (owner[0])
    dprintf(idx, "%s: %s\n", _("Permanent owner(s)"), owner);
  dprintf(idx, _("Ignores last %d mins\n"), ignore_time);
}

void reaffirm_owners()
{
  char *p, *q, s[121];
  struct userrec *u;

  /* Make sure default owners are +n */
  if (owner[0]) {
    q = owner;
    p = strchr(q, ',');
    while (p) {
      strncpyz(s, q, p - q);
      rmspace(s);
      u = get_user_by_handle(userlist, s);
      if (u)
	u->flags = sanity_check(u->flags | USER_OWNER);
      q = p + 1;
      p = strchr(q, ',');
    }
    strcpy(s, q);
    rmspace(s);
    u = get_user_by_handle(userlist, s);
    if (u)
      u->flags = sanity_check(u->flags | USER_OWNER);
  }
}

void chanprog()
{
  admin[0] = 0;
  helpdir[0] = 0;
  tempdir[0] = 0;
  conmask = 0;
  /* Turn off read-only variables (make them write-able) for rehash */
  protect_readonly = 0;
  /* Now read it */
  if (!readtclprog(configfile))
    fatal(_("CONFIG FILE NOT LOADED (NOT FOUND, OR ERROR)"), 0);
  /* We should be safe now */
  call_hook(HOOK_REHASH);
  protect_readonly = 1;
  if (!botnetnick[0]) {
    strncpyz(botnetnick, origbotname, HANDLEN + 1);
  }
  if (!botnetnick[0])
    fatal("I don't have a botnet nick!!\n", 0);
  if (!userfile[0])
    fatal(_("STARTING BOT IN USERFILE CREATION MODE.\n\
Telnet to the bot and enter 'NEW' as your nickname."), 0);
  if (!readuserfile(userfile, &userlist)) {
    if (!make_userfile) {
      char tmp[178];

      snprintf(tmp, sizeof tmp, _("USER FILE NOT FOUND!  (try ./eggdrop -m %s to make one)\n"), configfile);
      fatal(tmp, 0);
    }
    printf(_("\n\nSTARTING BOT IN USERFILE CREATION MODE.\n\
Telnet to the bot and enter 'NEW' as your nickname.\n"));
    if (module_find("server", 0, 0))
      printf(_("OR go to IRC and type:  /msg %s hello\n"), origbotname);
    printf(_("This will make the bot recognize you as the master.\n\n"));
  } else if (make_userfile) {
     make_userfile = 0;
     printf(_("USERFILE ALREADY EXISTS (drop the -m)\n"));
  }
  if (helpdir[0])
    if (helpdir[strlen(helpdir) - 1] != '/')
      strcat(helpdir, "/");
  if (tempdir[0])
    if (tempdir[strlen(tempdir) - 1] != '/')
      strcat(tempdir, "/");
  /* Test tempdir: it's vital */
  {
    FILE *f;
    char s[161], rands[8];

    /* Possible file race condition solved by using a random string
     * and the process id in the filename.
     * FIXME: This race is only partitially fixed. We could still be
     *        overwriting an existing file / following a malicious
     *        link.
     */
    make_rand_str(rands, 7); /* create random string */
    sprintf(s, "%s.test-%u-%s", tempdir, getpid(), rands);
    f = fopen(s, "w");
    if (f == NULL)
      fatal(_("CANT WRITE TO TEMP DIR"), 0);
    fclose(f);
    unlink(s);
  }
  reaffirm_owners();
}

/* Reload the user file from disk
 */
void reload()
{
  FILE *f;

  f = fopen(userfile, "r");
  if (f == NULL) {
    putlog(LOG_MISC, "*", _("Cant reload user file!"));
    return;
  }
  fclose(f);
  noshare = 1;
  clear_userlist(userlist);
  noshare = 0;
  userlist = NULL;
  if (!readuserfile(userfile, &userlist))
    fatal(_("User file is missing!"), 0);
  reaffirm_owners();
  call_hook(HOOK_READ_USERFILE);
}

void rehash()
{
  call_hook(HOOK_PRE_REHASH);
  noshare = 1;
  clear_userlist(userlist);
  noshare = 0;
  userlist = NULL;
  chanprog();
}

/* Oddly enough, written by proton (Emech's coder)
 */
int isowner(char *name)
{
  char *pa, *pb;
  char nl, pl;

  if (!owner || !*owner)
    return (0);
  if (!name || !*name)
    return (0);
  nl = strlen(name);
  pa = owner;
  pb = owner;
  while (1) {
    while (1) {
      if ((*pb == 0) || (*pb == ',') || (*pb == ' '))
	break;
      pb++;
    }
    pl = (unsigned int) pb - (unsigned int) pa;
    if (pl == nl && !strncasecmp(pa, name, nl))
      return (1);
    while (1) {
      if ((*pb == 0) || ((*pb != ',') && (*pb != ' ')))
	break;
      pb++;
    }
    if (*pb == 0)
      return (0);
    pa = pb;
  }
}
