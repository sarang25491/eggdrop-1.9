/*
 * cmds.c --
 *
 *	commands from a user via dcc
 *	(split in 2, this portion contains no-irc commands)
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
static const char rcsid[] = "$Id: cmds.c,v 1.117 2003/03/06 12:08:15 tothwolf Exp $";
#endif

#include "main.h"
#include "modules.h"
#include "logfile.h"
#include "misc.h"
#include "cmdt.h"		/* cmd_t				*/
#include "core_binds.h"		/* check_bind_act, check_bind_chpt, 
				   check_bind_chjn, check_bind_chof	*/
#include "users.h"		/* get_user_by_host, set_user,
				   USERENTRY_PASS			*/
#include "chanprog.h"		/* masktype, tell_verbose_status, 
				   tell_settings, maskname, logmodes
				   isowner, reload			*/
#include "dccutil.h"		/* dprintf_eggdrop, not_away, set_away
				   tell_dcc, do_boot, chanout_but,
				   dcc_chatter, flush_lines		*/
#include "net.h"		/* tell_netdebug, killsock		*/
#include "cmds.h"		/* prototypes				*/

#include <ctype.h>
#include "traffic.h" /* egg_traffic_t */

extern struct chanset_t	*chanset;
extern struct dcc_t	*dcc;
extern user_t *userlist;
extern int dcc_total, backgrd, make_userfile, do_restart, conmask, strict_host,
           term_z, con_chan;
extern egg_traffic_t traffic;
extern char myname[], ver[], network[], owner[], spaces[], quit_msg[];
extern time_t now, online_since;
extern module_entry *module_list;

#ifndef MAKING_MODS
extern struct dcc_table DCC_CHAT, DCC_BOT, DCC_RELAY, DCC_FORK_BOT,
			DCC_CHAT_PASS; 
#endif /* MAKING_MODS   */

static char	*btos(unsigned long);


static void tell_who(user_t *u, int idx, int chan)
{
  int i, k, ok = 0, atr = u ? u->flags : 0, len;
  char s[1024];			/* temp fix - 1.4 has a better one */

  if (!chan)
    dprintf(idx, "Party line members:  (* = owner, + = master, @ = op)\n");
  else {
    dprintf(idx,
	    "People on channel %s%d:  (* = owner, + = master, @ = op)\n",
	      (chan < 100000) ? "" : "*", chan % 100000);
  }
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_CHAT)
      if (dcc[i].u.chat->channel == chan) {
	spaces[len = HANDLEN - strlen(dcc[i].nick)] = 0;
	if (atr & USER_OWNER) {
	  sprintf(s, "  [%.2lu]  %c%s%s %s", dcc[i].sock,
		  (geticon(dcc[i].user) == '-' ? ' ' : geticon(dcc[i].user)),
		  dcc[i].nick, spaces, dcc[i].host);
	} else {
	  sprintf(s, "  %c%s%s %s",
		  (geticon(dcc[i].user) == '-' ? ' ' : geticon(dcc[i].user)),
		  dcc[i].nick, spaces, dcc[i].host);
	}
	spaces[len] = ' ';
	if (atr & USER_MASTER) {
	  if (dcc[i].u.chat->con_flags)
	    sprintf(&s[strlen(s)], " (con:%s)",
		    masktype(dcc[i].u.chat->con_flags));
	}
	if (now - dcc[i].timeval > 300) {
	  unsigned long days, hrs, mins;

	  days = (now - dcc[i].timeval) / 86400;
	  hrs = ((now - dcc[i].timeval) - (days * 86400)) / 3600;
	  mins = ((now - dcc[i].timeval) - (hrs * 3600)) / 60;
	  if (days > 0)
	    sprintf(&s[strlen(s)], " (idle %lud%luh)", days, hrs);
	  else if (hrs > 0)
	    sprintf(&s[strlen(s)], " (idle %luh%lum)", hrs, mins);
	  else
	    sprintf(&s[strlen(s)], " (idle %lum)", mins);
	}
	dprintf(idx, "%s\n", s);
	if (dcc[i].u.chat->away != NULL)
	  dprintf(idx, "      AWAY: %s\n", dcc[i].u.chat->away);
      }
  ok = 0;
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type == &DCC_CHAT) && (dcc[i].u.chat->channel != chan)) {
      if (!ok) {
	ok = 1;
	dprintf(idx, _("Other people on the bot:\n"));
      }
      spaces[len = HANDLEN - strlen(dcc[i].nick)] = 0;
      if (atr & USER_OWNER) {
	sprintf(s, "  [%.2lu]  %c%s%s ",
		dcc[i].sock,
		(geticon(dcc[i].user) == '-' ? ' ' : geticon(dcc[i].user)),
		dcc[i].nick, spaces);
      } else {
	sprintf(s, "  %c%s%s ",
		(geticon(dcc[i].user) == '-' ? ' ' : geticon(dcc[i].user)),
		dcc[i].nick, spaces);
      }
      spaces[len] = ' ';
      if (atr & USER_MASTER) {
	if (dcc[i].u.chat->channel < 0)
	  strcat(s, "(-OFF-) ");
	else if (!dcc[i].u.chat->channel)
	  strcat(s, "(party) ");
	else
	  sprintf(&s[strlen(s)], "(%5d) ", dcc[i].u.chat->channel);
      }
      strcat(s, dcc[i].host);
      if (atr & USER_MASTER) {
	if (dcc[i].u.chat->con_flags)
	  sprintf(&s[strlen(s)], " (con:%s)",
		  masktype(dcc[i].u.chat->con_flags));
      }
      if (now - dcc[i].timeval > 300) {
	k = (now - dcc[i].timeval) / 60;
	if (k < 60)
	  sprintf(&s[strlen(s)], " (idle %dm)", k);
	else
	  sprintf(&s[strlen(s)], " (idle %dh%dm)", k / 60, k % 60);
      }
      dprintf(idx, "%s\n", s);
      if (dcc[i].u.chat->away != NULL)
	dprintf(idx, "      AWAY: %s\n", dcc[i].u.chat->away);
    }
    if ((atr & USER_MASTER) && dcc[i].type && (dcc[i].type->flags & DCT_SHOWWHO) &&
	(dcc[i].type != &DCC_CHAT)) {
      if (!ok) {
	ok = 1;
	dprintf(idx, _("Other people on the bot:\n"));
      }
      spaces[len = HANDLEN - strlen(dcc[i].nick)] = 0;
      if (atr & USER_OWNER) {
	sprintf(s, "  [%.2lu]  %c%s%s (files) %s",
		dcc[i].sock, dcc[i].status & STAT_CHAT ? '+' : ' ',
		dcc[i].nick, spaces, dcc[i].host);
      } else {
	sprintf(s, "  %c%s%s (files) %s",
		dcc[i].status & STAT_CHAT ? '+' : ' ',
		dcc[i].nick, spaces, dcc[i].host);
      }
      spaces[len] = ' ';
      dprintf(idx, "%s\n", s);
    }
  }
}

static int cmd_me(user_t *u, int idx, char *par)
{
  int i;

  if (dcc[idx].u.chat->channel < 0) {
    dprintf(idx, _("You have chat turned off.\n"));
    return(0);
  }
  if (!par[0]) {
    dprintf(idx, "Usage: me <action>\n");
    return(0);
  }
  if (dcc[idx].u.chat->away != NULL)
    not_away(idx);
  for (i = 0; i < dcc_total; i++)
  check_bind_act(dcc[idx].nick, dcc[idx].u.chat->channel, par);
  return(0);
}

static int cmd_motd(user_t *u, int idx, char *par)
{
    show_motd(idx);
  return(1);
}

static int cmd_away(user_t *u, int idx, char *par)
{
  if (strlen(par) > 60)
    par[60] = 0;
  set_away(idx, par);
  return(1);
}

static int cmd_back(user_t *u, int idx, char *par)
{
  not_away(idx);
  return(1);
}

static int cmd_newpass(user_t *u, int idx, char *par)
{
  char *new;

  if (!par[0]) {
    dprintf(idx, "Usage: newpass <newpassword>\n");
    return(0);
  }
  new = newsplit(&par);
  if (strlen(new) > 16)
    new[16] = 0;
  if (strlen(new) < 6) {
    dprintf(idx, _("Please use at least 6 characters.\n"));
    return(0);
  }
  set_user(&USERENTRY_PASS, u, new);
  putlog(LOG_CMDS, "*", "#%s# newpass ...", dcc[idx].nick);
  dprintf(idx, _("Changed password to '%s'\n"), new);
  return(0);
}

static int cmd_rehelp(user_t *u, int idx, char *par)
{
  dprintf(idx, _("Reload help cache...\n"));
  reload_help_data();
  return(1);
}

static int cmd_help(user_t *u, int idx, char *par)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
  if (par[0]) {
    if (!strcmp(par, "all"))
      tellallhelp(idx, "all", &fr);
    else if (strchr(par, '*') || strchr(par, '?')) {
      char *p = par;

      /* Check if the search pattern only consists of '*' and/or '?'
       * If it does, show help for "all" instead of listing all help
       * entries.
       */
      for (p = par; *p && ((*p == '*') || (*p == '?')); p++);
      if (*p)
        tellwildhelp(idx, par, &fr);
      else
	tellallhelp(idx, "all", &fr);
    } else
      tellhelp(idx, par, &fr, 0);
  } else {
    if (glob_op(fr) || glob_botmast(fr) || chan_op(fr))
      tellhelp(idx, "help", &fr, 0);
    else
      tellhelp(idx, "helpparty", &fr, 0);
  }
  return(1);
}

static int cmd_who(user_t *u, int idx, char *par)
{
    putlog(LOG_CMDS, "*", "#%s# who", dcc[idx].nick);
    if (dcc[idx].u.chat->channel < 0)
      tell_who(u, idx, 0);
    else
      tell_who(u, idx, dcc[idx].u.chat->channel);

  return(1);
}

static int cmd_whois(user_t *u, int idx, char *par)
{
  if (!par[0]) {
    dprintf(idx, "Usage: whois <handle>\n");
    return(0);
  }
  tell_user_ident(idx, par, u ? (u->flags & USER_MASTER) : 0);
  return(1);
}

static int cmd_match(user_t *u, int idx, char *par)
{
  int start = 1, limit = 20;
  char *s, *s1, *chname;

  if (!par[0]) {
    dprintf(idx, "Usage: match <nick/host> [[skip] count]\n");
    return(0);
  }
  s = newsplit(&par);
  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = "";
  if (atoi(par) > 0) {
    s1 = newsplit(&par);
    if (atoi(par) > 0) {
      start = atoi(s1);
      limit = atoi(par);
    } else
      limit = atoi(s1);
  }
  tell_users_match(idx, s, start, limit, u ? (u->flags & USER_MASTER) : 0,
		   chname);
  return(1);
}

static int cmd_uptime(user_t *u, int idx, char *par)
{
  char s[256];
  unsigned long uptime, tmp, hr, min; 

  uptime = now - online_since;
  s[0] = 0;
  if (uptime > 86400) {
    tmp = (uptime / 86400);
    sprintf(s, "%lu day%s, ", tmp, (tmp == 1) ? "" : "s");
    uptime -= (tmp * 86400);
  }
  hr = (uptime / 3600);
  uptime -= (hr * 3600);
  min = (uptime / 60);
  sprintf(&s[strlen(s)], "%02lu:%02lu", hr, min);

  dprintf(idx, "%s %s  (%s)\n", _("Online for"), s, backgrd ?
	  _("background") : term_z ? _("terminal mode") : con_chan ?
	  _("status mode") : _("log dump mode"));
  return(1);
}

static int cmd_status(user_t *u, int idx, char *par)
{
  int atr = u ? u->flags : 0;

  if (!strcasecmp(par, "all")) {
    if (!(atr & USER_MASTER)) {
      dprintf(idx, _("You do not have Bot Master privileges.\n"));
      return(0);
    }
    tell_verbose_status(idx);
    dprintf(idx, "\n");
    tell_settings(idx);
    do_module_report(idx, 1, NULL);
  } else {
    tell_verbose_status(idx);
    do_module_report(idx, 0, NULL);
  }
  return(1);
}

static int cmd_dccstat(user_t *u, int idx, char *par)
{
  tell_dcc(idx);
  return(1);
}

static int cmd_boot(user_t *u, int idx, char *par)
{
  int i, files = 0, ok = 0;
  char *who;
  user_t *u2;

  if (!par[0]) {
    dprintf(idx, "Usage: boot nick[@bot]\n");
    return(0);
  }
  who = newsplit(&par);
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type && !strcasecmp(dcc[i].nick, who)
        && !ok && (dcc[i].type->flags & DCT_CANBOOT)) {
      u2 = get_user_by_handle(userlist, dcc[i].nick);
      if (u2 && (u2->flags & USER_OWNER)
          && strcasecmp(dcc[idx].nick, who)) {
        dprintf(idx, _("You can't boot a bot owner.\n"));
        return(0);
      }
      if (u2 && (u2->flags & USER_MASTER) && !(u && (u->flags & USER_MASTER))) {
        dprintf(idx, _("You can't boot a bot master.\n"));
        return(0);
      }
      files = (dcc[i].type->flags & DCT_FILES);
      if (files)
        dprintf(idx, _("Booted %s from the file area.\n"), dcc[i].nick);
      else
        dprintf(idx, _("Booted %s from the party line.\n"), dcc[i].nick);
      do_boot(i, dcc[idx].nick, par);
      ok = 1;
    }
  if (!ok) {
    dprintf(idx, _("Who?  No such person on the party line.\n"));
    return(0);
  }
  return(1);
}

static int cmd_console(user_t *u, int idx, char *par)
{
  char *nick, s[2], s1[512];
  int dest = 0, i, ok = 0, pls, md;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  module_entry *me;

  if (!par[0]) {
    dprintf(idx, "Your console is %s: %s (%s).\n",
	    dcc[idx].u.chat->con_chan,
	    masktype(dcc[idx].u.chat->con_flags),
	    maskname(dcc[idx].u.chat->con_flags));
    return(1);
  }
  get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
  strcpy(s1, par);
  nick = newsplit(&par);
  /* Don't remove '+' as someone couldn't have '+' in CHANMETA cause
   * he doesn't use IRCnet ++rtc.
   */
  if (nick[0] && !strchr(CHANMETA "+-*", nick[0]) && glob_master(fr)) {
    for (i = 0; i < dcc_total; i++)
      if (dcc[i].type && !strcasecmp(nick, dcc[i].nick) &&
	  (dcc[i].type == &DCC_CHAT) && (!ok)) {
	ok = 1;
	dest = i;
      }
    if (!ok) {
      dprintf(idx, "No such user on the party line!\n");
      return(0);
    }
    nick[0] = 0;
  } else
    dest = idx;
  if (!nick[0])
    nick = newsplit(&par);
  /* Consider modeless channels, starting with '+' */
  if ((nick [0] == '+' && findchan_by_dname(nick)) ||
      (nick [0] != '+' && strchr(CHANMETA "*", nick[0]))) {
    if (strcmp(nick, "*") && !findchan_by_dname(nick)) {
      dprintf(idx, "Invalid console channel: %s.\n", nick);
      return(0);
    }
    get_user_flagrec(u, &fr, nick);
    if (!chan_op(fr) && !glob_op(fr)) {
      dprintf(idx, "You don't have op or master access to channel %s.\n",
	      nick);
      return(0);
    }
    strlcpy(dcc[dest].u.chat->con_chan, nick, sizeof dcc[dest].u.chat->con_chan);
    nick[0] = 0;
    if ((dest == idx) && !glob_master(fr) && !chan_master(fr))
      /* Consoling to another channel for self */
      dcc[dest].u.chat->con_flags &= ~(LOG_MISC | LOG_CMDS | LOG_WALL);
  }
  if (!nick[0])
    nick = newsplit(&par);
  pls = 1;
  if (nick[0]) {
    if ((nick[0] != '+') && (nick[0] != '-'))
      dcc[dest].u.chat->con_flags = 0;
    for (; *nick; nick++) {
      if (*nick == '+')
	pls = 1;
      else if (*nick == '-')
	pls = 0;
      else {
	s[0] = *nick;
	s[1] = 0;
	md = logmodes(s);
	if ((dest == idx) && !glob_master(fr) && pls) {
	  if (chan_master(fr))
	    md &= ~(LOG_FILES | LOG_LEV1 | LOG_LEV2 | LOG_LEV3 |
		    LOG_LEV4 | LOG_LEV5 | LOG_LEV6 | LOG_LEV7 |
		    LOG_LEV8 | LOG_DEBUG);
	  else
	    md &= ~(LOG_MISC | LOG_CMDS | LOG_FILES | LOG_LEV1 |
		    LOG_LEV2 | LOG_LEV3 | LOG_LEV4 | LOG_LEV5 |
		    LOG_LEV6 | LOG_LEV7 | LOG_LEV8 | LOG_WALL |
		    LOG_DEBUG);
	}
	if (!glob_owner(fr) && pls)
	  md &= ~(LOG_RAW | LOG_SRVOUT);
	if (!glob_botmast(fr) && pls)
	  md &= ~LOG_BOTS;
	if (pls)
	  dcc[dest].u.chat->con_flags |= md;
	else
	  dcc[dest].u.chat->con_flags &= ~md;
      }
    }
  }
  if (dest == idx) {
    dprintf(idx, "Set your console to %s: %s (%s).\n",
	    dcc[idx].u.chat->con_chan,
	    masktype(dcc[idx].u.chat->con_flags),
	    maskname(dcc[idx].u.chat->con_flags));
  } else {
    dprintf(idx, "Set console of %s to %s: %s (%s).\n", dcc[dest].nick,
	    dcc[dest].u.chat->con_chan,
	    masktype(dcc[dest].u.chat->con_flags),
	    maskname(dcc[dest].u.chat->con_flags));
    dprintf(dest, "%s set your console to %s: %s (%s).\n", dcc[idx].nick,
	    dcc[dest].u.chat->con_chan,
	    masktype(dcc[dest].u.chat->con_flags),
	    maskname(dcc[dest].u.chat->con_flags));
  }
  /* New style autosave -- drummer,07/25/1999*/
  if ((me = module_find("console", 1, 1))) {
    Function *func = me->funcs;
    (func[CONSOLE_DOSTORE]) (dest);
  }
  return(1);
}

static int cmd_pls_bot(user_t *u, int idx, char *par)
{
  char *handle, *addr, *p, *q, *host;
  user_t *u1;
  struct bot_addr *bi;
  int addrlen;

  if (!par[0]) {
    dprintf(idx, "Usage: +bot <handle> [address[:telnet-port[/relay-port]]] "
            "[host]\n");
    return(0);
  }

  handle = newsplit(&par);
  addr = newsplit(&par);
  host = newsplit(&par);

  if (strlen(handle) > HANDLEN)
    handle[HANDLEN] = 0;

  if (get_user_by_handle(userlist, handle))
    dprintf(idx, _("Someone already exists by that name.\n"));
    return(0);
  }

  if (strchr(BADHANDCHARS, handle[0]) != NULL) {
    dprintf(idx, _("You can't start a botnick with '%c'.\n"), handle[0]);
    return(0);
  }

  if (strlen(addr) > 60)
    addr[60] = 0;

  userlist = adduser(userlist, handle, "none", "-", USER_BOT);
  u1 = get_user_by_handle(userlist, handle);
  bi = malloc(sizeof(struct bot_addr));

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
    bi->address = strdup(addr);
    bi->telnet_port = 3333;
    bi->relay_port = 3333;
  } else {
    bi->address = malloc(addrlen + 1);
    strlcpy(bi->address, addr, sizeof bi->address);
    p = q + 1;
    bi->telnet_port = atoi(p);
    q = strchr(p, '/');
    if (!q)
      bi->relay_port = bi->telnet_port;
    else
      bi->relay_port = atoi(q + 1);
  }
  set_user(&USERENTRY_BOTADDR, u1, bi);
  dprintf(idx, _("Added bot '%s' with %s%s%s%s and %s%s%s%s.\n"), handle,
          addr[0] ? "address " : "no address ", addr[0] ? "'" : "",
          addr[0] ? addr : "", addr[0] ? "'" : "",
          host[0] ? "hostmask " : "no hostmask", addr[0] ? "'" : "",
          host[0] ? host : "", addr[0] ? "'" : "");
  if (host[0])
    addhost_by_handle(handle, host);
  else if (!add_bot_hostmask(idx, handle))
    dprintf(idx, _("You'll want to add a hostmask if this bot will ever be on any channels that I'm on.\n"));
  return(1);
}

static int cmd_chhandle(user_t *u, int idx, char *par)
{
  char hand[HANDLEN + 1], newhand[HANDLEN + 1];
  int i, atr = u ? u->flags : 0, atr2;
  user_t *u2;

  strlcpy(hand, newsplit(&par), sizeof hand);
  strlcpy(newhand, newsplit(&par), sizeof newhand);

  if (!hand[0] || !newhand[0]) {
    dprintf(idx, "Usage: chhandle <oldhandle> <newhandle>\n");
    return(0);
  }
  for (i = 0; i < strlen(newhand); i++)
    if ((newhand[i] <= 32) || (newhand[i] >= 127) || (newhand[i] == '@'))
      newhand[i] = '?';
  if (strchr(BADHANDCHARS, newhand[0]) != NULL)
    dprintf(idx, _("Bizarre quantum forces prevent nicknames from starting with %c.\n"),
           newhand[0]);
  else if (get_user_by_handle(userlist, newhand) &&
          strcasecmp(hand, newhand))
    dprintf(idx, _("Somebody is already using %s.\n"), newhand);
  else {
    u2 = get_user_by_handle(userlist, hand);
    atr2 = u2 ? u2->flags : 0;
    if ((atr & USER_BOTMAST) && !(atr & USER_MASTER) &&
       !(atr2 & USER_BOT))
      dprintf(idx, _("You can't change handles for non-bots.\n"));
    else if ((atr2 & USER_OWNER) && !(atr & USER_OWNER) &&
            strcasecmp(dcc[idx].nick, hand))
      dprintf(idx, _("You can't change a bot owner's handle.\n"));
    else if (isowner(hand) && strcasecmp(dcc[idx].nick, hand))
      dprintf(idx, _("You can't change a permanent bot owner's handle.\n"));
    else if (!strcasecmp(newhand, myname) && !(atr2 & USER_BOT))
      dprintf(idx, _("Hey! That's MY name!\n"));
    else if (change_handle(u2, newhand)) {
      dprintf(idx, _("Changed.\n"));
    } else
      dprintf(idx, _("Failed.\n"));
  }
  return(1);
}

static int cmd_handle(user_t *u, int idx, char *par)
{
  char oldhandle[HANDLEN + 1], newhandle[HANDLEN + 1];
  int i;

  strlcpy(newhandle, newsplit(&par), sizeof newhandle);

  if (!newhandle[0]) {
    dprintf(idx, "Usage: handle <new-handle>\n");
    return(0);
  }
  for (i = 0; i < strlen(newhandle); i++)
    if ((newhandle[i] <= 32) || (newhandle[i] >= 127) || (newhandle[i] == '@'))
      newhandle[i] = '?';
  if (strchr(BADHANDCHARS, newhandle[0]) != NULL) {
    dprintf(idx, _("Bizarre quantum forces prevent handle from starting with '%c'\n"),
	    newhandle[0]);
  } else if (get_user_by_handle(userlist, newhandle) &&
	     strcasecmp(dcc[idx].nick, newhandle)) {
    dprintf(idx, _("Somebody is already using %s.\n"), newhandle);
  } else if (!strcasecmp(newhandle, myname)) {
    dprintf(idx, _("Hey!  That's MY name!\n"));
  } else {
    strlcpy(oldhandle, dcc[idx].nick, sizeof oldhandle);
    if (change_handle(u, newhandle)) {
      dprintf(idx, _("Okay, changed.\n"));
    } else
      dprintf(idx, _("Failed.\n"));
  }
  return(1);
}

static int cmd_chpass(user_t *u, int idx, char *par)
{
  char *handle, *new;
  int atr = u ? u->flags : 0, l;

  if (!par[0])
    dprintf(idx, "Usage: chpass <handle> [password]\n");
  else {
    handle = newsplit(&par);
    u = get_user_by_handle(userlist, handle);
    if (!u)
      dprintf(idx, _("No such user.\n"));
    else if ((atr & USER_BOTMAST) && !(atr & USER_MASTER) &&
	     !(u->flags & USER_BOT))
      dprintf(idx, _("You can't change passwords for non-bots.\n"));
    else if ((u->flags & USER_OWNER) && !(atr & USER_OWNER) &&
	     strcasecmp(handle, dcc[idx].nick))
      dprintf(idx, _("You can't change a bot owner's password.\n"));
    else if (isowner(handle) && strcasecmp(dcc[idx].nick, handle))
      dprintf(idx, _("You can't change a permanent bot owner's password.\n"));
    else if (!par[0]) {
      set_user(&USERENTRY_PASS, u, NULL);
      dprintf(idx, _("Removed password.\n"));
    } else {
      l = strlen(new = newsplit(&par));
      if (l > 16)
	new[16] = 0;
      if (l < 6)
	dprintf(idx, _("Please use at least 6 characters.\n"));
      else {
	set_user(&USERENTRY_PASS, u, new);
	dprintf(idx, _("Changed password.\n"));
      }
    }
  }
  return(0);
}

static int cmd_chaddr(user_t *u, int idx, char *par)
{
  int telnet_port = 3333, relay_port = 3333;
  char *handle, *addr, *p, *q;
  struct bot_addr *bi;
  user_t *u1;
  int addrlen;

  handle = newsplit(&par);
  if (!par[0]) {
    dprintf(idx, "Usage: chaddr <botname> <address[:telnet-port[/relay-port]]>\n");
    return(0);
  }
  addr = newsplit(&par);
  if (strlen(addr) > UHOSTMAX)
    addr[UHOSTMAX] = 0;
  u1 = get_user_by_handle(userlist, handle);
  if (!u1 || !(u1->flags & USER_BOT)) {
    dprintf(idx, _("This command is only useful for tandem bots.\n"));
    return(0);
  }
  dprintf(idx, _("Changed bot's address.\n"));

  bi = (struct bot_addr *) get_user(&USERENTRY_BOTADDR, u1);
  if (bi) {
    telnet_port = bi->telnet_port;
    relay_port = bi->relay_port;
  }

  bi = malloc(sizeof(struct bot_addr));

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
    bi->address = strdup(addr);
    bi->telnet_port = telnet_port;
    bi->relay_port = relay_port;
  } else {
    bi->address = malloc(addrlen + 1);
    strlcpy(bi->address, addr, sizeof bi->address);
    p = q + 1;
    bi->telnet_port = atoi(p);
    q = strchr(p, '/');
    if (!q) {
      bi->relay_port = bi->telnet_port;
    } else {
      bi->relay_port = atoi(q + 1);
    }
  }
  set_user(&USERENTRY_BOTADDR, u1, bi);
  return(1);
}

static int cmd_comment(user_t *u, int idx, char *par)
{
  char *handle;
  user_t *u1;

  handle = newsplit(&par);
  if (!par[0]) {
    dprintf(idx, "Usage: comment <handle> <newcomment>\n");
    return(0);
  }
  u1 = get_user_by_handle(userlist, handle);
  if (!u1) {
    dprintf(idx, _("No such user!\n"));
    return(0);
  }
  if ((u1->flags & USER_OWNER) && !(u && (u->flags & USER_OWNER)) &&
      strcasecmp(handle, dcc[idx].nick)) {
    dprintf(idx, _("Can't change comment on the bot owner.\n"));
    return(0);
  }
  if (!strcasecmp(par, "none")) {
    dprintf(idx, _("Okay, comment blanked.\n"));
    set_user(&USERENTRY_COMMENT, u1, NULL);
    return(0);
  }
  dprintf(idx, _("Changed comment.\n"));
  set_user(&USERENTRY_COMMENT, u1, par);
  return(1);
}

static int cmd_restart(user_t *u, int idx, char *par)
{
  if (!backgrd) {
    dprintf(idx, "%s\n", _("You cannot .restart a bot when running -n (due to tcl)."));
    return(0);
  }
  dprintf(idx, _("Restarting.\n"));
  if (make_userfile)
    make_userfile = 0;
  write_userfile(-1);
  putlog(LOG_MISC, "*", "%s", _("Restarting ..."));
  do_restart = idx;
  return(1);
}

static int cmd_rehash(user_t *u, int idx, char *par)
{
  dprintf(idx, "%s\n", _("Rehashing."));
  if (make_userfile)
    make_userfile = 0;
  write_userfile(-1);
  putlog(LOG_MISC, "*", "%s", _("Rehashing..."));
  do_restart = -2;
  return(1);
}

static int cmd_reload(user_t *u, int idx, char *par)
{
  dprintf(idx, "%s\n", _("Reloading user file..."));
  reload();
  return(1);
}

void cmd_die(user_t *u, int idx, char *par)
{
  char s1[1024], s2[1024];

  if (par[0]) {
    snprintf(s1, sizeof s1, "%s (%s: %s)", _("BOT SHUTDOWN"), dcc[idx].nick,
		 par);
    snprintf(s2, sizeof s2, "%s %s!%s (%s)", _("DIE BY"), dcc[idx].nick, 
		 dcc[idx].host, par);
    strlcpy(quit_msg, par, sizeof quit_msg);
  } else {
    snprintf(s1, sizeof s1, "%s (%s %s)", _("BOT SHUTDOWN"), _("Authorized by"),
		 dcc[idx].nick);
    snprintf(s2, sizeof s2, "%s %s!%s (%s)", _("DIE BY"), dcc[idx].nick, 
		 dcc[idx].host, _("requested"));
    strlcpy(quit_msg, dcc[idx].nick, sizeof quit_msg);
  }
  kill_bot(s1, s2);
}

static int cmd_simul(user_t *u, int idx, char *par)
{
  char *nick;
  int i, ok = 0;

  nick = newsplit(&par);
  if (!par[0]) {
    dprintf(idx, "Usage: simul <hand> <text>\n");
    return(0);
  }
  if (isowner(nick)) {
    dprintf(idx, _("Unable to '.simul' permanent owners.\n"));
    return(0);
  }
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type && !strcasecmp(nick, dcc[i].nick) && !ok &&
	(dcc[i].type->flags & DCT_SIMUL)) {
      if (dcc[i].type && dcc[i].type->activity) {
	dcc[i].type->activity(i, par, strlen(par));
	ok = 1;
      }
    }
  if (!ok)
    dprintf(idx, _("No such user on the party line.\n"));
  return(1);
}

static int cmd_save(user_t *u, int idx, char *par)
{
  dprintf(idx, _("Saving user file...\n"));
  write_userfile(-1);
  return(1);
}

static int cmd_backup(user_t *u, int idx, char *par)
{
  dprintf(idx, _("Backing up the channel & user files...\n"));
  call_hook(HOOK_BACKUP);
  return(1);
}

/* After messing with someone's user flags, make sure the dcc-chat flags
 * are set correctly.
 */
int check_dcc_attrs(user_t *u, int oatr)
{
  int i, stat;

  if (!u)
    return 0;
  /* Make sure default owners are +n */
  if (isowner(u->handle)) {
    u->flags = sanity_check(u->flags | USER_OWNER);
  }
  for (i = 0; i < dcc_total; i++) {
    if (dcc[i].type && (dcc[i].type->flags & DCT_MASTER) &&
	(!strcasecmp(u->handle, dcc[i].nick))) {
      stat = dcc[i].status;
      if ((oatr & USER_MASTER) && !(u->flags & USER_MASTER)) {
	struct flag_record fr = {FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

	dcc[i].u.chat->con_flags &= ~(LOG_MISC | LOG_CMDS | LOG_RAW |
				      LOG_FILES | LOG_LEV1 | LOG_LEV2 |
				      LOG_LEV3 | LOG_LEV4 | LOG_LEV5 |
				      LOG_LEV6 | LOG_LEV7 | LOG_LEV8 |
				      LOG_WALL | LOG_DEBUG);
	get_user_flagrec(u, &fr, NULL);
	if (!chan_master(fr))
	  dcc[i].u.chat->con_flags |= (LOG_MISC | LOG_CMDS);
	dprintf(i, "*** POOF! ***\n");
	dprintf(i, _("You are no longer a master on this bot.\n"));
      }
      if (!(oatr & USER_MASTER) && (u->flags & USER_MASTER)) {
	dcc[i].u.chat->con_flags |= conmask;
	dprintf(i, "*** POOF! ***\n");
	dprintf(i, _("You are now a master on this bot.\n"));
      }
      if (!(oatr & USER_BOTMAST) && (u->flags & USER_BOTMAST)) {
	dprintf(i, "### POOF! ###\n");
	dprintf(i, _("You are now a botnet master on this bot.\n"));
      }
      if ((oatr & USER_BOTMAST) && !(u->flags & USER_BOTMAST)) {
	dprintf(i, "### POOF! ###\n");
	dprintf(i, _("You are no longer a botnet master on this bot.\n"));
      }
      if (!(oatr & USER_OWNER) && (u->flags & USER_OWNER)) {
	dprintf(i, "@@@ POOF! @@@\n");
	dprintf(i, _("You are now an OWNER of this bot.\n"));
      }
      if ((oatr & USER_OWNER) && !(u->flags & USER_OWNER)) {
	dprintf(i, "@@@ POOF! @@@\n");
	dprintf(i, _("You are no longer an owner of this bot.\n"));
      }
      if ((stat & STAT_PARTY) && (u->flags & USER_OP))
	stat &= ~STAT_PARTY;
      if (!(stat & STAT_PARTY) && !(u->flags & USER_OP) &&
	  !(u->flags & USER_MASTER))
	stat |= STAT_PARTY;
      if ((stat & STAT_CHAT) && !(u->flags & USER_PARTY) &&
	  !(u->flags & USER_MASTER))
	stat &= ~STAT_CHAT;
      if ((dcc[i].type->flags & DCT_FILES) && !(stat & STAT_CHAT) &&
	  ((u->flags & USER_MASTER) || (u->flags & USER_PARTY)))
	stat |= STAT_CHAT;
      dcc[i].status = stat;
      /* Check if they no longer have access to wherever they are.
       *
       * NOTE: DON'T kick someone off the party line just cuz they lost +p
       *       (pinvite script removes +p after 5 mins automatically)
       */
      if ((dcc[i].type->flags & DCT_FILES) && !(u->flags & USER_XFER) &&
	  !(u->flags & USER_MASTER)) {
	dprintf(i, "-+- POOF! -+-\n");
	dprintf(i, _("You no longer have file area access.\n\n"));
	putlog(LOG_MISC, "*", _("DCC user [%s]%s removed from file system"),
	       dcc[i].nick, dcc[i].host);
	if (dcc[i].status & STAT_CHAT) {
	  struct chat_info *ci;

	  ci = dcc[i].u.file->chat;
	  free(dcc[i].u.file);
	  dcc[i].u.chat = ci;
	  dcc[i].status &= (~STAT_CHAT);
	  dcc[i].type = &DCC_CHAT;
	  if (dcc[i].u.chat->channel >= 0) {
	    chanout_but(-1, dcc[i].u.chat->channel,
			"*** %s has returned.\n", dcc[i].nick);
	  }
	} else {
	  killsock(dcc[i].sock);
	  lostdcc(i);
	}
      }
    }
  }
  return u->flags;
}

int check_dcc_chanattrs(user_t *u, char *chname, int chflags,
			int ochatr)
{
  int i, found = 0, atr = u ? u->flags : 0;
  struct chanset_t *chan;

  if (!u)
    return 0;
  for (i = 0; i < dcc_total; i++) {
    if (dcc[i].type && (dcc[i].type->flags & DCT_MASTER) &&
	!strcasecmp(u->handle, dcc[i].nick)) {
      if ((ochatr & USER_MASTER) && !(chflags & USER_MASTER)) {
	if (!(atr & USER_MASTER))
	  dcc[i].u.chat->con_flags &= ~(LOG_MISC | LOG_CMDS);
	dprintf(i, "*** POOF! ***\n");
	dprintf(i, _("You are no longer a master on %s.\n"), chname);
      }
      if (!(ochatr & USER_MASTER) && (chflags & USER_MASTER)) {
	dcc[i].u.chat->con_flags |= conmask;
	if (!(atr & USER_MASTER))
	  dcc[i].u.chat->con_flags &=
	    ~(LOG_LEV1 | LOG_LEV2 | LOG_LEV3 | LOG_LEV4 |
	      LOG_LEV5 | LOG_LEV6 | LOG_LEV7 | LOG_LEV8 |
	      LOG_RAW | LOG_DEBUG | LOG_WALL | LOG_FILES | LOG_SRVOUT);
	dprintf(i, "*** POOF! ***\n");
	dprintf(i, _("You are now a master on %s.\n"), chname);
      }
      if (!(ochatr & USER_OWNER) && (chflags & USER_OWNER)) {
	dprintf(i, "@@@ POOF! @@@\n");
	dprintf(i, _("You are now an OWNER of %s.\n"), chname);
      }
      if ((ochatr & USER_OWNER) && !(chflags & USER_OWNER)) {
	dprintf(i, "@@@ POOF! @@@\n");
	dprintf(i, _("You are no longer an owner of %s.\n"), chname);
      }
      if (((ochatr & (USER_OP | USER_MASTER | USER_OWNER)) &&
	   (!(chflags & (USER_OP | USER_MASTER | USER_OWNER)))) ||
	  ((chflags & (USER_OP | USER_MASTER | USER_OWNER)) &&
	   (!(ochatr & (USER_OP | USER_MASTER | USER_OWNER))))) {
	struct flag_record fr = {FR_CHAN, 0, 0, 0, 0, 0};

	for (chan = chanset; chan && !found; chan = chan->next) {
	  get_user_flagrec(u, &fr, chan->dname);
	  if (fr.chan & (USER_OP | USER_MASTER | USER_OWNER))
	    found = 1;
	}
	if (!chan)
	  chan = chanset;
	if (chan)
	  strcpy(dcc[i].u.chat->con_chan, chan->dname);
	else
	  strcpy(dcc[i].u.chat->con_chan, "*");
      }
    }
  }
  return chflags;
}

static int cmd_chattr(user_t *u, int idx, char *par)
{
  char *hand, *arg = NULL, *tmpchg = NULL, *chg = NULL, work[1024];
  struct chanset_t *chan = NULL;
  user_t *u2;
  struct flag_record pls = {0, 0, 0, 0, 0, 0},
  		     mns = {0, 0, 0, 0, 0, 0},
		     user = {0, 0, 0, 0, 0, 0};
  module_entry *me;
  int fl = -1, of = 0, ocf = 0;

  if (!par[0]) {
    dprintf(idx, "Usage: chattr <handle> [changes] [channel]\n");
    return(0);
  }
  hand = newsplit(&par);
  u2 = get_user_by_handle(userlist, hand);
  if (!u2) {
    dprintf(idx, _("No such user!\n"));
    return(0);
  }

  /* Parse args */
  if (par[0]) {
    arg = newsplit(&par);
    if (par[0]) {
      /* .chattr <handle> <changes> <channel> */
      chg = arg;
      arg = newsplit(&par);
      chan = findchan_by_dname(arg);
    } else {
      chan = findchan_by_dname(arg);
      /* Consider modeless channels, starting with '+' */
      if (!(arg[0] == '+' && chan) &&
          !(arg[0] != '+' && strchr (CHANMETA, arg[0]))) {
	/* .chattr <handle> <changes> */
        chg = arg;
        chan = NULL; /* uh, !strchr (CHANMETA, channel[0]) && channel found?? */
	arg = NULL;
      }
      /* .chattr <handle> <channel>: nothing to do... */
    }
  }
  /* arg:  pointer to channel name, NULL if none specified
   * chan: pointer to channel structure, NULL if none found or none specified
   * chg:  pointer to changes, NULL if none specified
   */
  assert(!(!arg && chan));
  if (arg && !chan) {
    dprintf(idx, _("No channel record for %s.\n"), arg);
    return(0);
  }
  if (chg) {
    if (!arg && strpbrk(chg, "&|")) {
      /* .chattr <handle> *[&|]*: use console channel if found... */
      if (!strcmp ((arg = dcc[idx].u.chat->con_chan), "*"))
        arg = NULL;
      else
        chan = findchan_by_dname(arg);
      if (arg && !chan) {
        dprintf (idx, _("Invalid console channel %s.\n"), arg);
	return(0);
      }
    } else if (arg && !strpbrk(chg, "&|")) {
      tmpchg = malloc(strlen(chg) + 2);
      strcpy(tmpchg, "|");
      strcat(tmpchg, chg);
      chg = tmpchg;
    }
  }
  par = arg;
  user.match = FR_GLOBAL;
  if (chan)
    user.match |= FR_CHAN;
  get_user_flagrec(u, &user, chan ? chan->dname : 0);
  if (!chan && !glob_botmast(user)) {
    dprintf(idx, _("You do not have Bot Master privileges.\n"));
    if (tmpchg)
      free(tmpchg);
    return(0);
  }
  if (chan && !glob_master(user) && !chan_master(user)) {
    dprintf(idx,
	    _("You do not have channel master privileges for channel %s.\n"),
	    par);
    if (tmpchg)
      free(tmpchg);
    return(0);
  }
  user.match &= fl;
  if (chg) {
    pls.match = user.match;
    break_down_flags(chg, &pls, &mns);
    /* No-one can change these flags on-the-fly */
    if (!glob_owner(user)) {
      pls.global &= ~(USER_OWNER | USER_MASTER | USER_BOTMAST);
      mns.global &= ~(USER_OWNER | USER_MASTER | USER_BOTMAST);

      if (chan) {
	pls.chan &= ~USER_OWNER;
	mns.chan &= ~USER_OWNER;
      }
      if (!glob_master(user)) {
	pls.global &= USER_PARTY | USER_XFER;
	mns.global &= USER_PARTY | USER_XFER;

	if (!glob_botmast(user)) {
	  pls.global = 0;
	  mns.global = 0;
	}
      }
    }
    if (chan && !chan_owner(user) && !glob_owner(user)) {
      pls.chan &= ~USER_MASTER;
      mns.chan &= ~USER_MASTER;
      if (!chan_master(user) && !glob_master(user)) {
	pls.chan = 0;
	mns.chan = 0;
      }
    }
    get_user_flagrec(u2, &user, par);
    if (user.match & FR_GLOBAL) {
      of = user.global;
      user.global = sanity_check((user.global |pls.global) &~mns.global);

      user.udef_global = (user.udef_global | pls.udef_global)
	& ~mns.udef_global;
    }
    if (chan) {
      ocf = user.chan;
      user.chan = chan_sanity_check((user.chan | pls.chan) & ~mns.chan,
				    user.global);

      user.udef_chan = (user.udef_chan | pls.udef_chan) & ~mns.udef_chan;
    }
    set_user_flagrec(u2, &user, par);
  }
  /* Get current flags and display them */
  if (user.match & FR_GLOBAL) {
    user.match = FR_GLOBAL;
    if (chg)
      check_dcc_attrs(u2, of);
    get_user_flagrec(u2, &user, NULL);
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, _("Global flags for %1$s are now +%2$s.\n"), hand, work);
    else
      dprintf(idx, _("No global flags for %s.\n"), hand);
  }
  if (chan) {
    user.match = FR_CHAN;
    get_user_flagrec(u2, &user, par);
    if (chg)
      check_dcc_chanattrs(u2, chan->dname, user.chan, ocf);
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, _("Channel flags for %1$s on %2$s are now +%3$s.\n"), hand,
	      chan->dname, work);
    else
      dprintf(idx, _("No flags for %1$s on %2$s.\n"), hand, chan->dname);
  }
  if (chg && (me = module_find("irc", 0, 0))) {
    Function *func = me->funcs;
    
    (func[IRC_CHECK_THIS_USER]) (hand, 0, NULL);
  }
  if (tmpchg)
    free(tmpchg);
  return(1);
}

static int cmd_botattr(user_t *u, int idx, char *par)
{
  char *hand, *chg = NULL, *arg = NULL, *tmpchg = NULL, work[1024];
  struct chanset_t *chan = NULL;
  user_t *u2;
  struct flag_record pls = {0, 0, 0, 0, 0, 0},
		     mns = {0, 0, 0, 0, 0, 0},
		     user = {0, 0, 0, 0, 0, 0};
  int idx2;

  if (!par[0]) {
    dprintf(idx, "Usage: botattr <handle> [changes] [channel]\n");
    return(0);
  }
  hand = newsplit(&par);
  u2 = get_user_by_handle(userlist, hand);
  if (!u2 || !(u2->flags & USER_BOT)) {
    dprintf(idx, _("No such bot!\n"));
    return(0);
  }
  for (idx2 = 0; idx2 < dcc_total; idx2++)
    if (dcc[idx2].type && !strcasecmp(dcc[idx2].nick, hand))
      break;
  if (idx2 != dcc_total) {
    dprintf(idx,
            _("You may not change the attributes of a directly linked bot.\n"));
    return(0);
  }
  /* Parse args */
  if (par[0]) {
    arg = newsplit(&par);
    if (par[0]) {
      /* .botattr <handle> <changes> <channel> */
      chg = arg;
      arg = newsplit(&par);
      chan = findchan_by_dname(arg);
    } else {
      chan = findchan_by_dname(arg);
      /* Consider modeless channels, starting with '+' */
      if (!(arg[0] == '+' && chan) &&
          !(arg[0] != '+' && strchr (CHANMETA, arg[0]))) {
	/* .botattr <handle> <changes> */
        chg = arg;
        chan = NULL; /* uh, !strchr (CHANMETA, channel[0]) && channel found?? */
	arg = NULL;
      }
      /* .botattr <handle> <channel>: nothing to do... */
    }
  }
  /* arg:  pointer to channel name, NULL if none specified
   * chan: pointer to channel structure, NULL if none found or none specified
   * chg:  pointer to changes, NULL if none specified
   */
  assert(!(!arg && chan));
  if (arg && !chan) {
    dprintf(idx, _("No channel record for %s.\n"), arg);
    return(0);
  }
  if (chg) {
    if (!arg && strpbrk(chg, "&|")) {
      /* botattr <handle> *[&|]*: use console channel if found... */
      if (!strcmp ((arg = dcc[idx].u.chat->con_chan), "*"))
        arg = NULL;
      else
        chan = findchan_by_dname(arg);
      if (arg && !chan) {
        dprintf (idx, _("Invalid console channel %s.\n"), arg);
	return(0);
      }
    } else if (arg && !strpbrk(chg, "&|")) {
      tmpchg = malloc(strlen(chg) + 2);
      strcpy(tmpchg, "|");
      strcat(tmpchg, chg);
      chg = tmpchg;
    }
  }
  par = arg;

  user.match = FR_GLOBAL;
  get_user_flagrec(u, &user, chan ? chan->dname : 0);
  if (!glob_botmast(user)) {
    dprintf(idx, _("You do not have Bot Master privileges.\n"));
    if (tmpchg)
      free(tmpchg);
    return(0);
  }
  if (chg) {
    user.match = FR_BOT | (chan ? FR_CHAN : 0);
    pls.match = user.match;
    break_down_flags(chg, &pls, &mns);
    pls.chan = 0;
    mns.chan = 0;
    user.match = FR_BOT | (chan ? FR_CHAN : 0);
    get_user_flagrec(u2, &user, par);
    user.bot = (user.bot | pls.bot) & ~mns.bot;
    if (chan)
      user.chan = (user.chan | pls.chan) & ~mns.chan;
    set_user_flagrec(u2, &user, par);
  }
  /* get current flags and display them */
  if (!chan || pls.bot || mns.bot) {
    user.match = FR_BOT;
    get_user_flagrec(u2, &user, NULL);
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, _("Bot flags for %1$s are now +%2$s.\n"), hand, work);
    else
      dprintf(idx, _("No bot flags for %s.\n"), hand);
  }
  if (chan) {
    user.match = FR_CHAN;
    get_user_flagrec(u2, &user, par);
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, _("Bot flags for %1$s on %2$s are now +%3$s.\n"), hand,
	      chan->dname, work);
    else
      dprintf(idx, _("No bot flags for %1$s on %2$s.\n"), hand, chan->dname);
  }
  if (tmpchg)
    free(tmpchg);
  return(1);
}

static int cmd_su(user_t *u, int idx, char *par)
{
  int atr = u ? u->flags : 0;
  struct flag_record fr = {FR_ANYWH | FR_CHAN | FR_GLOBAL, 0, 0, 0, 0, 0};

  u = get_user_by_handle(userlist, par);

  if (!par[0])
    dprintf(idx, "Usage: su <user>\n");
  else if (!u)
    dprintf(idx, _("No such user.\n"));
  else if (u->flags & USER_BOT)
    dprintf(idx, _("Can't su to a bot... then again, why would you wanna?\n"));
  else if (dcc[idx].u.chat->su_nick)
    dprintf(idx, _("You cannot currently double .su, try .su'ing directly\n"));
  else {
    get_user_flagrec(u, &fr, NULL);
    if (!glob_party(fr) && !(atr & USER_BOTMAST))
      dprintf(idx, _("No party line access permitted for %s.\n"), par);
    else {
      correct_handle(par);
      if (!(atr & USER_OWNER) ||
	  ((u->flags & USER_OWNER) && (isowner(par)) &&
	   !(isowner(dcc[idx].nick)))) {
	/* This check is only important for non-owners */
	if (u_pass_match(u, "-")) {
	  dprintf(idx,
		  _("No password set for user. You may not .su to them.\n"));
	  return(0);
	}
	chanout_but(-1, dcc[idx].u.chat->channel,
		    "*** %s left the party line.\n", dcc[idx].nick);
	/* Store the old nick in the away section, for weenies who can't get
	 * their password right ;)
	 */
	if (dcc[idx].u.chat->away != NULL)
	  free(dcc[idx].u.chat->away);
	dcc[idx].u.chat->away = calloc(1, strlen(dcc[idx].nick) + 1);
	strcpy(dcc[idx].u.chat->away, dcc[idx].nick);
	dcc[idx].u.chat->su_nick = calloc(1, strlen(dcc[idx].nick) + 1);
	strcpy(dcc[idx].u.chat->su_nick, dcc[idx].nick);
	dcc[idx].user = u;
	strcpy(dcc[idx].nick, par);
	/* Display password prompt and turn off echo (send IAC WILL ECHO). */
	dprintf(idx, "Enter password for %s%s\n", par,
		(dcc[idx].status & STAT_TELNET) ? TLN_IAC_C TLN_WILL_C
	       					  TLN_ECHO_C : "");
	dcc[idx].type = &DCC_CHAT_PASS;
      } else if (atr & USER_OWNER) {
	chanout_but(-1, dcc[idx].u.chat->channel,
		    "*** %s left the party line.\n", dcc[idx].nick);
	dprintf(idx, _("Setting your username to %s.\n"), par);
	if (atr & USER_MASTER)
	  dcc[idx].u.chat->con_flags = conmask;
	dcc[idx].u.chat->su_nick = calloc(1, strlen(dcc[idx].nick) + 1);
	strcpy(dcc[idx].u.chat->su_nick, dcc[idx].nick);
	dcc[idx].user = u;
	strcpy(dcc[idx].nick, par);
	dcc_chatter(idx);
      }
    }
  }
  return(1);
}

static int cmd_fixcodes(user_t *u, int idx, char *par)
{
  if (dcc[idx].status & STAT_ECHO) {
    dcc[idx].status |= STAT_TELNET;
    dcc[idx].status &= ~STAT_ECHO;
    dprintf(idx, _("Turned on telnet codes\n"));
    putlog(LOG_CMDS, "*", "#%s# fixcodes (telnet on)", dcc[idx].nick);
    return(0);
  }
  if (dcc[idx].status & STAT_TELNET) {
    dcc[idx].status |= STAT_ECHO;
    dcc[idx].status &= ~STAT_TELNET;
    dprintf(idx, _("Turned off telnet codes\n"));
    putlog(LOG_CMDS, "*", "#%s# fixcodes (telnet off)", dcc[idx].nick);
    return(0);
  }
  return(1);
}

static int cmd_page(user_t *u, int idx, char *par)
{
  int a;
  module_entry *me;

  if (!par[0]) {
    if (dcc[idx].status & STAT_PAGE) {
      dprintf(idx, _("Currently paging outputs to %d lines.\n"),
	      dcc[idx].u.chat->max_line);
    } else
      dprintf(idx, _("You don't have paging on.\n"));
    return(0);
  }
  a = atoi(par);
  if ((!a && !par[0]) || !strcasecmp(par, "off")) {
    dcc[idx].status &= ~STAT_PAGE;
    dcc[idx].u.chat->max_line = 0x7ffffff;	/* flush_lines needs this */
    while (dcc[idx].u.chat->buffer)
      flush_lines(idx, dcc[idx].u.chat);
    dprintf(idx, _("Paging turned off.\n"));
    putlog(LOG_CMDS, "*", "#%s# page off", dcc[idx].nick);
  } else if (a > 0) {
    dprintf(idx, P_("Paging turned on, stopping every %d line.\n",
                    "Paging turned on, stopping every %d lines.\n", a), a);
    dcc[idx].status |= STAT_PAGE;
    dcc[idx].u.chat->max_line = a;
    dcc[idx].u.chat->line_count = 0;
    dcc[idx].u.chat->current_lines = 0;
    putlog(LOG_CMDS, "*", "#%s# page %d", dcc[idx].nick, a);
  } else {
    dprintf(idx, "Usage: page <off or #>\n");
    return(0);
  }
  /* New style autosave here too -- rtc, 09/28/1999*/
  if ((me = module_find("console", 1, 1))) {
    Function *func = me->funcs;
    (func[CONSOLE_DOSTORE]) (idx);
  }
  return(1);
}

static int cmd_module(user_t *u, int idx, char *par)
{
  do_module_report(idx, 2, par[0] ? par : NULL);
  return(1);
}

static int cmd_loadmod(user_t *u, int idx, char *par)
{
  const char *p;

     if (!isowner(dcc[idx].nick)) {
         dprintf(idx, _("What?  You need .help\n"));
         return(0);
     }
  if (!par[0]) {
    dprintf(idx, "%s: loadmod <module>\n", _("Usage"));
  } else {
    p = module_load(par);
    if (p)
      dprintf(idx, "%s: %s %s\n", par, _("Error loading module:"), p);
    else {
      dprintf(idx, _("Module loaded: %-16s"), par);
      dprintf(idx, "\n");
    }
  }
  return(1);
}

static int cmd_unloadmod(user_t *u, int idx, char *par)
{
  char *p;

     if (!isowner(dcc[idx].nick)) {
         dprintf(idx, _("What?  You need .help\n"));
         return(0);
     }
  if (!par[0]) {
    dprintf(idx, "%s: unloadmod <module>\n", _("Usage"));
  } else {
    p = module_unload(par, dcc[idx].nick);
    if (p)
      dprintf(idx, "%s %s: %s\n", _("Error unloading module:"), par, p);
    else {
      dprintf(idx, "%s %s\n", _("Module unloaded:"), par);
    }
  }
  return(1);
}

static int cmd_pls_ignore(user_t *u, int idx, char *par)
{
  char			*who;
  char			 s[UHOSTLEN];
  unsigned long int	 expire_time = 0;

  if (!par[0]) {
    dprintf(idx,
	    "Usage: +ignore <hostmask> [%%<XdXhXm>] [comment]\n");
    return(0);
  }

  who = newsplit(&par);
  if (par[0] == '%') {
    char		*p, *p_expire;
    unsigned long int	 expire_foo;

    p = newsplit(&par);
    p_expire = p + 1;
    while (*(++p) != 0) {
      switch (tolower(*p)) {
      case 'd':
	*p = 0;
	expire_foo = strtol(p_expire, NULL, 10);
	if (expire_foo > 365)
	  expire_foo = 365;
	expire_time += 86400 * expire_foo;
	p_expire = p + 1;
	break;
      case 'h':
	*p = 0;
	expire_foo = strtol(p_expire, NULL, 10);
	if (expire_foo > 8760)
	  expire_foo = 8760;
	expire_time += 3600 * expire_foo;
	p_expire = p + 1;
	break;
      case 'm':
	*p = 0;
	expire_foo = strtol(p_expire, NULL, 10);
	if (expire_foo > 525600)
	  expire_foo = 525600;
	expire_time += 60 * expire_foo;
	p_expire = p + 1;
      }
    }
  }
  if (!par[0])
    par = "requested";
  else if (strlen(par) > 65)
    par[65] = 0;
  if (strlen(who) > UHOSTMAX - 4)
    who[UHOSTMAX - 4] = 0;

  /* Fix missing ! or @ BEFORE continuing */
  if (!strchr(who, '!')) {
    if (!strchr(who, '@'))
      simple_sprintf(s, "%s!*@*", who);
    else
      simple_sprintf(s, "*!%s", who);
  } else if (!strchr(who, '@'))
    simple_sprintf(s, "%s@*", who);
  else
    strcpy(s, who);

  if (match_ignore(s))
    dprintf(idx, _("That already matches an existing ignore.\n"));
  else {
    dprintf(idx, "Now ignoring: %s (%s)\n", s, par);
    addignore(s, dcc[idx].nick, par, expire_time ? now + expire_time : 0L);
  }
  return(1);
}

static int cmd_mns_ignore(user_t *u, int idx, char *par)
{
  char buf[UHOSTLEN];

  if (!par[0]) {
    dprintf(idx, "Usage: -ignore <hostmask | ignore #>\n");
    return(0);
  }
  strlcpy(buf, par, sizeof buf);
  if (delignore(buf)) {
    dprintf(idx, _("No longer ignoring: %s\n"), buf);
  } else
    dprintf(idx, _("That ignore cannot be found.\n"));
  return(1);
}

static int cmd_ignores(user_t *u, int idx, char *par)
{
  tell_ignores(idx, par);
  return(1);
}

static int cmd_pls_user(user_t *u, int idx, char *par)
{
  char *handle, *host;

  if (!par[0]) {
    dprintf(idx, "Usage: +user <handle> [hostmask]\n");
    return(0);
  }
  handle = newsplit(&par);
  host = newsplit(&par);
  if (strlen(handle) > HANDLEN)
    handle[HANDLEN] = 0;
  if (get_user_by_handle(userlist, handle))
    dprintf(idx, _("Someone already exists by that name.\n"));
  else if (strchr(BADNICKCHARS, handle[0]) != NULL)
    dprintf(idx, _("You can't start a nick with '%c'.\n"), handle[0]);
  else if (!strcasecmp(handle, myname))
    dprintf(idx, _("Hey! That's MY name!\n"));
  else {
    userlist = adduser(userlist, handle, host, "-", 0);
    dprintf(idx, _("Added %1$s (%2$s) with no password and no flags.\n"),
            handle, host[0] ? host : _("no host"));
  }
  return(1);
}

static int cmd_mns_user(user_t *u, int idx, char *par)
{
  int idx2;
  char *handle;
  user_t *u2;
  module_entry *me;

  if (!par[0]) {
    dprintf(idx, "Usage: -user <hand>\n");
    return(0);
  }
  handle = newsplit(&par);
  u2 = get_user_by_handle(userlist, handle);
  if (!u2 || !u) {
    dprintf(idx, _("No such user!\n"));
    return(0);
  }
  if (isowner(u2->handle)) {
    dprintf(idx, _("You can't remove the permanent bot owner!\n"));
    return(0);
  }
  if ((u2->flags & USER_OWNER) && !(u->flags & USER_OWNER)) {
    dprintf(idx, _("You can't remove a bot owner!\n"));
    return(0);
  }
  if (u2->flags & USER_BOT) {
    for (idx2 = 0; idx2 < dcc_total; idx2++)
      if (dcc[idx2].type && !strcasecmp(dcc[idx2].nick, handle))
        break;
    if (idx2 != dcc_total) {
      dprintf(idx, _("You can't remove a directly linked bot.\n"));
      return(0);
    }
  }
  if ((u->flags & USER_BOTMAST) && !(u->flags & USER_MASTER) &&
      !(u2->flags & USER_BOT)) {
    dprintf(idx, _("You can't remove users who aren't bots!\n"));
    return(0);
  }
  if ((me = module_find("irc", 0, 0))) {
    Function *func = me->funcs;

   (func[IRC_CHECK_THIS_USER]) (handle, 1, NULL);
  }
  if (deluser(handle)) {
    dprintf(idx, _("Deleted %s.\n"), handle);
  } else
    dprintf(idx, _("Failed.\n"));
  return(1);
}

static int cmd_pls_host(user_t *u, int idx, char *par)
{
  char *handle, *host;
  user_t *u2;
  struct list_type *q;
  struct flag_record fr = {FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};
  struct flag_record fr2 = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0},
                     fr  = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  if (!par[0]) {
    dprintf(idx, "Usage: +host [handle] <newhostmask>\n");
    return(0);
  }

  handle = newsplit(&par);

  if (par[0]) {
    host = newsplit(&par);
    u2 = get_user_by_handle(userlist, handle);
  } else {
    host = handle;
    handle = dcc[idx].nick;
    u2 = u;
  }
  if (!u2 || !u) {
    dprintf(idx, _("No such user.\n"));
    return(0);
  }
  get_user_flagrec(u, &fr, NULL);
  if (strcasecmp(handle, dcc[idx].nick)) {
    get_user_flagrec(u2, &fr2, NULL);
    if (!glob_master(fr) && !glob_bot(fr2) && !chan_master(fr)) {
      dprintf(idx, _("You can't add hostmasks to non-bots.\n"));
      return(0);
    }
    if ((glob_owner(fr2) || glob_master(fr2)) && !glob_owner(fr)) {
      dprintf(idx, _("You can't add hostmasks to the bot owner/master.\n"));
      return(0);
    }
    if ((chan_owner(fr2) || chan_master(fr2)) && !glob_master(fr) &&
        !glob_owner(fr) && !chan_owner(fr)) {
      dprintf(idx, "You can't add hostmasks to a channel owner/master.\n");
      return(0);
    }
    if (!glob_botmast(fr) && !glob_master(fr) && !chan_master(fr)) {
      dprintf(idx, "Permission denied.\n");
      return(0);
    }
  }
  if (!glob_botmast(fr) && !chan_master(fr) && get_user_by_host(host)) {
    dprintf(idx, "You cannot add a host matching another user!\n");
    return(0);
  }
  for (q = get_user(&USERENTRY_HOSTS, u); q; q = q->next)
    if (!strcasecmp(q->extra, host)) {
      dprintf(idx, _("That hostmask is already there.\n"));
      return(0);
    }
  addhost_by_handle(handle, host);
  dprintf(idx, "Added '%s' to %s.\n", host, handle);
  if ((me = module_find("irc", 0, 0))) {
    Function *func = me->funcs;

    (func[IRC_CHECK_THIS_USER]) (handle, 0, NULL);
  }
  return(1);
}

static int cmd_mns_host(user_t *u, int idx, char *par)
{
  char *handle, *host;
  user_t *u2;
  struct flag_record fr2 = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0},
                     fr  = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};
  module_entry *me;

  if (!par[0]) {
    dprintf(idx, "%s: -host [%s] <%s>\n", _("Usage"), _("handle"), 
	    _("hostmask"));
    return(0);
  }
  handle = newsplit(&par);
  if (par[0]) {
    host = newsplit(&par);
    u2 = get_user_by_handle(userlist, handle);
  } else {
    host = handle;
    handle = dcc[idx].nick;
    u2 = u;
  }
  if (!u2 || !u) {
    dprintf(idx, "%s\n", _("No such user."));
    return(0);
  }

  get_user_flagrec(u, &fr, NULL);
  get_user_flagrec(u2, &fr2, NULL);
  if (strcasecmp(handle, dcc[idx].nick)) {
    if (!glob_master(fr) && !glob_bot(fr2) && !chan_master(fr)) {
      dprintf(idx, _("You can't remove hostmasks from non-bots.\n"));
      return(0);
    }
    if ((glob_owner(fr2) || glob_master(fr2)) && !glob_owner(fr)) {
      dprintf(idx, _("You can't remove hostmasks from a bot owner/master.\n"));
      return(0);
    }
    if ((chan_owner(fr2) || chan_master(fr2)) && !glob_master(fr) &&
        !glob_owner(fr) && !chan_owner(fr)) {
      dprintf(idx, "You can't remove hostmasks from a channel owner/master.\n");
      return(0);
    }
    if (!glob_botmast(fr) && !glob_master(fr) && !chan_master(fr)) {
      dprintf(idx, _("Permission denied.\n"));
      return(0);
    }
  }
  if (delhost_by_handle(handle, host)) {
    dprintf(idx, _("Removed '%1$s' from %2$s.\n"), host, handle);
    if ((me = module_find("irc", 0, 0))) {
      Function *func = me->funcs;

      (func[IRC_CHECK_THIS_USER]) (handle, 2, host);
    }
  } else
    dprintf(idx, _("Failed.\n"));
  return(1);
}

static int cmd_modules(user_t *u, int idx, char *par)
{
  module_entry *me;

    dprintf(idx, _("Modules loaded:\n"));
    for (me = module_list; me; me = me->next)
      dprintf(idx, "  Module: %s (v%d.%d)\n", me->name, me->major, me->minor);
    dprintf(idx, _("End of modules list.\n"));
  return(1);
}

static int cmd_traffic(user_t *u, int idx, char *par)
{
  unsigned long itmp, itmp2;

  dprintf(idx, "Traffic since last restart\n");
  dprintf(idx, "==========================\n");
  if (traffic.out_total.irc > 0 || traffic.in_total.irc > 0 || traffic.out_today.irc > 0 ||
      traffic.in_today.irc > 0) {
    dprintf(idx, "IRC:\n");
    dprintf(idx, "  out: %s", btos(traffic.out_total.irc + traffic.out_today.irc));
              dprintf(idx, " (%s today)\n", btos(traffic.out_today.irc));
    dprintf(idx, "   in: %s", btos(traffic.in_total.irc + traffic.in_today.irc));
              dprintf(idx, " (%s today)\n", btos(traffic.in_today.irc));
  }
  if (traffic.out_total.dcc > 0 || traffic.in_total.dcc > 0 || traffic.out_today.dcc > 0 ||
      traffic.in_today.dcc > 0) {
    dprintf(idx, "Partyline:\n");
    itmp = traffic.out_total.dcc + traffic.out_today.dcc;
    itmp2 = traffic.out_today.dcc;
    dprintf(idx, "  out: %s", btos(itmp));
              dprintf(idx, " (%s today)\n", btos(itmp2));
    dprintf(idx, "   in: %s", btos(traffic.in_total.dcc + traffic.in_today.dcc));
              dprintf(idx, " (%s today)\n", btos(traffic.in_today.dcc));
  }
  if (traffic.out_total.unknown > 0 || traffic.out_today.unknown > 0) {
    dprintf(idx, "Misc:\n");
    dprintf(idx, "  out: %s", btos(traffic.out_total.unknown + traffic.out_today.unknown));
              dprintf(idx, " (%s today)\n", btos(traffic.out_today.unknown));
    dprintf(idx, "   in: %s", btos(traffic.in_total.unknown + traffic.in_today.unknown));
              dprintf(idx, " (%s today)\n", btos(traffic.in_today.unknown));
  }
  dprintf(idx, "---\n");
  dprintf(idx, "Total:\n");
  itmp =  traffic.out_total.irc + traffic.out_total.bn + traffic.out_total.dcc +
          traffic.out_total.unknown + traffic.out_today.irc +
          traffic.out_today.dcc + traffic.out_today.unknown;
  itmp2 = traffic.out_today.irc + traffic.out_today.dcc +
          traffic.out_today.unknown;
  dprintf(idx, "  out: %s", btos(itmp));
              dprintf(idx, " (%s today)\n", btos(itmp2));
  dprintf(idx, "   in: %s", btos(traffic.in_total.irc + traffic.in_total.dcc +
          traffic.in_total.unknown + traffic.in_today.irc +
          traffic.in_today.dcc + traffic.in_today.unknown));
  dprintf(idx, " (%s today)\n", btos(traffic.in_today.irc +
          traffic.in_today.dcc + traffic.in_today.unknown));
  return(1);
}

static char traffictxt[20];
static char *btos(unsigned long  bytes)
{
  char unit[10];
  float xbytes;

  sprintf(unit, "Bytes");
  xbytes = bytes;
  if (xbytes > 1024.0) {
    sprintf(unit, "KBytes");
    xbytes = xbytes / 1024.0;
  }
  if (xbytes > 1024.0) {
    sprintf(unit, "MBytes");
    xbytes = xbytes / 1024.0;
  }
  if (xbytes > 1024.0) {
    sprintf(unit, "GBytes");
    xbytes = xbytes / 1024.0;
  }
  if (xbytes > 1024.0) {
    sprintf(unit, "TBytes");
    xbytes = xbytes / 1024.0;
  }
  if (bytes > 1024)
    sprintf(traffictxt, "%.2f %s", xbytes, unit);
  else
    sprintf(traffictxt, "%lu Bytes", bytes);
  return traffictxt;
}

static int cmd_whoami(user_t *u, int idx, char *par)
{
  dprintf(idx, _("You are %s@%s.\n"), dcc[idx].nick, myname);
  return(1);
}

static int cmd_quit(user_t *u, int idx, char *text)
{
  dprintf(idx, _("*** Ja mata!\n"));
  flush_lines(idx, dcc[idx].u.chat);
  putlog(LOG_MISC, "*", _("DCC connection closed (%s!%s)"), dcc[idx].nick,
	 dcc[idx].host);
  if (dcc[idx].u.chat->channel >= 0) {
    chanout_but(-1, dcc[idx].u.chat->channel, "*** %s left the party line%s%s\n", dcc[idx].nick, text[0] ? ": " : ".", text);
  }

  if (dcc[idx].u.chat->su_nick) {
    dcc[idx].user = get_user_by_handle(userlist, dcc[idx].u.chat->su_nick);
    dcc[idx].type = &DCC_CHAT;
    dprintf(idx, _("Returning to real nick %s!\n"), dcc[idx].u.chat->su_nick);
    free_null(dcc[idx].u.chat->su_nick);
    dcc_chatter(idx);
  } else if ((dcc[idx].sock != STDOUT) || backgrd) {
    killsock(dcc[idx].sock);
    lostdcc(idx);
  } else {
    dprintf(DP_STDOUT, "\n### SIMULATION RESET\n\n");
    dcc_chatter(idx);
  }
  return(1);
}



/* DCC CHAT COMMANDS
 */
/* Function call should be:
 *   int cmd_whatever(idx,"parameters");
 * As with msg commands, function is responsible for any logging.
 */
cmd_t C_dcc[] =
{
  {"+bot",		"t",	(Function) cmd_pls_bot,		NULL},
  {"+host",		"tm|m",	(Function) cmd_pls_host,	NULL},
  {"+ignore",		"m",	(Function) cmd_pls_ignore,	NULL},
  {"+user",		"m",	(Function) cmd_pls_user,	NULL},
  {"-bot",		"t",	(Function) cmd_mns_user,	NULL},
  {"-host",		"",	(Function) cmd_mns_host,	NULL},
  {"-ignore",		"m",	(Function) cmd_mns_ignore,	NULL},
  {"-user",		"m",	(Function) cmd_mns_user,	NULL},
  {"away",		"",	(Function) cmd_away,		NULL},
  {"back",		"",	(Function) cmd_back,		NULL},
  {"backup",		"m|m",	(Function) cmd_backup,		NULL},
  {"boot",		"t",	(Function) cmd_boot,		NULL},
  {"botattr",		"t",	(Function) cmd_botattr,		NULL},
  {"chaddr",		"t",	(Function) cmd_chaddr,		NULL},
  {"chattr",		"m|m",	(Function) cmd_chattr,		NULL},
  {"chhandle",		"t",	(Function) cmd_chhandle,	NULL},
  {"chnick",		"t",	(Function) cmd_chhandle,	NULL},
  {"chpass",		"t",	(Function) cmd_chpass,		NULL},
  {"comment",		"m",	(Function) cmd_comment,		NULL},
  {"console",		"to|o",	(Function) cmd_console,		NULL},
  {"dccstat",		"t",	(Function) cmd_dccstat,		NULL},
  {"die",		"n",	(Function) cmd_die,		NULL},
  {"fixcodes",		"",	(Function) cmd_fixcodes,	NULL},
  {"help",		"",	(Function) cmd_help,		NULL},
  {"ignores",		"m",	(Function) cmd_ignores,		NULL},
  {"loadmod",		"n",	(Function) cmd_loadmod,		NULL},
  {"match",		"to|o",	(Function) cmd_match,		NULL},
  {"me",		"",	(Function) cmd_me,		NULL},
  {"module",		"m",	(Function) cmd_module,		NULL},
  {"modules",		"n",	(Function) cmd_modules,		NULL},
  {"motd",		"",	(Function) cmd_motd,		NULL},
  {"newpass",		"",	(Function) cmd_newpass,		NULL},
  {"handle",		"",	(Function) cmd_handle,		NULL},
  {"page",		"",	(Function) cmd_page,		NULL},
  {"quit",		"",	(Function) cmd_quit,		NULL},
  {"rehash",		"m",	(Function) cmd_rehash,		NULL},
  {"rehelp",		"n",	(Function) cmd_rehelp,		NULL},
  {"reload",		"m|m",	(Function) cmd_reload,		NULL},
  {"restart",		"m",	(Function) cmd_restart,		NULL},
  {"save",		"m|m",	(Function) cmd_save,		NULL},
  {"simul",		"n",	(Function) cmd_simul,		NULL},
  {"status",		"m|m",	(Function) cmd_status,		NULL},
  {"su",		"",	(Function) cmd_su,		NULL},
  {"unloadmod",		"n",	(Function) cmd_unloadmod,	NULL},
  {"uptime",		"m|m",	(Function) cmd_uptime,		NULL},
  {"who",		"",	(Function) cmd_who,		NULL},
  {"whois",		"to|o",	(Function) cmd_whois,		NULL},
  {"traffic",		"m|m",	(Function) cmd_traffic,		NULL},
  {"whoami",		"",	(Function) cmd_whoami,		NULL},
  {NULL,		NULL,	NULL,				NULL}
};
