/*
 * scriptcmds.c -- Script API for irc module
 *
 * Streamlined by answer.
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
static const char rcsid[] = "$Id: scriptcmds.c,v 1.3 2003/02/10 00:09:08 wcc Exp $";
#endif
*/

static int script_chanlist (script_var_t *retval, int nargs, char *whichchan, char *whichflags);
static int script_botisop (script_var_t *retval, int nargs, char *whichchan);
static int script_ischanjuped (script_var_t *retval, char *whichchan);
static int script_botisvoice (script_var_t *retval, int nargs, char *whichchan);
static int script_botonchan (script_var_t *retval, int nargs, char *whichchan);
static int script_isop (script_var_t *retval, int nargs, char *whichnick, char *whichchan);
static int script_isvoice (script_var_t *retval, int nargs, char *whichnick, char *whichchan);
static int script_wasop (script_var_t *retval, char *whichnick, char *whichchan);
static int script_onchan (script_var_t *retval, int nargs, char *whichnick, char *whichchan);
static int script_handonchan (script_var_t *retval, int nargs, char *whichhand, char *whichchan);
static int script_ischanban (script_var_t *retval, char *whichmask, char *whichchan);
static int script_ischanexempt (script_var_t *retval, char *whichmask, char *whichchan);
static int script_ischaninvite (script_var_t *retval, char *whichmask, char *whichchan);
static int script_getchanhost (script_var_t *retval, int nargs, char *whichnick, char *whichchan);
static int script_onchansplit (script_var_t *retval, int nargs, char *whichnick, char *whichchan);
static int script_maskhost (script_var_t *retval, char *whichmask);
static int script_getchanidle (script_var_t *retval, char *whichnick, char *whichchan);
static int script_chanbans (script_var_t *retval, char *whichchan);
static int script_chanexempts (script_var_t *retval, char *whichchan);
static int script_chaninvites (script_var_t *retval, char *whichchan);
static int script_getchanmode (script_var_t *retval, char *whichchan);
static int script_getchanjoin (script_var_t *retval, char *whichnick, char *whichchan);
static int script_channame2dname (script_var_t *retval, char *whichchan);
static int script_chandname2name (script_var_t *retval, char *whichchan);
static int script_flushmode (script_var_t *retval, char *whichchan);
static int script_pushmode (script_var_t *retval, int nargs, char *whichchan, char *whichmode, char *supparg);
static int script_resetbans (script_var_t *retval, char *whichchan);
static int script_resetexempts (script_var_t *retval, char *whichchan);
static int script_resetinvites (script_var_t *retval, char *whichchan);
static int script_resetchan (script_var_t *retval, char *whichchan);
static int script_topic (script_var_t *retval, char *whichchan);
static int script_hand2nick (script_var_t *retval, int nargs, char *whichhand, char *whichchan);
static int script_nick2hand (script_var_t *retval, int nargs, char *whichnick, char *whichchan);
static int script_putkick (script_var_t *retval, int nargs, char *whichchan, char *nicklist, char *whichcomment);


static int script_chanlist (script_var_t *retval, int nargs, char *whichchan, char *whichflags)
{
  char* *ulist = NULL; /* ulist is an array of char* (one per user) */
  int f, nbusers = 0;
  memberlist *m;
  struct chanset_t *chan;
  struct flag_record plus = {FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0},
                     minus = {FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0},
                     user = {FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0};

  chan = findchan_by_dname(whichchan);
  if (!chan) {
    /* msprintf() comes from lib/egglib/msprintf.h */
    /* SCRIPT_FREE because mem has been allocated manually */
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }

  /* We prepare retval to be a list of strings */
  retval->type = SCRIPT_ARRAY | SCRIPT_STRING | SCRIPT_FREE;

  if (nargs == 1) {
    /* No flag restrictions so just whiz it thru quick */
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      nbusers++;
      ulist = (char* *) realloc(ulist, sizeof(char *) * nbusers);
      ulist[nbusers-1] = strdup(m->nick);
    }
    retval->value = (void *) ulist;
    retval->len = nbusers;
    return(1);
  }
  break_down_flags(whichflags, &plus, &minus);
  f = (minus.global || minus.udef_global ||
       minus.chan || minus.udef_chan || minus.bot);
  /* Return empty set if asked for flags but flags don't exist */
  if (!plus.global && !plus.udef_global &&
      !plus.chan && !plus.udef_chan && !plus.bot && !f)
    return(1);
  minus.match = plus.match ^ (FR_AND | FR_OR);

  for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
    get_user_flagrec(m->user, &user, whichchan);
    user.match = plus.match;
    if (flagrec_eq(&plus, &user)) {
      if (!f || !flagrec_eq(&minus, &user)) {
        nbusers++;
	ulist = (char* *) realloc(ulist, sizeof(char *) * nbusers);
	ulist[nbusers-1] = strdup(m->nick);
      }
    }
  }
  retval->value = (void *) ulist;
  retval->len = nbusers;
  return(1);
}

static int script_botisop (script_var_t *retval, int nargs, char *whichchan)
{
  struct chanset_t *chan, *thechan = NULL;
  retval->type = SCRIPT_INTEGER;

  if (nargs == 1) { /* A chan is passed */
    chan = findchan_by_dname(whichchan);
    thechan = chan;
    if (!thechan) {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
      return(0);
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if (me_op(chan)) {
      retval->value = (void *) 1;
      return(1);
    }
    chan = chan->next;
  }
  retval->value = (void *) 0;
  return(1);
}

static int script_ischanjuped (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;
  retval->type = SCRIPT_INTEGER;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  if (channel_juped(chan))
    retval->value = (void *) 1;
  else
    retval->value = (void *) 0;
  return(1);
}

static int script_botisvoice (script_var_t *retval, int nargs, char *whichchan)
{
  struct chanset_t *chan, *thechan = NULL;
  memberlist *mx;
  retval->type = SCRIPT_INTEGER;

  if (nargs == 1) {
    chan = findchan_by_dname(whichchan);
    thechan = chan;
    if (!thechan) {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
      return(0);
    }
  } else
   chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if ((mx = ismember(chan, botname)) && chan_hasvoice(mx)) {
      retval->value = (void *) 1;
       return(1);
    }
    chan = chan->next;
  }
  retval->value = (void *) 0;
  return(1);
}

static int script_botonchan (script_var_t *retval, int nargs, char *whichchan)
{
  struct chanset_t *chan, *thechan = NULL;
  retval->type = SCRIPT_INTEGER;

  if (nargs == 1) {
    chan = findchan_by_dname(whichchan);
    thechan = chan;
    if (!thechan) {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
      return(0);
    }
  } else
   chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if (ismember(chan, botname)) {
      retval->value = (void *) 1;
      return(1);
    }
    chan = chan->next;
  }
  retval->value = (void *) 0;
  return(1);
}

static int script_isop (script_var_t *retval, int nargs, char *whichnick, char *whichchan)
{
  struct chanset_t *chan, *thechan = NULL;
  memberlist *mx;
  retval->type = SCRIPT_INTEGER;

  if (nargs == 2) {
    chan = findchan_by_dname(whichchan);
    thechan = chan;
    if (!thechan) {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
      return(0);
    }
  } else
   chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if ((mx = ismember(chan, whichnick)) && chan_hasop(mx)) {
      retval->value = (void *) 1;
      return(1);
    }
    chan = chan->next;
  }
  retval->value = (void *) 0;
  return(1);
}

static int script_isvoice (script_var_t *retval, int nargs, char *whichnick, char *whichchan)
{
  struct chanset_t *chan, *thechan = NULL;
  memberlist *mx;
  retval->type = SCRIPT_INTEGER;

  if (nargs == 2) {
    chan = findchan_by_dname(whichchan);
    thechan = chan;
    if (!thechan) {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
      return(0);
    }
  } else
   chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if ((mx = ismember(chan, whichnick)) && chan_hasvoice(mx)) {
      retval->value = (void *) 1;
      return(1);
    }
    chan = chan->next;
  }
  retval->value = (void *) 0;
  return(1);
}

static int script_wasop (script_var_t *retval, char *whichnick, char *whichchan)
{
  struct chanset_t *chan;
  memberlist *mx;
  retval->type = SCRIPT_INTEGER;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  if ((mx = ismember(chan, whichnick)) && chan_wasop(mx))
    retval->value = (void *) 1;
  else
    retval->value = (void *) 0;
  return(1);
}


static int script_onchan (script_var_t *retval, int nargs, char *whichnick, char *whichchan)
{
  struct chanset_t *chan, *thechan = NULL;
  retval->type = SCRIPT_INTEGER;

  if (nargs == 2) {
    chan = findchan_by_dname(whichchan);
    thechan = chan;
    if (!chan) {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
      return(0);
    }
 } else
  chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if (ismember(chan, whichnick)) {
      retval->value = (void *) 1;
      return(1);
    }
    chan = chan->next;
  }
  retval->value = (void *) 0;
  return(1);
}

static int script_handonchan (script_var_t *retval, int nargs, char *whichhand, char *whichchan)
{
  struct chanset_t *chan, *thechan = NULL;
  memberlist *m;
  retval->type = SCRIPT_INTEGER;

  if (nargs == 2) {
    chan = findchan_by_dname(whichchan);
    thechan = chan;
    if (!thechan) {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
      return(0);
    }
  } else
   chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      if (m->user && !irccmp(m->user->handle, whichhand)) {
	retval->value = (void *) 1;
	return(1);
      }
    }
    chan = chan->next;
  }
  retval->value = (void *) 0;
  return(1);
}

static int script_ischanban (script_var_t *retval, char *whichmask, char *whichchan)
{
  struct chanset_t *chan;
  retval->type = SCRIPT_INTEGER;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  if (ischanban(chan, whichmask))
    retval->value = (void *) 1;
  else
    retval->value = (void *) 0;
  return(1);
}

static int script_ischanexempt (script_var_t *retval, char *whichmask, char *whichchan)
{
  struct chanset_t *chan;
  retval->type = SCRIPT_INTEGER;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  if (ischanexempt(chan, whichmask))
    retval->value = (void *) 1;
  else
    retval->value = (void *) 0;
  return(1);
}

static int script_ischaninvite (script_var_t *retval, char *whichmask, char *whichchan)
{

  struct chanset_t *chan;
  retval->type = SCRIPT_INTEGER;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  if (ischaninvite(chan, whichmask))
    retval->value = (void *) 1;
  else
    retval->value = (void *) 0;
  return(1);
}

static int script_getchanhost (script_var_t *retval, int nargs, char *whichnick, char *whichchan)
{
  struct chanset_t *chan, *thechan = NULL;
  memberlist *m;
  retval->type = SCRIPT_STRING;

  if (nargs == 2) {
    chan = findchan_by_dname(whichchan);
    thechan = chan;
    if (!thechan) {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
      return(0);
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    m = ismember(chan, whichnick);
    if (m) {
      retval->value = m->userhost;
      return(1);
    }
    chan = chan->next;
  }
  return(1);
}

static int script_onchansplit (script_var_t *retval, int nargs, char *whichnick, char *whichchan)
{
  struct chanset_t *chan, *thechan = NULL;
  memberlist *m;
  retval->type = SCRIPT_INTEGER;

  if (nargs == 2) {
    chan = findchan_by_dname(whichchan);
    thechan = chan;
    if (!thechan) {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
      return(0);
    }
  } else
   chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    m = ismember(chan, whichnick);
    if (m && chan_issplit(m)) {
      retval->value = (void *) 1;
      return(1);
    }
    chan = chan->next;
  }
  retval->value = (void *) 0;
  return(1);
}

static int script_maskhost (script_var_t *retval, char *whichmask)
{
  char new[121];
  retval->type = SCRIPT_STRING;
  maskban(whichmask, new);
  retval->value = (void *) new;
  return(1);
}

static int script_getchanidle (script_var_t *retval, char *whichnick, char *whichchan)
{
  memberlist *m;
  struct chanset_t *chan;
  int x;
  retval->type = SCRIPT_INTEGER;

  if (!(chan = findchan_by_dname(whichchan))) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  m = ismember(chan, whichnick);
  if (m) {
    x = (int) ((now - (m->last)) / 60);
    retval->value = (void *) x;
    return(1);
  }
  retval->value = (void *) 0;
  return(1);
}

static inline int script_chanmasks(script_var_t *ret, masklist *m)
{
  char work[11]; /* contains a string from a long int */
  script_var_t *sublist;

  /* We prepare return value to be a list */
  ret->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
  ret->len = 0;
  ret->value = NULL;

  for (; m && m->mask && m->mask[0]; m = m->next) {
    sprintf(work, "%lo", now - m->timer);

    /* since we use SCRIPT_VAR, we have to create a list of
    ** script_var_t*, each one is created by script_string. */
    sublist = script_list(3,
                          script_string(m->mask, -1),
                          script_string(m->who, -1),
                          script_string((char *) work, -1)
              );
    script_list_append(ret, sublist);
  }
  return(1);
}

static int script_chanbans (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }

  script_chanmasks(retval, chan->channel.ban);
  return(1);
}

static int script_chanexempts (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }

  script_chanmasks(retval, chan->channel.exempt);
  return(1);
}

static int script_chaninvites (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }

  script_chanmasks(retval, chan->channel.invite);
  return(1);
}

static int script_getchanmode (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;
  retval->type = SCRIPT_STRING;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }

  retval->value = getchanmode(chan);
  return(1);
}

static int script_getchanjoin (script_var_t *retval, char *whichnick, char *whichchan)
{
  struct chanset_t *chan;
  memberlist *m;

  /* m->joined is a long int unsigned */
  retval->type = SCRIPT_UNSIGNED;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  m = ismember(chan, whichnick);
  if (m == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("%s is not on %s", whichnick, whichchan);
    return(0);
  }

  retval->value = (void *) m->joined;
  return(1);
}

static int script_channame2dname (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;
  retval->type = SCRIPT_STRING;

  chan = findchan(whichchan);
  if (chan) {
      retval->value = (void *) chan->dname;
      return(1);
  } else {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel-name: %s", whichchan);
      return(0);
  }
}

static int script_chandname2name (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;
  retval->type = SCRIPT_STRING;

  chan = findchan_by_dname(whichchan);
  if (chan) {
      retval->value = (void *) chan->name;
      return(1);
  } else {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel-dname: %s", whichchan);
      return(0);
  }
}

/* flushmode <chan> */
static int script_flushmode (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  flush_mode(chan, NORMAL);
  return(1);
}

static int script_pushmode (script_var_t *retval, int nargs, char *whichchan, char *whichmode, char *supparg)
{
  struct chanset_t *chan;
  char plus, mode;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  plus = whichmode[0]; /* + or - */
  mode = whichmode[1]; /* the mode letter */
  if ((plus != '+') && (plus != '-')) {
    mode = plus;
    plus = '+';
  }
  if (nargs == 3)
    add_mode(chan, plus, mode, supparg);
  else
    add_mode(chan, plus, mode, "");
  return(1);
}

static int script_resetbans (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  resetbans(chan);
  return(1);
}

static int script_resetexempts (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  resetexempts(chan);
  return(1);
}

static int script_resetinvites (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  resetinvites(chan);
  return(1);
}

static int script_resetchan (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  reset_chan_info(chan);
  return(1);
}

static int script_topic (script_var_t *retval, char *whichchan)
{
  struct chanset_t *chan;
  retval->type = SCRIPT_STRING;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  retval->value = chan->channel.topic;
  return(1);
}

static int script_hand2nick (script_var_t *retval, int nargs, char *whichhand, char *whichchan)
{/* drummer */
  memberlist *m;
  struct chanset_t *chan, *thechan = NULL;
  retval->type = SCRIPT_STRING;

  if (nargs == 2) {
    chan = findchan_by_dname(whichchan);
    thechan = chan;
    if (chan == NULL) {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
      return(0);
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      if (m->user && !irccmp(m->user->handle, whichhand)) {
        retval->value = m->nick;
        return(1);
      }
    }
    chan = chan->next;
  }
  return(1);
}

static int script_nick2hand (script_var_t *retval, int nargs, char *whichnick, char *whichchan)
{/* drummer */
  memberlist *m;
  struct chanset_t *chan, *thechan = NULL;
  retval->type = SCRIPT_STRING;

  if (nargs == 2) {
    chan = findchan_by_dname(whichchan);
    thechan = chan;
    if (chan == NULL) {
      retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
      retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
      return(0);
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    m = ismember(chan, whichnick);
    if (m) {
      retval->value = (m->user ? m->user->handle : "*");
      return(1);
    }
    chan = chan->next;
  }
  retval->value = "*";
  return(1);
}

/* Sends an optimal number of kicks per command (as defined by kick_method)
 * to the server, simialer to kick_all.
 */
static int script_putkick (script_var_t *retval, int nargs, char *whichchan, char *nicklist, char *whichcomment)
{
  struct chanset_t *chan;
  int k = 0, l;
  char kicknick[512], *nick, *p, *comment = NULL;
  memberlist *m;

  chan = findchan_by_dname(whichchan);
  if (chan == NULL) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR | SCRIPT_FREE;
    retval->value = (void *) msprintf("Invalid channel: %s", whichchan);
    return(0);
  }
  if (nargs == 3)
    comment = whichcomment;
  else
    comment = "";
  if (!me_op(chan)) {
    retval->type = SCRIPT_STRING | SCRIPT_ERROR;
    retval->value = (void *) "need op";
    return(0);
  }

  kicknick[0] = 0;
  p = nicklist; /* beginning of first nick */
  /* Loop through all given nicks */
  while(p) {
    nick = p;
    p = strchr(nick, ',');	/* Search for beginning of next nick */
    if (p) {
      *p = 0;
      p++;
    }

    m = ismember(chan, nick);
    if (!m)
      continue;			/* Skip non-existant nicks */
    m->flags |= SENTKICK;	/* Mark as pending kick */
    if (kicknick[0])
      strcat(kicknick, ",");
    strcat(kicknick, nick);	/* Add to local queue */
    k++;

    /* Check if we should send the kick command yet */
    l = strlen(chan->name) + strlen(kicknick) + strlen(comment);
    if (((kick_method != 0) && (k == kick_method)) || (l > 480)) {
      dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, kicknick, comment);
      k = 0;
      kicknick[0] = 0;
    }
  }
  /* Clear out all pending kicks in our local kick queue */
  if (k > 0)
    dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, kicknick, comment);
  return(1);
}

static script_command_t irc_script_cmds[] = {
  {"", "chanlist", script_chanlist, NULL, 1, "ss", "channel ?flags?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "botisop", script_botisop, NULL, 0, "s", "?channel?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "ischanjuped", script_ischanjuped, NULL, 1, "s", "channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "botisvoice", script_botisvoice, NULL, 0, "s", "?channel?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "botonchan", script_botonchan, NULL, 0, "s", "channel ?flags?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "isop", script_isop, NULL, 1, "ss", "nick ?channel?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "isvoice", script_isvoice, NULL, 1, "ss", "nick ?channel?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "wasop", script_wasop, NULL, 2, "ss", "nick channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "onchan", script_onchan, NULL, 1, "ss", "nick channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "handonchan", script_handonchan, NULL, 1, "ss", "handle ?channel?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "ischanban", script_ischanban, NULL, 2, "ss", "ban channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "ischanexempt", script_ischanexempt, NULL, 2, "ss", "exempt channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "ischaninvite", script_ischaninvite, NULL, 2, "ss", "invite channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "getchanhost", script_getchanhost, NULL, 1, "ss", "nick ?channel?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "onchansplit", script_onchansplit, NULL, 1, "ss", "nick ?channel?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "maskhost", script_maskhost, NULL, 1, "s", "nick!user@host", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "getchanidle", script_getchanidle, NULL, 2, "ss", "nick channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "chanbans", script_chanbans, NULL, 1, "s", "channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "chanexempts", script_chanexempts, NULL, 1, "s", "channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "chaninvites", script_chaninvites, NULL, 1, "s", "channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "getchanmode", script_getchanmode, NULL, 1, "s", "channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "getchanjoin", script_getchanjoin, NULL, 2, "ss", "nick channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "channame2dname", script_channame2dname, NULL, 1, "s", "channel-name", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "chandname2name", script_chandname2name, NULL, 1, "s", "channel-dname", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "flushmode", script_flushmode, NULL, 1, "s", "channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "pushmode", script_pushmode, NULL, 2, "sss", "channel mode ?arg?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "resetbans", script_resetbans, NULL, 1, "s", "channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "resetexempts", script_resetexempts, NULL, 1, "s", "channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "resetinvites", script_resetinvites, NULL, 1, "s", "channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "resetchan", script_resetchan, NULL, 1, "s", "channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "topic", script_topic, NULL, 1, "s", "channel", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
  {"", "hand2nick", script_hand2nick, NULL, 1, "ss", "handle ?channel?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "nick2hand", script_nick2hand, NULL, 1, "ss", "nick ?channel?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {"", "putkick", script_putkick, NULL, 2, "sss", "channel nick?s? ?comment?", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
  {0}
};


