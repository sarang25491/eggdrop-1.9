/*
 * msgcmds.c --
 *
 *	all commands entered via /MSG
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

/* FIXME: #include mess
#ifndef lint
static const char rcsid[] = "$Id: msgcmds.c,v 1.17 2003/02/15 05:04:57 wcc Exp $";
#endif
*/

static int msg_hello(char *nick, char *h, struct userrec *u, char *p)
{
  char host[UHOSTLEN], s[UHOSTLEN], s1[UHOSTLEN], handle[HANDLEN + 1];
  char *p1;
  struct chanset_t *chan;

  if (!learn_users && !make_userfile)
    return 0;

  if (match_my_nick(nick))
    return 1;

  strlcpy(handle, nick, sizeof handle);
  if (get_user_by_handle(userlist, handle)) {
    dprintf(DP_HELP, _("NOTICE %s :I dont recognize you from that host.\n"), nick);
    dprintf(DP_HELP, _("NOTICE %s :Either you are using someone elses nickname or you need to type: /MSG %s IDENT (password)\n"), nick, botname);
    return 1;
  }
  snprintf(s, sizeof s, "%s!%s", nick, h);
  if (u_match_mask(global_bans, s)) {
    dprintf(DP_HELP, "NOTICE %s :%s.\n", nick, _("You're banned, goober."));
    return 1;
  }

  maskhost(s, host);
  if (make_userfile) {
    userlist = adduser(userlist, handle, host, "-",
		       sanity_check(default_flags | USER_MASTER | USER_OWNER));
    set_user(&USERENTRY_HOSTS, get_user_by_handle(userlist, handle),
	     "-telnet!*@*");
  } else
    userlist = adduser(userlist, handle, host, "-",
		       sanity_check(default_flags));
  putlog(LOG_MISC, "*", "%s %s (%s)", _("Introduced to"), nick, host);

  for (chan = chanset; chan; chan = chan->next)
    if (ismember(chan, handle))
      add_chanrec_by_handle(userlist, handle, chan->dname);
  dprintf(DP_HELP, _("NOTICE %s :Hi %s!  Im %s, an eggdrop bot.\n"), nick, nick, botname);
  dprintf(DP_HELP, _("NOTICE %s :Ill recognize you by hostmask %s from now on.\n"), nick, host);

  if (make_userfile) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("YOU ARE THE OWNER ON THIS BOT NOW"));
    dprintf(DP_HELP, _("NOTICE %s :As master you really need to set a password: with /MSG %s pass <your-chosen-password>.\n"), nick, botname);
    dprintf(DP_HELP, _("NOTICE %s :All major commands are used from DCC chat. From now on, you dont need to use the -m option when starting the bot.  Enjoy !!!\n"), nick);
    putlog(LOG_MISC, "*", _("Bot installation complete, first master is %s"), handle);
    make_userfile = 0;
    write_userfile(-1);
    add_note(handle, myname, _("Welcome to Eggdrop! =]"), -1, 0);
  } else {
    fr.global = default_flags;

    dprintf(DP_HELP, _("NOTICE %s :All commands are done via /MSG. For the complete list, /MSG %s help   Cya!\n"), nick, botname);
  }
  if (strlen(nick) > HANDLEN)
    /* Notify the user that his/her handle was truncated. */
    dprintf(DP_HELP, _("NOTICE %s :Your nick was too long and therefore it was truncated to %s.\n"), nick, handle);
  if (notify_new[0]) {
    snprintf(s, sizeof s, _("introduced to %s from %s"), nick, host);
    strcpy(s1, notify_new);
    while (s1[0]) {
      p1 = strchr(s1, ',');
      if (p1 != NULL) {
	*p1 = 0;
	p1++;
	rmspace(p1);
      }
      rmspace(s1);
      add_note(s1, myname, s, -1, 0);
      if (p1 == NULL)
	s1[0] = 0;
      else
	strcpy(s1, p1);
    }
  }
  return 1;
}

static int msg_pass(char *nick, char *host, struct userrec *u, char *par)
{
  char *old, *new;

  if (match_my_nick(nick))
    return 1;
  if (!u)
    return 1;
  if (u->flags & USER_BOT)
    return 1;
  if (!par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick,
	    u_pass_match(u, "-") ? _("You dont have a password set.") : _("You have a password set."));
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! PASS?", nick, host, u->handle);
    return 1;
  }
  old = newsplit(&par);
  if (!u_pass_match(u, "-") && !par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You already have a password set."));
    return 1;
  }
  if (par[0]) {
    if (!u_pass_match(u, old)) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Incorrect password."));
      return 1;
    }
    new = newsplit(&par);
  } else {
    new = old;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! PASS...", nick, host, u->handle);
  if (strlen(new) > 15)
    new[15] = 0;
  if (strlen(new) < 6) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Please use at least 6 characters."));
    return 0;
  }
  set_user(&USERENTRY_PASS, u, new);
  dprintf(DP_HELP, "NOTICE %s :%s '%s'.\n", nick,
	  new == old ? _("Password set to:") : _("Password changed to:"), new);
  return 1;
}

static int msg_ident(char *nick, char *host, struct userrec *u, char *par)
{
  char s[UHOSTLEN], s1[UHOSTLEN], *pass, who[NICKLEN];
  struct userrec *u2;

  if (match_my_nick(nick))
    return 1;
  if (u && (u->flags & USER_BOT))
    return 1;

  pass = newsplit(&par);
  if (!par[0])
    strcpy(who, nick);
  else {
    strncpy(who, par, NICKMAX);
    who[NICKMAX] = 0;
  }
  u2 = get_user_by_handle(userlist, who);
  if (!u2) {
    if (u && !quiet_reject)
      dprintf(DP_HELP, _("NOTICE %s :Youre not %s, youre %s.\n"), nick, nick, u->handle);
  } else if (irccmp(who, origbotname) && !(u2->flags & USER_BOT)) {
    /* This could be used as detection... */
    if (u_pass_match(u2, "-")) {
      putlog(LOG_CMDS, "*", "(%s!%s) !*! IDENT %s", nick, host, who);
      if (!quiet_reject)
	dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You dont have a password set."));
    } else if (!u_pass_match(u2, pass)) {
      if (!quiet_reject)
	dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Access denied."));
    } else if (u == u2) {
      if (!quiet_reject)
	dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("I recognize you there."));
      return 1;
    } else if (u) {
      if (!quiet_reject)
	dprintf(DP_HELP, _("NOTICE %s :Youre not %s, youre %s.\n"), nick, who, u->handle);
      return 1;
    } else {
      putlog(LOG_CMDS, "*", "(%s!%s) !*! IDENT %s", nick, host, who);
      snprintf(s, sizeof s, "%s!%s", nick, host);
      maskhost(s, s1);
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, _("Added hostmask"), s1);
      addhost_by_handle(who, s1);
      check_this_user(who, 0, NULL);
      return 1;
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !*! failed IDENT %s", nick, host, who);
  return 1;
}

static int msg_info(char *nick, char *host, struct userrec *u, char *par)
{
  char s[121], *pass, *chname, *p;
  int locked = 0;

  if (match_my_nick(nick))
    return 1;
  if (!use_info)
    return 1;
  if (!u)
    return 0;
  if (u->flags & USER_BOT)
    return 1;
  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed INFO", nick, host, u->handle);
      return 1;
    }
  } else {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed INFO", nick, host, u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You dont have a password set."));
    return 1;
  }
  if (par[0] && (strchr(CHANMETA, par[0]) != NULL)) {
    if (!findchan_by_dname(chname = newsplit(&par))) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("I dont monitor that channel."));
      return 1;
    }
  } else
    chname = 0;
  if (par[0]) {
    p = get_user(&USERENTRY_INFO, u);
    if (p && (p[0] == '@'))
      locked = 1;
    if (chname) {
      get_handle_chaninfo(u->handle, chname, s);
      if (s[0] == '@')
	locked = 1;
    }
    if (locked) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Your info line is locked"));
      return 1;
    }
    if (!strcasecmp(par, "none")) {
      par[0] = 0;
      if (chname) {
	set_handle_chaninfo(userlist, u->handle, chname, NULL);
	dprintf(DP_HELP, "NOTICE %s :%s %s.\n", nick, _("Removed your info line on"), chname);
	putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO %s NONE", nick, host,
	       u->handle, chname);
      } else {
	set_user(&USERENTRY_INFO, u, NULL);
	dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Removed your info line."));
	putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO NONE", nick, host, u->handle);
      }
      return 1;
    }
    if (par[0] == '@')
      par++;
    dprintf(DP_HELP, "NOTICE %s :%s %s\n", nick, _("Now:"), par);
    if (chname) {
      set_handle_chaninfo(userlist, u->handle, chname, par);
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO %s ...", nick, host, u->handle,
	     chname);
    } else {
      set_user(&USERENTRY_INFO, u, par);
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO ...", nick, host, u->handle);
    }
    return 1;
  }
  if (chname) {
    get_handle_chaninfo(u->handle, chname, s);
    p = s;
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO? %s", nick, host, u->handle,
	   chname);
  } else {
    p = get_user(&USERENTRY_INFO, u);
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO?", nick, host, u->handle);
  }
  if (p && p[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s %s\n", nick, _("Currently:"), p);
    dprintf(DP_HELP, "NOTICE %s :%s /msg %s info <pass>%s%s none\n",
	    nick, _("To remove it:"), botname, chname ? " " : "", chname
	    ? chname : "");
  } else {
    if (chname)
      dprintf(DP_HELP, "NOTICE %s :%s %s.\n", nick, _("You have no info set on"), chname);
    else
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You have no info set."));
  }
  return 1;
}

static int msg_who(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  memberlist *m;
  char s[UHOSTLEN], also[512], *info;
  int i;

  if (match_my_nick(nick))
    return 1;
  if (!u)
    return 0;
  if (!use_info)
    return 1;
  if (!par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s: /msg %s who <channel>\n", nick,
	    _("Usage"), botname);
    return 0;
  }
  chan = findchan_by_dname(par);
  if (!chan) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("I dont monitor that channel."));
    return 0;
  }
  get_user_flagrec(u, &fr, par);
  if (channel_hidden(chan) && !hand_on_chan(chan, u) &&
      !glob_op(fr) && !glob_friend(fr) &&
      !chan_op(fr) && !chan_friend(fr)) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Channel is currently hidden."));
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! WHO", nick, host, u->handle);
  also[0] = 0;
  i = 0;
  for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
    struct userrec *u;

    snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    u = get_user_by_host(s);
    info = get_user(&USERENTRY_INFO, u);
    if (u && (u->flags & USER_BOT))
      info = 0;
    if (info && (info[0] == '@'))
      info++;
    else if (u) {
      get_handle_chaninfo(u->handle, chan->dname, s);
      if (s[0]) {
	info = s;
	if (info[0] == '@')
	  info++;
      }
    }
    if (info && info[0])
      dprintf(DP_HELP, "NOTICE %s :[%9s] %s\n", nick, m->nick, info);
    else {
      if (match_my_nick(m->nick))
	dprintf(DP_HELP, "NOTICE %s :[%9s] <-- I'm the bot, of course.\n",
		nick, m->nick);
      else if (u && (u->flags & USER_BOT)) {
	dprintf(DP_HELP, "NOTICE %s :[%9s] <-- another bot\n", nick, m->nick);
      } else {
	if (i) {
	  also[i++] = ',';
	  also[i++] = ' ';
	}
	i += my_strcpy(also + i, m->nick);
	if (i > 400) {
	  dprintf(DP_HELP, "NOTICE %s :No info: %s\n", nick, also);
	  i = 0;
	  also[0] = 0;
	}
      }
    }
  }
  if (i) {
    dprintf(DP_HELP, "NOTICE %s :No info: %s\n", nick, also);
  }
  return 1;
}

static int msg_whois(char *nick, char *host, struct userrec *u, char *par)
{
  char s[UHOSTLEN], s1[81], *s2;
  int ok;
  struct chanset_t *chan;
  memberlist *m;
  struct chanuserrec *cr;
  struct userrec *u2;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  struct xtra_key *xk;
  time_t tt = 0;

  if (match_my_nick(nick))
    return 1;
  if (!u)
    return 0;
  if (!par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s: /msg %s whois <handle>\n", nick,
	    MISC_USAGE, botname);
    return 0;
  }

  if (strlen(par) > NICKMAX)
    par[NICKMAX] = 0;
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! WHOIS %s", nick, host, u->handle, par);
  u2 = get_user_by_handle(userlist, par);
  if (!u2) {
    /* No such handle -- maybe it's a nickname of someone on a chan? */
    ok = 0;
    for (chan = chanset; chan && !ok; chan = chan->next) {
      m = ismember(chan, par);
      if (m) {
	snprintf(s, sizeof s, "%s!%s", par, m->userhost);
	u2 = get_user_by_host(s);
	if (u2) {
	  ok = 1;
	  dprintf(DP_HELP, "NOTICE %s :[%s] AKA '%s':\n", nick,
		  par, u2->handle);
	}
      }
    }
    if (!ok) {
      dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, par, _("No user record."));
      return 1;
    }
  }
  s2 = get_user(&USERENTRY_INFO, u2);
  if (s2 && (s2[0] == '@'))
    s2++;
  if (s2 && s2[0] && !(u2->flags & USER_BOT))
    dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, u2->handle, s2);
  for (xk = get_user(&USERENTRY_XTRA, u2); xk; xk = xk->next)
    if (!strcasecmp(xk->key, "EMAIL"))
      dprintf(DP_HELP, "NOTICE %s :[%s] email: %s\n", nick, u2->handle,
	      xk->data);
  ok = 0;
  for (chan = chanset; chan; chan = chan->next) {
    if (hand_on_chan(chan, u2)) {
      snprintf(s1, sizeof s1, "NOTICE %s :[%s] %s %s.", nick, u2->handle,
		   _("Now on channel"), chan->dname);
      ok = 1;
    } else {
      get_user_flagrec(u, &fr, chan->dname);
      cr = get_chanrec(u2, chan->dname);
      if (cr && (cr->laston > tt) &&
	  (!channel_hidden(chan) || hand_on_chan(chan, u) || glob_op(fr) ||
	  glob_friend(fr) || chan_op(fr) || chan_friend(fr))) {
	tt = cr->laston;
	strftime(s, 14, "%b %d %H:%M", localtime(&tt));
	ok = 1;
	snprintf(s1, sizeof s1, "NOTICE %s :[%s] %s %s on %s", nick, u2->handle,
		     _("Last seen at"), s, chan->dname);
      }
    }
  }
  if (!ok)
    snprintf(s1, sizeof s1, "NOTICE %s :[%s] %s", nick, u2->handle, _("Never joined one of my channels."));
  if (u2->flags & USER_OP)
    strcat(s1, _("  (is a global op)"));
  if (u2->flags & USER_BOT)
    strcat(s1, _("  (is a bot)"));
  if (u2->flags & USER_MASTER)
    strcat(s1, _("  (is a master)"));
  dprintf(DP_HELP, "%s\n", s1);
  return 1;
}

static int msg_help(char *nick, char *host, struct userrec *u, char *par)
{
  char *p;

  if (match_my_nick(nick))
    return 1;

  if (!u) {
    if (!quiet_reject) {
      if (!learn_users)
        dprintf(DP_HELP, "NOTICE %s :No access\n", nick);
      else {
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("I dont know you; please introduce yourself first."));
        dprintf(DP_HELP, "NOTICE %s :/MSG %s hello\n", nick, botname);
      }
    }
    return 0;
  }

  if (helpdir[0]) {
    struct flag_record fr = {FR_ANYWH | FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

    get_user_flagrec(u, &fr, 0);
    if (!par[0])
      showhelp(nick, "help", &fr, 0);
    else {
      for (p = par; *p != 0; p++)
	if ((*p >= 'A') && (*p <= 'Z'))
	  *p += ('a' - 'A');
      showhelp(nick, par, &fr, 0);
    }
  } else
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("No help."));

  return 1;
}

/* I guess just op them on every channel they're on, unless they specify
 * a parameter.
 */
static int msg_op(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (match_my_nick(nick))
    return 1;
  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    /* Prevent people from gaining ops when no password set */
    if (!u_pass_match(u, "-")) {
      if (par[0]) {
	chan = findchan_by_dname(par);
	if (chan && channel_active(chan)) {
	  get_user_flagrec(u, &fr, par);
	  if (chan_op(fr) || glob_op(fr))
	    add_mode(chan, '+', 'o', nick);
	    putlog(LOG_CMDS, "*", "(%s!%s) !%s! OP %s",
		   nick, host, u->handle, par);
	  return 1;
	}
      } else {
	for (chan = chanset; chan; chan = chan->next) {
	  get_user_flagrec(u, &fr, chan->dname);
	  if (chan_op(fr) || glob_op(fr))
	    add_mode(chan, '+', 'o', nick);
	}
	putlog(LOG_CMDS, "*", "(%s!%s) !%s! OP", nick, host, u->handle);
	return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !*! failed OP", nick, host);
  return 1;
}

static int msg_key(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (match_my_nick(nick))
    return 1;
  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    /* Prevent people from getting key with no pass set */
    if (!u_pass_match(u, "-")) {
      if (!(chan = findchan_by_dname(par))) {
	dprintf(DP_HELP, "NOTICE %s :%s: /MSG %s key <pass> <channel>\n",
		nick, _("Usage"), botname);
	return 1;
      }
      if (!channel_active(chan)) {
	dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par, _("Not on that channel right now."));
	return 1;
      }
      chan = findchan_by_dname(par);
      if (chan && channel_active(chan)) {
	get_user_flagrec(u, &fr, par);
	if (chan_op(fr) || glob_op(fr)) {
	  if (chan->channel.key[0]) {
	    dprintf(DP_SERVER, "NOTICE %s :%s: key is %s\n", nick, par,
		    chan->channel.key);
	    if (invite_key && (chan->channel.mode & CHANINV)) {
	      dprintf(DP_SERVER, "INVITE %s %s\n", nick, chan->name);
	      putlog(LOG_CMDS, "*", "(%s!%s) !%s! KEY %s",
		     nick, host, u->handle, par);
	    }
	  } else {
	    dprintf(DP_HELP, "NOTICE %s :%s: no key set for this channel\n",
		    nick, par);
	    putlog(LOG_CMDS, "*", "(%s!%s) !%s! KEY %s", nick, host, u->handle,
		   par);
	  }
	}
	return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed KEY %s", nick, host,
	 (u ? u->handle : "*"), par);
  return 1;
}

/* Don't have to specify a channel now and can use this command
 * regardless of +autovoice or being a chanop. (guppy 7Jan1999)
 */
static int msg_voice(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (match_my_nick(nick))
    return 1;
  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    if (!u_pass_match(u, "-")) {
      if (par[0]) {
	chan = findchan_by_dname(par);
	if (chan && channel_active(chan)) {
	  get_user_flagrec(u, &fr, par);
	  if (chan_voice(fr) || glob_voice(fr) ||
	      chan_op(fr) || glob_op(fr)) {
	    add_mode(chan, '+', 'v', nick);
	    putlog(LOG_CMDS, "*", "(%s!%s) !%s! VOICE %s",
		   nick, host, u->handle, par);
	  } else
	    putlog(LOG_CMDS, "*", "(%s!%s) !*! failed VOICE %s",
		nick, host, par);
	  return 1;
	}
      } else {
	for (chan = chanset; chan; chan = chan->next) {
	  get_user_flagrec(u, &fr, chan->dname);
	  if (chan_voice(fr) || glob_voice(fr) ||
	      chan_op(fr) || glob_op(fr))
	    add_mode(chan, '+', 'v', nick);
	}
	putlog(LOG_CMDS, "*", "(%s!%s) !%s! VOICE", nick, host, u->handle);
	return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !*! failed VOICE", nick, host);
  return 1;
}

static int msg_invite(char *nick, char *host, struct userrec *u, char *par)
{
  char *pass;
  struct chanset_t *chan;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (match_my_nick(nick))
    return 1;
  pass = newsplit(&par);
  if (u_pass_match(u, pass) && !u_pass_match(u, "-")) {
    if (par[0] == '*') {
      for (chan = chanset; chan; chan = chan->next) {
	get_user_flagrec(u, &fr, chan->dname);
	if ((chan_op(fr) || glob_op(fr)) && (chan->channel.mode & CHANINV))
	  dprintf(DP_SERVER, "INVITE %s %s\n", nick, chan->name);
      }
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! INVITE ALL", nick, host,
	     u->handle);
      return 1;
    }
    if (!(chan = findchan_by_dname(par))) {
      dprintf(DP_HELP, "NOTICE %s :%s: /MSG %s invite <pass> <channel>\n",
	      nick, _("Usage"), botname);
      return 1;
    }
    if (!channel_active(chan)) {
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par, _("Not on that channel right now."));
      return 1;
    }
    /* We need to check access here also (dw 991002) */
    get_user_flagrec(u, &fr, par);
    if (chan_op(fr) || glob_op(fr)) {
      dprintf(DP_SERVER, "INVITE %s %s\n", nick, chan->name);
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! INVITE %s", nick, host,
	     u->handle, par);
      return 1;
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed INVITE %s", nick, host,
	 (u ? u->handle : "*"), par);
  return 1;
}

static int msg_status(char *nick, char *host, struct userrec *u, char *par)
{
  char s[256];
  char *ve_t, *un_t;
  char *pass;
  int i, l;
  struct chanset_t *chan;
#ifdef HAVE_UNAME
  struct utsname un;

  if (uname(&un) >= 0) {
#endif
    ve_t = " ";
    un_t = "*unknown*";
#ifdef HAVE_UNAME
  } else {
    ve_t = un.release;
    un_t = un.sysname;
  }
#endif

  if (match_my_nick(nick))
    return 1;
  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed STATUS", nick, host,
	     u->handle);
      return 1;
    }
  } else {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed STATUS", nick, host, u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You dont have a password set."));
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! STATUS", nick, host, u->handle);
  dprintf(DP_HELP, "NOTICE %s :I am %s, running %s.\n", nick, myname, Version);
  dprintf(DP_HELP, "NOTICE %s :Running on %s %s\n", nick, un_t, ve_t);
  if (admin[0])
    dprintf(DP_HELP, "NOTICE %s :Admin: %s\n", nick, admin);
  /* Fixed previous lame code. Well it's still lame, will overflow the
   * buffer with a long channel-name. <cybah>
   */
  strcpy(s, "Channels: ");
  l = 10;
  for (chan = chanset; chan; chan = chan->next) {
    l += my_strcpy(s + l, chan->dname);
    if (!channel_active(chan))
      l += my_strcpy(s + l, " (trying)");
    else if (channel_pending(chan))
      l += my_strcpy(s + l, " (pending)");
    else if (!any_ops(chan))
      l += my_strcpy(s + l, " (opless)");
    else if (!me_op(chan))
      l += my_strcpy(s + l, " (want ops!)");
    s[l++] = ',';
    s[l++] = ' ';
    if (l > 70) {
      s[l] = 0;
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, s);
      strcpy(s, "          ");
      l = 10;
    }
  }
  if (l > 10) {
    s[l] = 0;
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, s);
  }
  i = count_users(userlist);
  dprintf(DP_HELP, "NOTICE %s :%d user%s\n", nick, i, i == 1 ? "" : "s");
  daysdur(now, server_online, s);
  dprintf(DP_HELP, "NOTICE %s :Connected %s\n", nick, s);
  dprintf(DP_HELP, "NOTICE %s :Online as: %s%s%s\n", nick, botname,
	  botuserhost[0] ? "!" : "", botuserhost[0] ? botuserhost : "");
  return 1;
}

static int msg_die(char *nick, char *host, struct userrec *u, char *par)
{
  char s[1024];
  char *pass;

  if (match_my_nick(nick))
    return 1;
  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed DIE", nick, host, u->handle);
      return 1;
    }
  } else {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed DIE", nick, host, u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You dont have a password set."));
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! DIE", nick, host, u->handle);
  dprintf(-serv, "NOTICE %s :%s\n", nick, _("Bot shut down beginning...."));
  if (!par[0])
    snprintf(s, sizeof s, "BOT SHUTDOWN (authorized by %s)", u->handle);
  else
    snprintf(s, sizeof s, "BOT SHUTDOWN (%s: %s)", u->handle, par);
  chatout("*** %s\n", s);
  if (!par[0])
    nuke_server(nick);
  else
    nuke_server(par);
  write_userfile(-1);
  sleep(1);			/* Give the server time to understand */
  snprintf(s, sizeof s, "DEAD BY REQUEST OF %s!%s", nick, host);
  fatal(s, 0);
  return 1;
}

static int msg_rehash(char *nick, char *host, struct userrec *u, char *par)
{
  if (match_my_nick(nick))
    return 1;
  if (u_pass_match(u, par)) {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! REHASH", nick, host, u->handle);
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Rehashing..."));
    if (make_userfile)
      make_userfile = 0;
    write_userfile(-1);
    do_restart = -2;
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed REHASH", nick, host, u->handle);
  return 1;
}

static int msg_save(char *nick, char *host, struct userrec *u, char *par)
{
  if (match_my_nick(nick))
    return 1;
  if (u_pass_match(u, par)) {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! SAVE", nick, host, u->handle);
    dprintf(DP_HELP, "NOTICE %s :Saving user file...\n", nick);
    write_userfile(-1);
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed SAVE", nick, host, u->handle);
  return 1;
}

static int msg_reset(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;

  if (match_my_nick(nick))
    return 1;
  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed RESET", nick, host,
	     u->handle);
      return 1;
    }
  } else {
    putlog(LOG_CMDS, "*", "(%s!%s) !*! failed RESET", nick, host);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You dont have a password set."));
    return 1;
  }
  if (par[0]) {
    chan = findchan_by_dname(par);
    if (!chan) {
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par, _("I dont monitor that channel."));
      return 0;
    }
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! RESET %s", nick, host, u->handle, par);
    dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par, _("Resetting channel info."));
    reset_chan_info(chan);
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! RESET ALL", nick, host, u->handle);
  dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Resetting channel info."));
  for (chan = chanset; chan; chan = chan->next)
    reset_chan_info(chan);
  return 1;
}

static int msg_jump(char *nick, char *host, struct userrec *u, char *par)
{
  char *s;
  int port;

  if (match_my_nick(nick))
    return 1;
  if (u_pass_match(u, "-")) {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed JUMP", nick, host, u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You dont have a password set."));
    return 1;
  }
  s = newsplit(&par);		/* Password */
  if (u_pass_match(u, s)) {
    if (par[0]) {
      s = newsplit(&par);
      port = atoi(newsplit(&par));
      if (!port)
	port = default_port;
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! JUMP %s %d %s", nick, host,
	     u->handle, s, port, par);
      strcpy(newserver, s);
      newserverport = port;
      strcpy(newserverpass, par);
    } else
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! JUMP", nick, host, u->handle);
    dprintf(-serv, "NOTICE %s :%s\n", nick, _("Jumping servers..."));
    cycle_time = 0;
    nuke_server("changing servers");
  } else
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed JUMP", nick, host, u->handle);
  return 1;
}

/* MSG COMMANDS
 *
 * Function call should be:
 *    int msg_cmd("handle","nick","user@host","params");
 *
 * The function is responsible for any logging. Return 1 if successful,
 * 0 if not.
 */
static cmd_t C_msg[] =
{
  {"die",		"n",	(Function) msg_die,		NULL},
  {"hello",		"",	(Function) msg_hello,		NULL},
  {"help",		"",	(Function) msg_help,		NULL},
  {"ident",		"",	(Function) msg_ident,		NULL},
  {"info",		"",	(Function) msg_info,		NULL},
  {"invite",		"o|o",	(Function) msg_invite,		NULL},
  {"jump",		"m",	(Function) msg_jump,		NULL},
  {"key",		"o|o",	(Function) msg_key,		NULL},
  {"op",		"",	(Function) msg_op,		NULL},
  {"pass",		"",	(Function) msg_pass,		NULL},
  {"rehash",		"m",	(Function) msg_rehash,		NULL},
  {"reset",		"m",	(Function) msg_reset,		NULL},
  {"save",		"m",	(Function) msg_save,		NULL},
  {"status",		"m|m",	(Function) msg_status,		NULL},
  {"voice",		"",	(Function) msg_voice,		NULL},
  {"who",		"",	(Function) msg_who,		NULL},
  {"whois",		"",	(Function) msg_whois,		NULL},
  {NULL,		NULL,	NULL,				NULL}
};
