/*
 * irc.c --
 *
 *	support for channels within the bot
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
static const char rcsid[] = "$Id: irc.c,v 1.25 2002/11/29 06:15:25 wcc Exp $";
#endif

#define MODULE_NAME "irc"
#define MAKING_IRC
#include "lib/eggdrop/module.h"
#include "modules/server/server.h"
#include "irc.h"
#include "modules/channels/channels.h"
#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

#define start irc_LTX_start

/* Import some bind tables from the server module. */
static bind_table_t *BT_ctcp, *BT_ctcr;

/* We also create a few. */
static bind_table_t *BT_topic, *BT_split, *BT_rejoin, *BT_quit, *BT_join, *BT_part, *BT_kick, *BT_nick, *BT_mode, *BT_need;

static eggdrop_t *egg = NULL;
static Function *channels_funcs = NULL, *server_funcs = NULL;

static int ctcp_mode;
static int strict_host;
static int wait_split = 300;		/* Time to wait for user to return from
					   net-split. */
static int max_bans = 20;
static int max_exempts = 20;
static int max_invites = 20;
static int max_modes = 30;
static int bounce_bans = 1;
static int bounce_exempts = 0;
static int bounce_invites = 0;
static int bounce_modes = 0;
static int wait_info = 15;
static int invite_key = 1;
static int no_chanrec_info = 0;
static int modesperline = 3;		/* Number of modes per line to send. */
static int mode_buf_len = 200;		/* Maximum bytes to send in 1 mode. */
static int use_354 = 0;			/* Use ircu's short 354 /who
					   responses. */
static int kick_method = 1;		/* How many kicks does the irc network
					   support at once?
					   0 = as many as possible.
					       (Ernst 18/3/1998) */
static int keepnick = 1;		/* Keep nick */
static int prevent_mixing = 1;		/* To prevent mixing old/new modes */
static int rfc_compliant = 1;

static int include_lk = 1;		/* For correct calculation
					   in real_add_mode. */

#include "chan.c"
#include "mode.c"
#include "cmdsirc.c"
#include "msgcmds.c"
#include "scriptcmds.c"


/* Contains the logic to decide wether we want to punish someone. Returns
 * true (1) if we want to, false (0) if not.
 */
static int want_to_revenge(struct chanset_t *chan, struct userrec *u,
			   struct userrec *u2, char *badnick, char *victim,
			   int mevictim)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  /* Do not take revenge upon ourselves. */
  if (match_my_nick(badnick))
    return 0;

  get_user_flagrec(u, &fr, chan->dname);

  /* Kickee is not a friend? */
  if (!chan_friend(fr) && !glob_friend(fr) &&
      /* ... and they didn't kick themself? */
      irccmp(badnick, victim)) {
    /* They kicked me? */
    if (mevictim) {
      /* ... and I'm allowed to take revenge? <snicker> */
      if (channel_revengebot(chan))
        return 1;
    /* Do we revenge for our users ... and do we actually know the victim? */
    } else if (channel_revenge(chan) && u2) {
      struct flag_record fr2 = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

      get_user_flagrec(u2, &fr2, chan->dname);
      /* Protecting friends? */
      if ((channel_protectfriends(chan) &&
	   /* ... and victim is valid friend? */
	   (chan_friend(fr2) || (glob_friend(fr2) && !chan_deop(fr2)))) ||
	  /* ... or protecting ops */
	  (channel_protectops(chan) &&
	   /* ... and kicked is valid op? */
	   (chan_op(fr2) || (glob_op(fr2) && !chan_deop(fr2)))))
	return 1;
    }
  }
  return 0;
}

/* Dependant on revenge_mode, punish the offender.
 */
static void punish_badguy(struct chanset_t *chan, char *whobad,
			  struct userrec *u, char *badnick, char *victim,
			  int mevictim, int type)
{
  char reason[1024], ct[81], *kick_msg;
  memberlist *m;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  m = ismember(chan, badnick);
  if (!m)
    return;
  get_user_flagrec(u, &fr, chan->dname);

  /* Get current time into a string */
  strftime(ct, 7, "%d %b", localtime(&now));

  /* Put together log and kick messages */
  reason[0] = 0;
  switch (type) {
  case REVENGE_KICK:
    kick_msg = _("dont kick my friends, bud");
    simple_sprintf(reason, "kicked %s off %s", victim, chan->dname);
    break;
  case REVENGE_DEOP:
    simple_sprintf(reason, "deopped %s on %s", victim, chan->dname);
    kick_msg = _("dont deop my friends, bud");
    break;
  default:
    kick_msg = "revenge!";
  }
  putlog(LOG_MISC, chan->dname, "Punishing %s (%s)", badnick, reason);

  /* Set the offender +d */
  if ((chan->revenge_mode > 0) &&
      /* ... unless there's no more to do */
      !(chan_deop(fr) || glob_deop(fr))) {
    char s[UHOSTLEN], s1[UHOSTLEN];
    memberlist *mx = NULL;

    /* Removing op */
    if (chan_op(fr) || (glob_op(fr) && !chan_deop(fr))) {
      fr.match = FR_CHAN;
      if (chan_op(fr)) {
        fr.chan &= ~USER_OP;
      } else {
        fr.chan |= USER_DEOP;
      }
      set_user_flagrec(u, &fr, chan->dname);
      putlog(LOG_MISC, "*", "No longer opping %s[%s] (%s)", u->handle, whobad,
	     reason);
    }
    /* ... or just setting to deop */
    else if (u) {
      /* In the user list already, cool :) */
      fr.match = FR_CHAN;
      fr.chan |= USER_DEOP;
      set_user_flagrec(u, &fr, chan->dname);
      simple_sprintf(s, "(%s) %s", ct, reason);
      putlog(LOG_MISC, "*", "Now deopping %s[%s] (%s)", u->handle, whobad, s);
    }
    /* ... or creating new user and setting that to deop */
    else {
      strcpy(s1, whobad);
      maskhost(s1, s);
      strcpy(s1, badnick);
      /* If that handle exists use "badX" (where X is an increasing number)
       * instead.
       */
      while (get_user_by_handle(userlist, s1)) {
        if (!strncmp(s1, "bad", 3)) {
          int i;

          i = atoi(s1 + 3);
          simple_sprintf(s1 + 3, "%d", i + 1);
        } else
          strcpy(s1, "bad1");		/* Start with '1' */
      }
      userlist = adduser(userlist, s1, s, "-", 0);
      fr.match = FR_CHAN;
      fr.chan = USER_DEOP;
      fr.udef_chan = 0;
      u = get_user_by_handle(userlist, s1);
      if ((mx = ismember(chan, badnick)))
        mx->user = u;
      set_user_flagrec(u, &fr, chan->dname);
      simple_sprintf(s, "(%s) %s (%s)", ct, reason, whobad);
      set_user(&USERENTRY_COMMENT, u, (void *) s);
      putlog(LOG_MISC, "*", "Now deopping %s (%s)", whobad, reason);
    }
  }

  /* Always try to deop the offender */
  if (!mevictim)
    add_mode(chan, '-', 'o', badnick);
  /* Ban. Should be done before kicking. */
  if (chan->revenge_mode > 2) {
    char *baduhost, s[UHOSTLEN], s1[UHOSTLEN], s2[UHOSTLEN];

    /* FIXME: clean this mess up */
    strlcpy(s, whobad, sizeof s);
    strtok(s, "!");
    baduhost = strtok(NULL, "!");
    maskhost(baduhost, s1);
    simple_sprintf(s2, "(%s) %s", ct, reason);
    u_addmask('b', chan, s1, botnetnick, s2, now + (60 * chan->ban_time), 0);
    if (!mevictim && me_op(chan)) {
      add_mode(chan, '+', 'b', s1);
      flush_mode(chan, QUICK);
    }
  }
  /* Kick the offender */
  if ((chan->revenge_mode > 1) &&
      /* ... or don't we kick ops? */
      (!channel_dontkickops(chan) ||
        !(chan_op(fr) || (glob_op(fr) && !chan_deop(fr)))) &&
      /* ... or have we sent the kick already? */
      !chan_sentkick(m) &&
      /* ... and can I actually do anything about it? */
      me_op(chan) && !mevictim) {
    dprintf(DP_MODE, "KICK %s %s :%s\n", chan->name, badnick, kick_msg);
    m->flags |= SENTKICK;
  }
}

/* Punishes bad guys under certain circumstances using methods as defined
 * by the revenge_mode flag.
 */
static void maybe_revenge(struct chanset_t *chan, char *whobad,
			  char *whovictim, int type)
{
  char buf[UHOSTLEN], buf2[UHOSTLEN], *badnick, *victim;
  int mevictim;
  struct userrec *u, *u2;

  if (!chan || (type < 0))
    return;

  /* Get info about offender */
  u = get_user_by_host(whobad);
  strlcpy(buf, whobad, sizeof buf);
  badnick = strtok(buf, "!");

  /* Get info about victim */
  u2 = get_user_by_host(whovictim);
  strlcpy(buf2, whovictim, sizeof buf2);
  victim = strtok(buf2, "!");
  mevictim = match_my_nick(victim);

  /* Do we want to revenge? */
  if (!want_to_revenge(chan, u, u2, badnick, victim, mevictim))
    return;	/* No, leave them alone ... */

  /* Haha! Do the vengeful thing ... */
  punish_badguy(chan, whobad, u, badnick, victim, mevictim, type);
}

/* Set the key.
 */
static void set_key(struct chanset_t *chan, char *k)
{
  free(chan->channel.key);
  if (k == NULL) {
    chan->channel.key = calloc(1, 1);
    return;
  }
  chan->channel.key = calloc(1, strlen(k) + 1);
  strcpy(chan->channel.key, k);
}

static int hand_on_chan(struct chanset_t *chan, struct userrec *u)
{
  char s[UHOSTLEN];
  memberlist *m;

  for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
    sprintf(s, "%s!%s", m->nick, m->userhost);
    if (u == get_user_by_host(s))
      return 1;
  }
  return 0;
}

/* Adds a ban, exempt or invite mask to the list
 * m should be chan->channel.(exempt|invite|ban)
 */
static void newmask(masklist *m, char *s, char *who)
{
  for (; m && m->mask[0] && irccmp(m->mask, s); m = m->next);
  if (m->mask[0])
    return;			/* Already existent mask */

  m->next = calloc(1, sizeof(masklist));
  m->next->next = NULL;
  m->next->mask = calloc(1, 1);
  m->next->mask[0] = 0;
  free(m->mask);
  m->mask = calloc(1, strlen(s) + 1);
  strcpy(m->mask, s);
  m->who = calloc(1, strlen(who) + 1);
  strcpy(m->who, who);
  m->timer = now;
}

/* Removes a nick from the channel member list (returns 1 if successful)
 */
static int killmember(struct chanset_t *chan, char *nick)
{
  memberlist *x, *old;

  old = NULL;
  for (x = chan->channel.member; x && x->nick[0]; old = x, x = x->next)
    if (!irccmp(x->nick, nick))
      break;
  if (!x || !x->nick[0]) {
    if (!channel_pending(chan))
    return 0;
  }
  if (old)
    old->next = x->next;
  else
    chan->channel.member = x->next;
  free(x);
  chan->channel.members--;

  /* The following two errors should NEVER happen. We will try to correct
   * them though, to keep the bot from crashing.
   */
  if (chan->channel.members < 0) {
     chan->channel.members = 0;
     for (x = chan->channel.member; x && x->nick[0]; x = x->next)
       chan->channel.members++;
  }
  if (!chan->channel.member) {
    chan->channel.member = calloc(1, sizeof(memberlist));
    chan->channel.member->nick[0] = 0;
    chan->channel.member->next = NULL;
  }
  return 1;
}

/* Check if I am a chanop. Returns boolean 1 or 0.
 */
static int me_op(struct chanset_t *chan)
{
  memberlist *mx = NULL;

  mx = ismember(chan, botname);
  if (!mx)
    return 0;
  if (chan_hasop(mx))
    return 1;
  else
    return 0;
}

/* Check whether I'm voice. Returns boolean 1 or 0.
 */
static int me_voice(struct chanset_t *chan)
{
  memberlist	*mx;

  mx = ismember(chan, botname);
  if (!mx)
    return 0;
  if (chan_hasvoice(mx))
    return 1;
  else
    return 0;
}

/* Check if there are any ops on the channel. Returns boolean 1 or 0.
 */
static int any_ops(struct chanset_t *chan)
{
  memberlist *x;

  for (x = chan->channel.member; x && x->nick[0]; x = x->next)
    if (chan_hasop(x))
      break;
  if (!x || !x->nick[0])
    return 0;
  return 1;
}

/* Reset the channel information.
 */
static void reset_chan_info(struct chanset_t *chan)
{
  /* Don't reset the channel if we're already resetting it */
  if (channel_inactive(chan)) {
    dprintf(DP_MODE,"PART %s\n", chan->name);
    return;
  }
  if (!channel_pending(chan)) {
    free(chan->channel.key);
    chan->channel.key = calloc(1, 1);
    clear_channel(chan, 1);
    chan->status |= CHAN_PEND;
    chan->status &= ~(CHAN_ACTIVE | CHAN_ASKEDMODES);
    if (!(chan->status & CHAN_ASKEDBANS)) {
      chan->status |= CHAN_ASKEDBANS;
      dprintf(DP_MODE, "MODE %s +b\n", chan->name);
    }
    if (!(chan->ircnet_status & CHAN_ASKED_EXEMPTS) &&
	use_exempts == 1) {
      chan->ircnet_status |= CHAN_ASKED_EXEMPTS;
      dprintf(DP_MODE, "MODE %s +e\n", chan->name);
    }
    if (!(chan->ircnet_status & CHAN_ASKED_INVITED) &&
	use_invites == 1) {
      chan->ircnet_status |= CHAN_ASKED_INVITED;
      dprintf(DP_MODE, "MODE %s +I\n", chan->name);
    }
    /* These 2 need to get out asap, so into the mode queue */
    dprintf(DP_MODE, "MODE %s\n", chan->name);
    if (use_354)
      dprintf(DP_MODE, "WHO %s %%c%%h%%n%%u%%f\n", chan->name);
    else
      dprintf(DP_MODE, "WHO %s\n", chan->name);
    /* clear_channel nuked the data...so */
  }
}

/* Leave the specified channel and notify registered Tcl procs. This
 * should not be called by itsself.
 */
static void do_channel_part(struct chanset_t *chan)
{
  if (!channel_inactive(chan) && chan->name[0]) {
    /* Using chan->name is important here, especially for !chans <cybah> */
    dprintf(DP_SERVER, "PART %s\n", chan->name);

    /* As we don't know of this channel anymore when we receive the server's
       ack for the above PART, we have to notify about it _now_. */
    check_tcl_part(botname, botuserhost, NULL, chan->dname, NULL);
  }
}

/* Report the channel status of every active channel to dcc chat every
 * 5 minutes.
 */
static void status_log()
{
  masklist *b;
  memberlist *m;
  struct chanset_t *chan;
  char s[20], s2[20];
  int chops, voice, nonops, bans, invites, exempts;

  if (!server_online)
    return;

  for (chan = chanset; chan != NULL; chan = chan->next) {
    if (channel_active(chan) && channel_logstatus(chan) &&
        !channel_inactive(chan)) {
      chops = 0;
      voice = 0;
      for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
	if (chan_hasop(m))
	  chops++;
	else if (chan_hasvoice(m))
	  voice++;
      }
      nonops = (chan->channel.members - (chops + voice));

      for (bans = 0, b = chan->channel.ban; b->mask[0]; b = b->next)
	bans++;
      for (exempts = 0, b = chan->channel.exempt; b->mask[0]; b = b->next)
	exempts++;
      for (invites = 0, b = chan->channel.invite; b->mask[0]; b = b->next)
	invites++;

      sprintf(s, "%d", exempts);
      sprintf(s2, "%d", invites);

      putlog(LOG_MISC, chan->dname,
	     "%s%-10s (%s) : [m/%d o/%d v/%d n/%d b/%d e/%s I/%s]",
             me_op(chan) ? "@" : me_voice(chan) ? "+" : "", chan->dname,
             getchanmode(chan), chan->channel.members, chops, voice, nonops,
	     bans, use_exempts ? s : "-", use_invites ? s2 : "-");
    }
  }
}

/* If i'm the only person on the channel, and i'm not op'd,
 * might as well leave and rejoin.
 */
static void check_lonely_channel(struct chanset_t *chan)
{
  memberlist *m;
  int i = 0;

  if (channel_pending(chan) || !channel_active(chan) || me_op(chan) ||
      channel_inactive(chan) || (chan->channel.mode & CHANANON))
    return;
  /* Count non-split channel members */
  for (m = chan->channel.member; m && m->nick[0]; m = m->next)
    if (!chan_issplit(m))
      i++;
  if (i == 1 && channel_cycle(chan) && !channel_stop_cycle(chan) && (chan->name[0] != '+')) {
    putlog(LOG_MISC, "*", "Trying to cycle %s to regain ops.", chan->dname);
    dprintf(DP_MODE, "PART %s\n", chan->name);
    /* If it's a !chan, we need to recreate the channel with !!chan <cybah> */
    dprintf(DP_MODE, "JOIN %s%s %s\n", (chan->dname[0] == '!') ? "!" : "",
	    chan->dname, chan->key_prot);
  } else 
    check_tcl_need(chan->dname, "cycle");
}

static void check_expired_chanstuff()
{
  masklist *b, *e;
  memberlist *m, *n;
  char s[UHOSTLEN];
  struct chanset_t *chan;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (!server_online)
    return;
  for (chan = chanset; chan; chan = chan->next) {
    if (channel_active(chan)) {
      if (me_op(chan)) {
	if (channel_dynamicbans(chan) && chan->ban_time)
	  for (b = chan->channel.ban; b->mask[0]; b = b->next)
	    if (now - b->timer > 60 * chan->ban_time &&
		!u_sticky_mask(chan->bans, b->mask) &&
		!u_sticky_mask(global_bans, b->mask) &&
		expired_mask(chan, b->who)) {
	      putlog(LOG_MODES, chan->dname,
		     "(%s) Channel ban on %s expired.",
		     chan->dname, b->mask);
	      add_mode(chan, '-', 'b', b->mask);
	      b->timer = now;
	    }

	if (use_exempts && channel_dynamicexempts(chan) && chan->exempt_time)
	  for (e = chan->channel.exempt; e->mask[0]; e = e->next)
	    if (now - e->timer > 60 * chan->exempt_time &&
		!u_sticky_mask(chan->exempts, e->mask) &&
		!u_sticky_mask(global_exempts, e->mask) &&
		expired_mask(chan, e->who)) {
	      /* Check to see if it matches a ban */
	      int match = 0;

	      for (b = chan->channel.ban; b->mask[0]; b = b->next)
		if (wild_match(b->mask, e->mask) ||
		    wild_match(e->mask, b->mask)) {
		  match = 1;
		  break;
	      }
	      /* Leave this extra logging in for now. Can be removed later
	       * Jason
	       */
	      if (match) {
		putlog(LOG_MODES, chan->dname,
		       "(%s) Channel exemption %s NOT expired. Exempt still set!",
		       chan->dname, e->mask);
	      } else {
		putlog(LOG_MODES, chan->dname,
		       "(%s) Channel exemption on %s expired.",
		       chan->dname, e->mask);
		add_mode(chan, '-', 'e', e->mask);
	      }
	      e->timer = now;
	    }

	if (use_invites && channel_dynamicinvites(chan) &&
	    invite_time && !(chan->channel.mode & CHANINV))
	  for (b = chan->channel.invite; b->mask[0]; b = b->next)
	    if (now - b->timer > 60 * chan->invite_time &&
		!u_sticky_mask(chan->invites, b->mask) &&
		!u_sticky_mask(global_invites, b->mask) &&
		expired_mask(chan, b->who)) {
	      putlog(LOG_MODES, chan->dname,
		     "(%s) Channel invitation on %s expired.",
		     chan->dname, b->mask);
	      add_mode(chan, '-', 'I', b->mask);
	      b->timer = now;
	    }

	if (chan->idle_kick)
	  for (m = chan->channel.member; m && m->nick[0]; m = m->next)
	    if (now - m->last >= chan->idle_kick * 60 &&
		!match_my_nick(m->nick) && !chan_issplit(m)) {
	      sprintf(s, "%s!%s", m->nick, m->userhost);
	      get_user_flagrec(m->user ? m->user : get_user_by_host(s),
			       &fr, chan->dname);
	      if (!(glob_bot(fr) || glob_friend(fr) ||
		    (glob_op(fr) && !chan_deop(fr)) ||
		    chan_friend(fr) || chan_op(fr))) {
		dprintf(DP_SERVER, "KICK %s %s :idle %d min\n", chan->name,
			m->nick, chan->idle_kick);
		m->flags |= SENTKICK;
	      }
	    }
      }
      for (m = chan->channel.member; m && m->nick[0]; m = n) {
	n = m->next;
	if (m->split && now - m->split > wait_split) {
	  sprintf(s, "%s!%s", m->nick, m->userhost);
	  check_tcl_sign(m->nick, m->userhost,
			 m->user ? m->user : get_user_by_host(s),
			 chan->dname, "lost in the netsplit");
	  putlog(LOG_JOIN, chan->dname,
		 "%s (%s) got lost in the net-split.",
		 m->nick, m->userhost);
	  killmember(chan, m->nick);
	}
	m = n;
      }
      check_lonely_channel(chan);
    }
    else if (!channel_inactive(chan) && !channel_pending(chan))
      dprintf(DP_MODE, "JOIN %s %s\n",
              (chan->name[0]) ? chan->name : chan->dname,
              chan->channel.key[0] ? chan->channel.key : chan->key_prot);
  }
}

static void check_tcl_joinspltrejn(char *nick, char *uhost, struct userrec *u,
			       char *chname, bind_table_t *table)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  char args[1024];

  simple_sprintf(args, "%s %s!%s", chname, nick, uhost);
  get_user_flagrec(u, &fr, chname);

  check_bind(table, args, &fr, nick, uhost, u, chname);
}

/* we handle part messages now *sigh* (guppy 27Jan2000) */

static void check_tcl_part(char *nick, char *uhost, struct userrec *u,
			       char *chname, char *text)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  char args[1024];

  simple_sprintf(args, "%s %s!%s", chname, nick, uhost);
  get_user_flagrec(u, &fr, chname);

  check_bind(BT_part, args, &fr, nick, uhost, u, chname, text);
}

static void check_tcl_signtopcnick(char *nick, char *uhost, struct userrec *u,
				   char *chname, char *reason,
				   bind_table_t *table)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  char args[1024];

  if (table == BT_quit) {
    simple_sprintf(args, "%s %s!%s", chname, nick, uhost);
  }
  else {
    simple_sprintf(args, "%s %s", chname, reason);
  }
  get_user_flagrec(u, &fr, chname);
  check_bind(table, args, &fr, nick, uhost, u, chname, reason);
}

static void check_tcl_kickmode(char *nick, char *uhost, struct userrec *u,
			       char *chname, char *dest, char *reason,
			       bind_table_t *table)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  char args[1024];

  get_user_flagrec(u, &fr, chname);
  if (table == BT_mode) {
    simple_sprintf(args, "%s %s", chname, dest);
  }
  else {
    simple_sprintf(args, "%s %s %s", chname, dest, reason);
  }
  check_bind(table, args, &fr, nick, uhost, u, chname, dest, reason);
}

static void check_tcl_need(char *chname, char *type)
{
  char buf[1024];

  simple_sprintf(buf, "%s %s", chname, type);
  check_bind(BT_need, buf, NULL, chname, type);
}

static tcl_ints myints[] =
{
  {"wait_split",		&wait_split,		0},
  {"wait_info",			&wait_info,		0},
  {"bounce_bans",		&bounce_bans,		0},
  {"bounce_exempts",		&bounce_exempts,	0},
  {"bounce_invites",		&bounce_invites,	0},
  {"bounce_modes",		&bounce_modes,		0},
  {"modes_per_line",		&modesperline,		0},
  {"mode_buf_length",		&mode_buf_len,		0},
  {"use_354",			&use_354,		0},
  {"kick_method",		&kick_method,		0},
  {"invite_key",		&invite_key,		0},
  {"no_chanrec_info",		&no_chanrec_info,	0},
  {"max_bans",			&max_bans,		0},
  {"max_exempts",		&max_exempts,		0},
  {"max_invites",		&max_invites,		0},
  {"max_modes",			&max_modes,		0},
  {"strict_host",		&strict_host,		0},
  {"ctcp_mode",			&ctcp_mode,		0},
  {"keep_nick",			&keepnick,		0},
  {"prevent_mixing",		&prevent_mixing,	0},
  {"rfc_compliant",		&rfc_compliant,		0},
  {"include_lk",		&include_lk,		0},
  {NULL,			NULL,			0}
};

/* Flush the modes for EVERY channel.
 */
static void flush_modes()
{
  struct chanset_t *chan;
  memberlist *m;

  if (modesperline > MODES_PER_LINE_MAX)
    modesperline = MODES_PER_LINE_MAX; 
  for (chan = chanset; chan; chan = chan->next) {
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      if (m->delay && m->delay <= now) {
	m->delay = 0L;
	m->flags &= ~FULL_DELAY;
        if (chan_sentop(m)) {
          m->flags &= ~SENTOP;
          add_mode(chan, '+', 'o', m->nick);
        }
        if (chan_sentvoice(m)) {
          m->flags &= ~SENTVOICE;
          add_mode(chan, '+', 'v', m->nick);
        }
      }
    }
    flush_mode(chan, NORMAL);
  }
}

static void irc_report(int idx, int details)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  char ch[1024], q[160], *p;
  int k, l;
  struct chanset_t *chan;

  strcpy(q, "Channels: ");
  k = 10;
  for (chan = chanset; chan; chan = chan->next) {
    if (idx != DP_STDOUT)
      get_user_flagrec(dcc[idx].user, &fr, chan->dname);
    if (idx == DP_STDOUT || glob_master(fr) || chan_master(fr)) {
      p = NULL;
      if (!channel_inactive(chan)) {
	if (chan->status & CHAN_JUPED)
	  p = _("juped");
	else if (!(chan->status & CHAN_ACTIVE))
	  p = _("trying");
	else if (chan->status & CHAN_PEND)
	  p = _("pending");
        else if (!any_ops(chan))
          p = _("opless");
	else if ((chan->dname[0] != '+') && !me_op(chan))
	  p = _("want ops!");
      }
      l = simple_sprintf(ch, "%s%s%s%s, ", chan->dname, p ? "(" : "",
			 p ? p : "", p ? ")" : "");
      if ((k + l) > 70) {
	dprintf(idx, "   %s\n", q);
	strcpy(q, "          ");
	k = 10;
      }
      k += my_strcpy(q + k, ch);
    }
  }
  if (k > 10) {
    q[k - 2] = 0;
    dprintf(idx, "    %s\n", q);
  }
}

static char *traced_rfccompliant(ClientData cdata, Tcl_Interp *irp,
				 char *name1, char *name2, int flags)
{
  /* This hook forces eggdrop core to change the irccmp match
   * function links to point to the rfc compliant versions if
   * rfc_compliant is 1, or to the normal version if it's 0.
   */
  add_hook(HOOK_IRCCMP, (Function) rfc_compliant);
  return NULL;
}

static char *irc_close()
{
  struct chanset_t *chan;

  dprintf(DP_MODE, "JOIN 0\n");
  for (chan = chanset; chan; chan = chan->next)
    clear_channel(chan, 1);
  bind_table_del(BT_topic);
  bind_table_del(BT_split);
  bind_table_del(BT_quit);
  bind_table_del(BT_rejoin);
  bind_table_del(BT_part);
  bind_table_del(BT_nick);
  bind_table_del(BT_mode);
  bind_table_del(BT_kick);
  bind_table_del(BT_join);
  bind_table_del(BT_need);
  rem_tcl_ints(myints);

  rem_builtins("dcc", irc_dcc);
  rem_builtins("raw", irc_raw);
  rem_builtins("msg", C_msg);

  script_delete_commands(irc_script_cmds);
  rem_help_reference("irc.help");
  del_hook(HOOK_MINUTELY, (Function) check_expired_chanstuff);
  del_hook(HOOK_5MINUTELY, (Function) status_log);
  del_hook(HOOK_ADD_MODE, (Function) real_add_mode);
  del_hook(HOOK_IDLE, (Function) flush_modes);
  Tcl_UntraceVar(interp, "rfc-compliant",
		 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		 traced_rfccompliant, NULL);
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *start();

static Function irc_table[] =
{
  /* 0 - 3 */
  (Function) start,
  (Function) irc_close,
  (Function) 0,
  (Function) irc_report,
  /* 4 - 7 */
  (Function) recheck_channel,
  (Function) me_op,
  (Function) recheck_channel_modes,
  (Function) do_channel_part,
  /* 8 - 10 */
  (Function) check_this_ban,
  (Function) check_this_user,
  (Function) me_voice,
};

char *start(eggdrop_t *eggdrop)
{
  struct chanset_t *chan;

  egg = eggdrop;

  module_register(MODULE_NAME, irc_table, 1, 3);
  if (!module_depend(MODULE_NAME, "eggdrop", 107, 0)) {
    module_undepend(MODULE_NAME);
    return "This module needs eggdrop1.7.0 or later";
  }
  if (!(server_funcs = module_depend(MODULE_NAME, "server", 1, 0))) {
    module_undepend(MODULE_NAME);
    return "You need the server module to use the irc module.";
  }
  if (!(channels_funcs = module_depend(MODULE_NAME, "channels", 1, 0))) {
    module_undepend(MODULE_NAME);
    return "You need the channels module to use the irc module.";
  }
  for (chan = chanset; chan; chan = chan->next) {
    if (!channel_inactive(chan))
      dprintf(DP_MODE, "JOIN %s %s\n",
              (chan->name[0]) ? chan->name : chan->dname, chan->key_prot);
    chan->status &= ~(CHAN_ACTIVE | CHAN_PEND | CHAN_ASKEDBANS);
    chan->ircnet_status &= ~(CHAN_ASKED_INVITED | CHAN_ASKED_EXEMPTS);
  }
  add_hook(HOOK_MINUTELY, (Function) check_expired_chanstuff);
  add_hook(HOOK_5MINUTELY, (Function) status_log);
  add_hook(HOOK_ADD_MODE, (Function) real_add_mode);
  add_hook(HOOK_IDLE, (Function) flush_modes);
  Tcl_TraceVar(interp, "rfc-compliant",
	       TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
	       traced_rfccompliant, NULL);
  add_tcl_ints(myints);

  /* Add our commands. */
  add_builtins("dcc", irc_dcc);
  add_builtins("raw", irc_raw);
  add_builtins("msg", C_msg);

  /* Import tables. */
  BT_ctcp = bind_table_lookup("ctcp");
  BT_ctcr = bind_table_lookup("ctcr");

  /* Create our own bind tables. */
  BT_topic = bind_table_add("topic", 5, "ssUss", MATCH_MASK, BIND_STACKABLE | BIND_USE_ATTR);
  BT_split = bind_table_add("split", 4, "ssUs", MATCH_MASK, BIND_STACKABLE | BIND_USE_ATTR);
  BT_rejoin = bind_table_add("rejoin", 4, "ssUs", MATCH_MASK, BIND_STACKABLE | BIND_USE_ATTR);
  BT_quit = bind_table_add("sign", 5, "ssUss", MATCH_MASK, BIND_STACKABLE | BIND_USE_ATTR);
  BT_join = bind_table_add("join", 4, "ssUs", MATCH_MASK, BIND_STACKABLE | BIND_USE_ATTR);
  BT_part = bind_table_add("part", 5, "ssUss", MATCH_MASK, BIND_STACKABLE | BIND_USE_ATTR);
  BT_nick = bind_table_add("nick", 5, "ssUss", MATCH_MASK, BIND_STACKABLE | BIND_USE_ATTR);
  BT_mode = bind_table_add("mode", 6, "ssUsss", MATCH_MASK, BIND_STACKABLE | BIND_USE_ATTR);
  BT_kick = bind_table_add("kick", 6, "ssUsss", MATCH_MASK, BIND_STACKABLE | BIND_USE_ATTR);
  BT_need = bind_table_add("need", 2, "ss", MATCH_MASK, BIND_STACKABLE | BIND_USE_ATTR);

  script_create_commands(irc_script_cmds);
  add_help_reference("irc.help");

  /* Update all rfc_ function pointers */
  add_hook(HOOK_IRCCMP, (Function) rfc_compliant);  
  
  return NULL;
}
