/*
 * userchan.c --
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
static const char rcsid[] = "$Id: userchan.c,v 1.16 2003/02/15 09:07:15 wcc Exp $";
#endif
*/

struct chanuserrec *get_chanrec(struct userrec *u, char *chname)
{
  struct chanuserrec *ch;

  for (ch = u->chanrec; ch; ch = ch->next) 
    if (!irccmp(ch->channel, chname))
      return ch;
  return NULL;
}

static struct chanuserrec *add_chanrec(struct userrec *u, char *chname)
{
  struct chanuserrec *ch = NULL;

  if (findchan_by_dname(chname)) {
    ch = malloc(sizeof(struct chanuserrec));

    ch->next = u->chanrec;
    u->chanrec = ch;
    ch->info = NULL;
    ch->flags = 0;
    ch->flags_udef = 0;
    ch->laston = 0;
    strncpy(ch->channel, chname, 81);
    ch->channel[80] = 0;
  }
  return ch;
}

static void add_chanrec_by_handle(struct userrec *bu, char *hand, char *chname)
{
  struct userrec *u;

  u = get_user_by_handle(bu, hand);
  if (!u)
    return;
  if (!get_chanrec(u, chname))
    add_chanrec(u, chname);
}

static void get_handle_chaninfo(char *handle, char *chname, char *s)
{
  struct userrec *u;
  struct chanuserrec *ch;

  u = get_user_by_handle(userlist, handle);
  if (u == NULL) {
    s[0] = 0;
    return;
  }
  ch = get_chanrec(u, chname);
  if (ch == NULL) {
    s[0] = 0;
    return;
  }
  if (ch->info == NULL) {
    s[0] = 0;
    return;
  }
  strcpy(s, ch->info);
  return;
}

static void set_handle_chaninfo(struct userrec *bu, char *handle,
				char *chname, char *info)
{
  struct userrec *u;
  struct chanuserrec *ch;
  struct chanset_t *cst;

  u = get_user_by_handle(bu, handle);
  if (!u)
    return;
  ch = get_chanrec(u, chname);
  if (!ch) {
    add_chanrec_by_handle(bu, handle, chname);
    ch = get_chanrec(u, chname);
  }
  if (info)
    if (strlen(info) > 80)
      info[80] = 0;
  if (ch->info != NULL)
    free(ch->info);
  if (info && info[0])
    ch->info = strdup(info);
  else
    ch->info = NULL;
  cst = findchan_by_dname(chname);
}

static void del_chanrec(struct userrec *u, char *chname)
{
  struct chanuserrec *ch = u->chanrec, *lst = NULL;

  while (ch) {
    if (!irccmp(chname, ch->channel)) {
      if (lst == NULL)
	u->chanrec = ch->next;
      else
	lst->next = ch->next;
      if (ch->info != NULL)
	free(ch->info);
      free(ch);
      return;
    }
    lst = ch;
    ch = ch->next;
  }
}

static void set_handle_laston(char *chan, struct userrec *u, time_t n)
{
  struct chanuserrec *ch;

  if (!u)
    return;
  touch_laston(u, chan, n);
  ch = get_chanrec(u, chan);
  if (!ch)
    return;
  ch->laston = n;
}

/* Is this mask sticky?
 */
static int u_sticky_mask(maskrec *u, char *uhost)
{
  for (; u; u = u->next)
    if (!irccmp(u->mask, uhost))
      return (u->flags & MASKREC_STICKY);
  return 0;
}

/* Set sticky attribute for a mask.
 */
static int u_setsticky_mask(int type, struct chanset_t *chan, char *uhost,
			    int sticky)
{
  int j;
  maskrec *u;
  char *botcmd;

  if (type == 'b') {
    if (chan) u = chan->bans;
    else u = global_bans;
    botcmd = "s";
  }
  else if (type == 'I') {
    if (chan) u = chan->invites;
    else u = global_invites;
    botcmd = "sInv";
  }
  else if (type == 'e') {
    if (chan) u = chan->exempts;
    else u = global_exempts;
    botcmd = "se";
  }
  else return(-1);

  j = atoi(uhost);
  if (!j)
    j = (-1);
  while(u) {
    if (j >= 0)
      j--;

    if (!j || ((j < 0) && !irccmp(u->mask, uhost))) {
      if (sticky > 0)
	u->flags |= MASKREC_STICKY;
      else if (!sticky)
	u->flags &= ~MASKREC_STICKY;
      else	/* We don't actually want to change, just skip over */
	return 0;
      if (!j)
	strcpy(uhost, u->mask);
      return 1;
    }

    u = u->next;
  }
  if (j >= 0)
    return -j;

  return 0;
}

/* Merge of u_equals_ban(), u_equals_exempt() and u_equals_invite().
 *
 * Returns:
 *   0       not a ban
 *   1       temporary ban
 *   2       perm ban
 */
static int u_equals_mask(maskrec *u, char *mask)
{
  for (; u; u = u->next)
    if (!irccmp(u->mask, mask)) {
      if (u->flags & MASKREC_PERM)
        return 2;
      else
        return 1;
    }
  return 0;
}

static int u_match_mask(maskrec *rec, char *mask)
{
  for (; rec; rec = rec->next)
    if (wild_match(rec->mask, mask))
      return 1;
  return 0;
}

static int u_delmask(char type, struct chanset_t *c, char *who, int doit)
{
  int j, i = 0;
  maskrec **u = NULL, *t;
  char temp[256];

  if (type == 'b')
    u = c ? &c->bans : &global_bans;
  if (type == 'e')
    u = c ? &c->exempts : &global_exempts;
  if (type == 'I')
    u = c ? &c->invites : &global_invites;

  if (!strchr(who, '!') && (j = atoi(who))) {
    j--;
    for (; (*u) && j; u = &((*u)->next), j--);
    if (*u) {
      strncpyz(temp, (*u)->mask, sizeof temp);
      i = 1;
    } else
      return -j - 1;
  } else {
    /* Find matching host, if there is one */
    for (; *u && !i; u = &((*u)->next))
      if (!irccmp((*u)->mask, who)) {
        strncpyz(temp, who, sizeof temp);
	i = 1;
	break;
      }
    if (!*u)
      return 0;
  }
  if (i && doit) {
    free((*u)->mask);
    if ((*u)->desc)
      free((*u)->desc);
    if ((*u)->user)
      free((*u)->user);
    t = *u;
    *u = (*u)->next;
    free(t);
  }
  return i;
}

/* Note: If first char of note is '*' it's a sticky mask.
 */
static int u_addmask(char type, struct chanset_t *chan, char *who, char *from,
		     char *note, time_t expire_time, int flags)
{
  char host[1024], s[1024];
  maskrec *p = NULL, *l, **u = NULL;
  module_entry *me;

  if (type == 'b')
    u = chan ? &chan->bans : &global_bans;
  if (type == 'e')
    u = chan ? &chan->exempts : &global_exempts;
  if (type == 'I')
    u = chan ? &chan->invites : &global_invites;

  strncpy(host, who, 256);
  host[256] = 0;

  /* Choke check: fix broken bans (must have '!' and '@') */
  if ((strchr(host, '!') == NULL) && (strchr(host, '@') == NULL))
    strcat(host, "!*@*");
  else if (strchr(host, '@') == NULL)
    strcat(host, "@*");
  else if (strchr(host, '!') == NULL) {
    char *i = strchr(host, '@');

    strcpy(s, i);
    *i = 0;
    strcat(host, "!*");
    strcat(host, s);
  }
  if ((me = module_find("server", 0, 0)) && me->funcs)
    simple_sprintf(s, "%s!%s", me->funcs[SERVER_BOTNAME],
		   me->funcs[SERVER_BOTUSERHOST]);
  else
    s[0] = 0;
  if (s[0] && type == 'b' && wild_match(host, s)) {
    putlog(LOG_MISC, "*", _("Wanted to ban myself--deflected."));
    return 0;
  }
  if (expire_time == now)
    return 1;

  for (l = *u; l; l = l->next)
    if (!irccmp(l->mask, host)) {
      p = l;
      break;
    }
			
  /* It shouldn't expire and be sticky also */
  if (note[0] == '*') {
    flags |= MASKREC_STICKY;
    note++;
  }
  if ((expire_time == 0L) || (flags & MASKREC_PERM)) {
    flags |= MASKREC_PERM;
    expire_time = 0L;
  }

  if (p == NULL) {
    p = malloc(sizeof(maskrec));
    p->next = *u;
    *u = p;
  } else {
    free(p->mask);
    free(p->user);
    free(p->desc);
  }
  p->expire = expire_time;
  p->added = now;
  p->lastactive = 0;
  p->flags = flags;
  p->mask = strdup(host);
  p->user = strdup(from);
  p->desc = strdup(note);
  return 1;
}

/* Take host entry from ban list and display it ban-style.
 */
static void display_ban(int idx, int number, maskrec *ban,
			struct chanset_t *chan, int show_inact)
{
  char dates[81], s[41];

  if (ban->added) {
    daysago(now, ban->added, s);
    sprintf(dates, "%s %s", _("Created"), s);
    if (ban->added < ban->lastactive) {
      strcat(dates, ", ");
      strcat(dates, _("last used"));
      strcat(dates, " ");
      daysago(now, ban->lastactive, s);
      strcat(dates, s);
    }
  } else
    dates[0] = 0;
  if (ban->flags & MASKREC_PERM)
    strcpy(s, "(perm)");
  else {
    char s1[41];

    days(ban->expire, now, s1);
    sprintf(s, "(expires %s)", s1);
  }
  if (ban->flags & MASKREC_STICKY)
    strcat(s, " (sticky)");
  if (!chan || ischanban(chan, ban->mask)) {
    if (number >= 0) {
      dprintf(idx, "  [%3d] %s %s\n", number, ban->mask, s);
    } else {
      dprintf(idx, "BAN: %s %s\n", ban->mask, s);
    }
  } else if (show_inact) {
    if (number >= 0) {
      dprintf(idx, "! [%3d] %s %s\n", number, ban->mask, s);
    } else {
      dprintf(idx, "BAN (%s): %s %s\n", _("inactive"), ban->mask, s);
    }
  } else
    return;
  dprintf(idx, "        %s: %s\n", ban->user, ban->desc);
  if (dates[0])
    dprintf(idx, "        %s\n", dates);
}

/* Take host entry from exempt list and display it ban-style.
 */
static void display_exempt(int idx, int number, maskrec *exempt,
			   struct chanset_t *chan, int show_inact)
{
  char dates[81], s[41];

  if (exempt->added) {
    daysago(now, exempt->added, s);
    sprintf(dates, "%s %s", _("Created"), s);
    if (exempt->added < exempt->lastactive) {
      strcat(dates, ", ");
      strcat(dates, _("last used"));
      strcat(dates, " ");
      daysago(now, exempt->lastactive, s);
      strcat(dates, s);
    }
  } else
    dates[0] = 0;
  if (exempt->flags & MASKREC_PERM)
    strcpy(s, "(perm)");
  else {
    char s1[41];

    days(exempt->expire, now, s1);
    sprintf(s, "(expires %s)", s1);
  }
  if (exempt->flags & MASKREC_STICKY)
    strcat(s, " (sticky)");
  if (!chan || ischanexempt(chan, exempt->mask)) {
    if (number >= 0) {
      dprintf(idx, "  [%3d] %s %s\n", number, exempt->mask, s);
    } else {
      dprintf(idx, "EXEMPT: %s %s\n", exempt->mask, s);
    }
  } else if (show_inact) {
    if (number >= 0) {
      dprintf(idx, "! [%3d] %s %s\n", number, exempt->mask, s);
    } else {
      dprintf(idx, "EXEMPT (%s): %s %s\n", _("inactive"), exempt->mask, s);
    }
  } else
    return;
  dprintf(idx, "        %s: %s\n", exempt->user, exempt->desc);
  if (dates[0])
    dprintf(idx, "        %s\n", dates);
}

/* Take host entry from invite list and display it ban-style.
 */
static void display_invite (int idx, int number, maskrec *invite,
			    struct chanset_t *chan, int show_inact)
{
  char dates[81], s[41];

  if (invite->added) {
    daysago(now, invite->added, s);
    sprintf(dates, "%s %s", _("Created"), s);
    if (invite->added < invite->lastactive) {
      strcat(dates, ", ");
      strcat(dates, _("last used"));
      strcat(dates, " ");
      daysago(now, invite->lastactive, s);
      strcat(dates, s);
    }
  } else
    dates[0] = 0;
  if (invite->flags & MASKREC_PERM)
    strcpy(s, "(perm)");
  else {
    char s1[41];

    days(invite->expire, now, s1);
    sprintf(s, "(expires %s)", s1);
  }
  if (invite->flags & MASKREC_STICKY)
    strcat(s, " (sticky)");
  if (!chan || ischaninvite(chan, invite->mask)) {
    if (number >= 0) {
      dprintf(idx, "  [%3d] %s %s\n", number, invite->mask, s);
    } else {
      dprintf(idx, "INVITE: %s %s\n", invite->mask, s);
    }
  } else if (show_inact) {
    if (number >= 0) {
      dprintf(idx, "! [%3d] %s %s\n", number, invite->mask, s);
    } else {
      dprintf(idx, "INVITE (%s): %s %s\n", _("inactive"), invite->mask, s);
    }
  } else
    return;
  dprintf(idx, "        %s: %s\n", invite->user, invite->desc);
  if (dates[0])
    dprintf(idx, "        %s\n", dates);
}

static void tell_bans(int idx, int show_inact, char *match)
{
  char *p = NULL, *chname;
  int k = 1;
  struct chanset_t *chan = NULL;
  maskrec *u;

  /* Was a channel given? */
  if (match[0]) {
    p = strdup(match);
    chname = strtok(p, " ");
    if (chname && (strchr(CHANMETA, chname[0]))) {
      chan = findchan_by_dname(chname);
      if (!chan) {
	dprintf(idx, "%s.\n", _("No such channel defined"));
	return;
      }
    } else
      match = chname;
    if (p)
      free_null(p);
  }

  /* don't return here, we want to show global bans even if no chan */
  if (!chan && !(chan = findchan_by_dname(dcc[idx].u.chat->con_chan)) &&
      !(chan = chanset))
    chan = NULL;

  if (chan && show_inact)
    dprintf(idx, "%s:   (! = %s %s)\n", _("Global bans"),
	    _("not active on"), chan->dname);
  else
    dprintf(idx, "%s:\n", _("Global bans"));
  for (u = global_bans; u; u = u->next) {
    if (match[0]) {
      if ((wild_match(match, u->mask)) ||
	  (wild_match(match, u->desc)) ||
	  (wild_match(match, u->user)))
	display_ban(idx, k, u, chan, 1);
      k++;
    } else
      display_ban(idx, k++, u, chan, show_inact);
  }
  if (chan) {
    if (show_inact)
      dprintf(idx, _("Channel bans for %s:   (! = not active, * = not placed by bot)\n"),
	      chan->dname);
    else
      dprintf(idx, _("Channel bans for %s:  (* = not placed by bot)\n"),
	      chan->dname);
    for (u = chan->bans; u; u = u->next) {
      if (match[0]) {
        if ((wild_match(match, u->mask)) ||
  	    (wild_match(match, u->desc)) ||
	    (wild_match(match, u->user)))
	  display_ban(idx, k, u, chan, 1);
	k++;
      } else
	display_ban(idx, k++, u, chan, show_inact);
    }
    if (chan->status & CHAN_ACTIVE) {
      masklist *b;
      /* FIXME: possible buffer overflow in fill[] */
      char buf[UHOSTLEN], *nick, *uhost, fill[UHOSTLEN * 2];
      int min, sec;

      for (b = chan->channel.ban; b && b->mask[0]; b = b->next) {    
	if ((!u_equals_mask(global_bans, b->mask)) &&
	    (!u_equals_mask(chan->bans, b->mask))) {
	  strlcpy(buf, b->who, sizeof buf);
	  nick = strtok(buf, "!");
	  uhost = strtok(NULL, "!");
	  if (nick)
	    sprintf(fill, "%s (%s!%s)", b->mask, nick, uhost);
	  else
	    sprintf(fill, "%s (server %s)", b->mask, uhost);
	  if (b->timer != 0) {
	    min = (now - b->timer) / 60;
	    sec = (now - b->timer) - (min * 60);
	    sprintf(buf, " (active %02d:%02d)", min, sec);
	    strcat(fill, buf);
	  }
	  if ((!match[0]) || (wild_match(match, b->mask)))
	    dprintf(idx, "* [%3d] %s\n", k, fill);
	  k++;
	}
      }
    }
  }
  if (k == 1)
    dprintf(idx, _("(There are no bans, permanent or otherwise.)\n"));
  if ((!show_inact) && (!match[0]))
    dprintf(idx, _("Use .bans all to see the total list.\n"));
}

static void tell_exempts(int idx, int show_inact, char *match)
{
  char *p = NULL, *chname;
  int k = 1;
  struct chanset_t *chan = NULL;
  maskrec *u;

  /* Was a channel given? */
  if (match[0]) {
    p = strdup(match);
    chname = strtok(p, " ");
    if (chname && (strchr(CHANMETA, chname[0]))) {
      chan = findchan_by_dname(chname);
      if (!chan) {
	dprintf(idx, _("No such channel defined.\n"));
	return;
      }
    } else
      match = chname;
    if (p)
      free_null(p);
  }

  /* don't return here, we want to show global exempts even if no chan */
  if (!chan && !(chan = findchan_by_dname(dcc[idx].u.chat->con_chan))
      && !(chan = chanset))
    chan = NULL;

  if (chan && show_inact)
    dprintf(idx, "%s:   (! = %s %s)\n", _("Global exempts"),
	    _("not active on"), chan->dname);
  else
    dprintf(idx, _("Global exempts:\n"));
  for (u = global_exempts; u; u = u->next) {
    if (match[0]) {
      if ((wild_match(match, u->mask)) ||
	  (wild_match(match, u->desc)) ||
	  (wild_match(match, u->user)))
	display_exempt(idx, k, u, chan, 1);
      k++;
    } else
      display_exempt(idx, k++, u, chan, show_inact);
  }
  if (chan) {
    if (show_inact)
      dprintf(idx, "%s %s:   (! = %s, * = %s)\n",
	      _("Channel exempts for"), chan->dname,
	      _("not active"),
	      _("not placed by bot"));
    else
      dprintf(idx, "%s %s:  (* = %s)\n",
	      _("Channel exempts for"), chan->dname,
	      _("not placed by bot"));
    for (u = chan->exempts; u; u = u->next) {
      if (match[0]) {
	if ((wild_match(match, u->mask)) ||
	    (wild_match(match, u->desc)) ||
	    (wild_match(match, u->user)))
	  display_exempt(idx, k, u, chan, 1);
	k++;
      } else
	display_exempt(idx, k++, u, chan, show_inact);
    }
    if (chan->status & CHAN_ACTIVE) {
      masklist *e;
      /* FIXME: possible buffer overflow in fill[] */
      char buf[UHOSTLEN], *nick, *uhost, fill[UHOSTLEN * 2];
      int min, sec;

      for (e = chan->channel.exempt; e && e->mask[0]; e = e->next) {
	if ((!u_equals_mask(global_exempts,e->mask)) &&
	    (!u_equals_mask(chan->exempts, e->mask))) {
	  strlcpy(buf, e->who, sizeof buf);
	  nick = strtok(buf, "!");
	  uhost = strtok(NULL, "!");
	  if (nick)
	    sprintf(fill, "%s (%s!%s)", e->mask, nick, uhost);
	  else
	    sprintf(fill, "%s (server %s)", e->mask, uhost);
	  if (e->timer != 0) {
	    min = (now - e->timer) / 60;
	    sec = (now - e->timer) - (min * 60);
	    sprintf(buf, " (active %02d:%02d)", min, sec);
	    strcat(fill, buf);
	  }
	  if ((!match[0]) || (wild_match(match, e->mask)))
	    dprintf(idx, "* [%3d] %s\n", k, fill);
	  k++;
	}
      }
    }
  }
  if (k == 1)
    dprintf(idx, "(There are no ban exempts, permanent or otherwise.)\n");
  if ((!show_inact) && (!match[0]))
    dprintf(idx, "%s.\n", _("Use .exempts all to see the total list"));
}

static void tell_invites(int idx, int show_inact, char *match)
{
  char *p = NULL, *chname;
  int k = 1;
  struct chanset_t *chan = NULL;
  maskrec *u;

  /* Was a channel given? */
  if (match[0]) {
    p = strdup(match);
    chname = strtok(p, " ");
    if (chname && (strchr(CHANMETA, chname[0]))) {
      chan = findchan_by_dname(chname);
      if (!chan) {
	dprintf(idx, "%s.\n", _("No such channel defined"));
	return;
      }
    } else
      match = chname;
    if (p)
      free_null(p);
  }

  /* don't return here, we want to show global invites even if no chan */
  if (!chan && !(chan = findchan_by_dname(dcc[idx].u.chat->con_chan))
      && !(chan = chanset))
    chan = NULL;

  if (chan && show_inact)
    dprintf(idx, "%s:   (! = %s %s)\n", _("Global invites"),
	    _("not active on"), chan->dname);
  else
    dprintf(idx, "%s:\n", _("Global invites"));
  for (u = global_invites; u; u = u->next) {
    if (match[0]) {
      if ((wild_match(match, u->mask)) ||
	  (wild_match(match, u->desc)) ||
	  (wild_match(match, u->user)))
	display_invite(idx, k, u, chan, 1);
      k++;
    } else
      display_invite(idx, k++, u, chan, show_inact);
  }
  if (chan) {
    if (show_inact)
      dprintf(idx, "%s %s:   (! = %s, * = %s)\n",
	      _("Channel invites for"), chan->dname,
	      _("not active"),
	      _("not placed by bot"));
    else
      dprintf(idx, "%s %s:  (* = %s)\n",
	      _("Channel invites for"), chan->dname,
	      _("not placed by bot"));
    for (u = chan->invites; u; u = u->next) {
      if (match[0]) {
	if ((wild_match(match, u->mask)) ||
	    (wild_match(match, u->desc)) ||
	    (wild_match(match, u->user)))
	  display_invite(idx, k, u, chan, 1);
	k++;
      } else
	display_invite(idx, k++, u, chan, show_inact);
    }
    if (chan->status & CHAN_ACTIVE) {
      masklist *i;
      /* FIXME: possible buffer overflow in fill[] */
      char buf[UHOSTLEN], *nick, *uhost, fill[UHOSTLEN * 2];
      int min, sec;

      for (i = chan->channel.invite; i && i->mask[0]; i = i->next) {
	if ((!u_equals_mask(global_invites,i->mask)) &&
	    (!u_equals_mask(chan->invites, i->mask))) {
	  strlcpy(buf, i->who, sizeof buf);
	  nick = strtok(buf, "!");
	  uhost = strtok(NULL, "!");
	  if (nick)
	    sprintf(fill, "%s (%s!%s)", i->mask, nick, uhost);
	  else
	    sprintf(fill, "%s (server %s)", i->mask, uhost);
	  if (i->timer != 0) {
	    min = (now - i->timer) / 60;
	    sec = (now - i->timer) - (min * 60);
	    sprintf(buf, " (active %02d:%02d)", min, sec);
	    strcat(fill, buf);
	  }
	  if ((!match[0]) || (wild_match(match, i->mask)))
	    dprintf(idx, "* [%3d] %s\n", k, fill);
	  k++;
	}
      }
    }
  }
  if (k == 1)
    dprintf(idx, "(There are no invites, permanent or otherwise.)\n");
  if ((!show_inact) && (!match[0]))
    dprintf(idx, "%s.\n", _("Use .invites all to see the total list"));
}

/* Write the ban lists and the ignore list to a file. */
static int write_bans(FILE *f, int idx)
{
  struct chanset_t *chan;
  maskrec *b;
  char	*mask;

  if (global_bans)
    if (fprintf(f, BAN_NAME " - -\n") == EOF)	/* Daemus */
      return 0;
  for (b = global_bans; b; b = b->next) {
    mask = str_escape(b->mask, ':', '\\');
    if (!mask ||
	fprintf(f, "- %s:%s%lu%s:+%lu:%lu:%s:%s\n", mask,
		(b->flags & MASKREC_PERM) ? "+" : "", b->expire,
		(b->flags & MASKREC_STICKY) ? "*" : "", b->added,
		b->lastactive, b->user ? b->user : myname,
		b->desc ? b->desc : "requested") == EOF) {
      if (mask)
	free(mask);
      return 0;
    }
    free(mask);
  }
  for (chan = chanset; chan; chan = chan->next)
    if (idx < 0) {
      struct flag_record fr = {FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0};

      if (idx >= 0)
	get_user_flagrec(dcc[idx].user, &fr, chan->dname);
    }
  return 1;
}

/* Write the exemptlists to a file.
 */
static int write_exempts(FILE *f, int idx)
{
  struct chanset_t *chan;
  maskrec *e;
  char	*mask;

  if (global_exempts)
    if (fprintf(f, EXEMPT_NAME " - -\n") == EOF) /* Daemus */
      return 0;
  for (e = global_exempts; e; e = e->next) {
    mask = str_escape(e->mask, ':', '\\');
    if (!mask ||
	fprintf(f, "%s %s:%s%lu%s:+%lu:%lu:%s:%s\n", "%", e->mask,
		(e->flags & MASKREC_PERM) ? "+" : "", e->expire,
		(e->flags & MASKREC_STICKY) ? "*" : "", e->added,
		e->lastactive, e->user ? e->user : myname,
		e->desc ? e->desc : "requested") == EOF) {
      if (mask)
	free(mask);
      return 0;
    }
    free(mask);
  }
  for (chan = chanset;chan;chan=chan->next)
    if (idx < 0) {
      struct flag_record fr = {FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0};

      if (idx >= 0)
	get_user_flagrec(dcc[idx].user,&fr,chan->dname);
    }
  return 1;
}

/* Write the invitelists to a file.
 */
static int write_invites(FILE *f, int idx)
{
  struct chanset_t *chan;
  maskrec *ir;
  char	*mask;

  if (global_invites)
    if (fprintf(f, INVITE_NAME " - -\n") == EOF) /* Daemus */
      return 0;
  for (ir = global_invites; ir; ir = ir->next)  {
    mask = str_escape(ir->mask, ':', '\\');
    if (!mask ||
	fprintf(f,"@ %s:%s%lu%s:+%lu:%lu:%s:%s\n",ir->mask,
		(ir->flags & MASKREC_PERM) ? "+" : "", ir->expire,
		(ir->flags & MASKREC_STICKY) ? "*" : "", ir->added,
		ir->lastactive, ir->user ? ir->user : myname,
		ir->desc ? ir->desc : "requested") == EOF) {
      if (mask)
	free(mask);
      return 0;
    }
    free(mask);
  }
  for (chan = chanset; chan; chan = chan->next)
    if (idx < 0) {
      struct flag_record fr = {FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0};

      if (idx >= 0)
	get_user_flagrec(dcc[idx].user,&fr,chan->dname);
    }
  return 1;
}

static void channels_writeuserfile(void)
{
  char	 s[1024];
  FILE	*f;
  int	 ret = 0;

  simple_sprintf(s, "%s~new", userfile);
  f = fopen(s, "a");
  if (f) {
    ret = write_bans(f, -1);
    ret += write_exempts(f, -1);
    ret += write_invites(f, -1);
    fclose(f);
  }
  if (ret < 3)
    putlog(LOG_MISC, "*", _("ERROR writing user file."));
  write_channels();
}

/* Expire mask originally set by `who' on `chan'?
 *
 * We might not want to expire masks in all cases, as other bots
 * often tend to immediately reset masks they've listed in their
 * internal ban list, making it quite senseless for us to remove
 * them in the first place.
 *
 * Returns 1 if a mask on `chan' by `who' may be expired and 0 if
 * not.
 */
static int expired_mask(struct chanset_t *chan, char *who)
{
  char buf[UHOSTLEN], *nick, *uhost;
  struct userrec *u;
  memberlist *m, *m2;

  /* Always expire masks, regardless of who set it? */
  if (force_expire)
    return 1;

  strlcpy(buf, who, sizeof buf);
  nick = strtok(buf, "!");
  uhost = strtok(NULL, "!");

  if (!nick)
    return 1;

  m = ismember(chan, nick);
  if (!m)
    for (m2 = chan->channel.member; m2 && m2->nick[0]; m2 = m2->next)
      if (!strcasecmp(uhost, m2->userhost)) {
	m = m2;
	break;
      }

  if (!m || !chan_hasop(m) || !irccmp(m->nick, botname))
    return 1;

  /* At this point we know the person/bot who set the mask is currently
   * present in the channel and has op.
   */

  if (m->user)
    u = m->user;
  else {
    simple_sprintf(buf, "%s!%s", m->nick, m->userhost);
    u = get_user_by_host(buf);
  }
  /* Do not expire masks set by bots. */
  if (u && u->flags & USER_BOT)
    return 0;
  else
    return 1;
}

/* Check for expired timed-bans.
 */
static void check_expired_bans(void)
{
  maskrec *u, *u2;
  struct chanset_t *chan;
  masklist *b;

  for (u = global_bans; u; u = u2) { 
    u2 = u->next;
    if (!(u->flags & MASKREC_PERM) && (now >= u->expire)) {
      putlog(LOG_MISC, "*", "%s %s (%s)", _("No longer banning"),
	     u->mask, _("expired"));
      for (chan = chanset; chan; chan = chan->next)
	for (b = chan->channel.ban; b->mask[0]; b = b->next)
	  if (!irccmp(b->mask, u->mask) &&
	      expired_mask(chan, b->who) && b->timer != now) {
	    add_mode(chan, '-', 'b', u->mask);
	    b->timer = now;
	  }
      u_delmask('b', NULL, u->mask, 1);
    }
  }
  /* Check for specific channel-domain bans expiring */
  for (chan = chanset; chan; chan = chan->next) {
    for (u = chan->bans; u; u = u2) {
      u2 = u->next;
      if (!(u->flags & MASKREC_PERM) && (now >= u->expire)) {
	putlog(LOG_MISC, "*", "%s %s %s %s (%s)", _("No longer banning"),
	       u->mask, _("on"), chan->dname, _("expired"));
	for (b = chan->channel.ban; b->mask[0]; b = b->next)
	  if (!irccmp(b->mask, u->mask) &&
	      expired_mask(chan, b->who) && b->timer != now) {
	    add_mode(chan, '-', 'b', u->mask);
	    b->timer = now;
	  }
	u_delmask('b', chan, u->mask, 1);
      }
    }
  }
}

/* Check for expired timed-exemptions
 */
static void check_expired_exempts(void)
{
  maskrec *u, *u2;
  struct chanset_t *chan;
  masklist *b, *e;
  int match;

  if (!use_exempts)
    return;
  for (u = global_exempts; u; u = u2) {
    u2 = u->next;
    if (!(u->flags & MASKREC_PERM) && (now >= u->expire)) {
      putlog(LOG_MISC, "*", "%s %s (%s)", _("No longer ban exempting"),
	     u->mask, _("expired"));
      for (chan = chanset; chan; chan = chan->next) {
        match = 0;
        b = chan->channel.ban;
        while (b->mask[0] && !match) {
          if (wild_match(b->mask, u->mask) ||
            wild_match(u->mask, b->mask))
            match = 1;
          else
            b = b->next;
        }
        if (match)
          putlog(LOG_MISC, chan->dname,
            "Exempt not expired on channel %s. Ban still set!",
            chan->dname);
	else
	  for (e = chan->channel.exempt; e->mask[0]; e = e->next)
	    if (!irccmp(e->mask, u->mask) &&
		expired_mask(chan, e->who) && e->timer != now) {
	      add_mode(chan, '-', 'e', u->mask);
	      e->timer = now;
	    }
      }
      u_delmask('e', NULL, u->mask,1);
    }
  }
  /* Check for specific channel-domain exempts expiring */
  for (chan = chanset; chan; chan = chan->next) {
    for (u = chan->exempts; u; u = u2) {
      u2 = u->next;
      if (!(u->flags & MASKREC_PERM) && (now >= u->expire)) {
        match=0;
        b = chan->channel.ban;
        while (b->mask[0] && !match) {
          if (wild_match(b->mask, u->mask) ||
            wild_match(u->mask, b->mask))
            match=1;
          else
            b = b->next;
        }
        if (match)
          putlog(LOG_MISC, chan->dname,
            "Exempt not expired on channel %s. Ban still set!",
            chan->dname);
        else {
          putlog(LOG_MISC, "*", "%s %s %s %s (%s)", _("No longer ban exempting"),
		 u->mask, _("on"), chan->dname, _("expired"));
	  for (e = chan->channel.exempt; e->mask[0]; e = e->next)
	    if (!irccmp(e->mask, u->mask) &&
		expired_mask(chan, e->who) && e->timer != now) {
	      add_mode(chan, '-', 'e', u->mask);
	      e->timer = now;
	    }
          u_delmask('e', chan, u->mask, 1);
        }
      }
    }
  }
}

/* Check for expired timed-invites.
 */
static void check_expired_invites(void)
{
  maskrec *u, *u2;
  struct chanset_t *chan;
  masklist *b;

  if (!use_invites)
    return;
  for (u = global_invites; u; u = u2) {
    u2 = u->next;
    if (!(u->flags & MASKREC_PERM) && (now >= u->expire)) {
      putlog(LOG_MISC, "*", "%s %s (%s)", _("No longer inviteing"),
	     u->mask, _("expired"));
      for (chan = chanset; chan; chan = chan->next)
	if (!(chan->channel.mode & CHANINV))
	  for (b = chan->channel.invite; b->mask[0]; b = b->next)
	    if (!irccmp(b->mask, u->mask) &&
		expired_mask(chan, b->who) && b->timer != now) {
	      add_mode(chan, '-', 'I', u->mask);
	      b->timer = now;
	    }
      u_delmask('I', NULL, u->mask,1);
    }
  }
  /* Check for specific channel-domain invites expiring */
  for (chan = chanset; chan; chan = chan->next) {
    for (u = chan->invites; u; u = u2) {
      u2 = u->next;
      if (!(u->flags & MASKREC_PERM) && (now >= u->expire)) {
	putlog(LOG_MISC, "*", "%s %s %s %s (%s)", _("No longer inviteing"),
	       u->mask, _("on"), chan->dname, _("expired"));
	if (!(chan->channel.mode & CHANINV))
	  for (b = chan->channel.invite; b->mask[0]; b = b->next)
	    if (!irccmp(b->mask, u->mask) &&
		expired_mask(chan, b->who) && b->timer != now) {
	      add_mode(chan, '-', 'I', u->mask);
	      b->timer = now;
	    }
	u_delmask('I', chan, u->mask, 1);
      }
    }
  }
}
