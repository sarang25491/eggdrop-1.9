/*
 * servmsg.c --
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

/* FIXME: #include mess
#ifndef lint
static const char rcsid[] = "$Id: servmsg.c,v 1.28 2002/06/18 06:12:32 guppy Exp $";
#endif
*/

#include "channels.h"

static int altnick_char = -1;
static char altnick_chars[] = "1234567890^_-[]{}abcdefghijklmnopqrstuvwxyz";

/* We try to change to a preferred unique nick here. We always first try the
 * specified alternate nick. If that failes, we repeatedly modify the nick
 * until it gets accepted.
 *
 * sent nick:
 *     "<altnick><c>"
 *                ^--- additional count character: 1-9^-_\\[]`a-z
 *          ^--------- given, alternate nick
 *
 * The last added character is always saved in altnick_char. At the very first
 * attempt (were altnick_char is 0), we try the alternate nick without any
 * additions.
 *
 * fixed by guppy (1999/02/24) and Fabian (1999/11/26)
 */
static void choose_altnick()
{
	int len;
	char *alt;

	len = strlen(botname);
	alt = get_altbotnick();

	/* First run? */
	if ((altnick_char == -1) && irccmp(alt, botname)) {
		/* Alternate nickname defined. Let's try that first. */
		str_redup(&botname, alt);
	}
	else if (altnick_char == -1) {
		botname = (char *)realloc(botname, len+2);
		botname[len] = altnick_chars[0];
		altnick_char = 0;
		botname[len+1] = 0;
	}
	else {
		altnick_char++;
		if (altnick_char >= sizeof(altnick_chars)) {
			make_rand_str(botname, len);
		}
		else {
			botname[len] = altnick_chars[altnick_char];
			botname[len+1] = 0;
		}
	}
	putlog(LOG_MISC, "*", _("NICK IN USE: Trying %s"), botname);
	dprintf(DP_MODE, "NICK %s\n", botname);
	return;
}

static void check_tcl_notc(char *nick, char *uhost, struct userrec *u,
	       		   char *dest, char *arg)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  get_user_flagrec(u, &fr, NULL);
  check_bind(BT_notice, arg, &fr, nick, uhost, u, arg, dest);
}

static int check_tcl_ctcpr(char *nick, char *uhost, struct userrec *u,
			   char *dest, char *keyword, char *args,
			   bind_table_t *table)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};
  get_user_flagrec(u, &fr, NULL);

  return check_bind(table, keyword, &fr, nick, uhost, u, dest, keyword, args);
}

static int check_tcl_wall(char *from, char *msg)
{
  int x;

  x = check_bind(BT_wall, msg, NULL, from, msg);
  if (x & BIND_RET_LOG) {
    putlog(LOG_WALL, "*", "!%s! %s", from, msg);
    return 1;
  }
  else return 0;
}

static int check_tcl_flud(char *nick, char *uhost, struct userrec *u,
			  char *ftype, char *chname)
{
  return check_bind(BT_flood, ftype, NULL, nick, uhost, u, ftype, chname);
}

static int match_my_nick(char *nick)
{
  if (!irccmp(nick, botname))
    return 1;
  return 0;
}

/* 001: welcome to IRC (use it to fix the server name)
 */
static int got001(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	/* Ok... first arg is what server decided our nick is. */
	server_online = now;
	str_redup(&botname, args[0]);

	check_bind_event("init-server");

	/* If the init-server bind made us leave the server, stop processing. */
	if (servidx == -1) return BIND_RET_BREAK;

	/* Send a whois request so we can see our nick!user@host */
	dprintf(DP_SERVER, "WHOIS %s\n", botname);

	/* Join all our channels. */
	channels_join_all();

	if (strcasecmp(from_nick, dcc[servidx].host)) {
		struct server_list *serv;

		putlog(LOG_MISC, "*", "(%s claims to be %s; updating server list)", dcc[servidx].host, from_nick);
		serv = server_get_current();
		if (serv) str_redup(&serv->realname, from_nick);
	}

	return(0);
}

/* Got 442: not on channel
	:server 442 nick #chan :You're not on that channel
 */
static int got442(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	struct chanset_t *chan;
	char *chname = args[1];

	chan = findchan(chname);

  if (chan && !channel_inactive(chan)) {
      module_entry	*me = module_find("channels", 0, 0);

      putlog(LOG_MISC, chname, _("Server says Im not on channel: %s"), chname);
      if (me && me->funcs)
	(me->funcs[CHANNEL_CLEAR])(chan, 1);
      chan->status &= ~CHAN_ACTIVE;
      dprintf(DP_MODE, "JOIN %s %s\n", chan->name,
	      chan->channel.key[0] ? chan->channel.key : chan->key_prot);
  }

  return 0;
}

/* Close the current server connection.
 */
static void nuke_server(char *reason)
{
  if (serv >= 0) {
    if (reason && (servidx > 0)) dprintf(servidx, "QUIT :%s\r\n", reason);
    disconnect_server();
  }
}

static char ctcp_reply[1024] = "";

static int lastmsgs[FLOOD_GLOBAL_MAX];
static char lastmsghost[FLOOD_GLOBAL_MAX][81];
static time_t lastmsgtime[FLOOD_GLOBAL_MAX];

/* Do on NICK, PRIVMSG, NOTICE and JOIN.
 */
static int detect_flood(char *floodnick, char *floodhost, char *from, int which)
{
  char *p, ftype[10], h[1024];
  struct userrec *u;
  int thr = 0, lapse = 0, atr;

  u = get_user_by_host(from);
  atr = u ? u->flags : 0;
  if (atr & (USER_BOT | USER_FRIEND))
    return 0;

  /* Determine how many are necessary to make a flood */
  switch (which) {
  case FLOOD_PRIVMSG:
  case FLOOD_NOTICE:
    thr = flud_thr;
    lapse = flud_time;
    strcpy(ftype, "msg");
    break;
  case FLOOD_CTCP:
    thr = flud_ctcp_thr;
    lapse = flud_ctcp_time;
    strcpy(ftype, "ctcp");
    break;
  }
  if ((thr == 0) || (lapse == 0))
    return 0;			/* No flood protection */
  /* Okay, make sure i'm not flood-checking myself */
  if (match_my_nick(floodnick))
    return 0;
  if (!strcasecmp(floodhost, botuserhost))
    return 0;			/* My user@host (?) */
  p = strchr(floodhost, '@');
  if (p) {
    p++;
    if (strcasecmp(lastmsghost[which], p)) {	/* New */
      strcpy(lastmsghost[which], p);
      lastmsgtime[which] = now;
      lastmsgs[which] = 0;
      return 0;
    }
  } else
    return 0;			/* Uh... whatever. */

  if (lastmsgtime[which] < now - lapse) {
    /* Flood timer expired, reset it */
    lastmsgtime[which] = now;
    lastmsgs[which] = 0;
    return 0;
  }
  lastmsgs[which]++;
  if (lastmsgs[which] >= thr) {	/* FLOOD */
    /* Reset counters */
    lastmsgs[which] = 0;
    lastmsgtime[which] = 0;
    lastmsghost[which][0] = 0;
    u = get_user_by_host(from);
    if (check_tcl_flud(floodnick, floodhost, u, ftype, "*"))
      return 0;
    /* Private msg */
    simple_sprintf(h, "*!*@%s", p);
    putlog(LOG_MISC, "*", _("Flood from @%s!  Placing on ignore!"), p);
    addignore(h, botnetnick, (which == FLOOD_CTCP) ? "CTCP flood" :
	      "MSG/NOTICE flood", now + (60 * ignore_time));
  }
  return 0;
}

static int check_ctcp_ctcr(int which, int to_channel, struct userrec *u, char *nick, char *uhost, char *dest, char *trailing)
{
	struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
	char *cmd, *space, *logdest, *text, *ctcptype;
	bind_table_t *table;
	int r, len, flags;

	len = strlen(trailing);
	if ((len < 2) || (trailing[0] != 1) || (trailing[len-1] != 1)) {
		/* Not a ctcp/ctcr. */
		return(0);
	}

	space = strchr(trailing, ' ');
	if (!space) return(1);

	*space = 0;

	trailing[len-1] = 0;
	cmd = trailing+1;	/* Skip over the \001 */
	text = space+1;

	if (which == 0) table = BT_ctcp;
	else table = BT_ctcr;

	get_user_flagrec(u, &fr, dest);

	r = check_bind(table, cmd, &fr, nick, uhost, u, dest, cmd, text);

	trailing[len-1] = 1;

	if (r & BIND_RET_BREAK) return(1);

	if (which == 0) ctcptype = "";
	else ctcptype = " reply";
	/* This should probably go in the partyline module later. */
	if (to_channel) {
		flags = LOG_PUBLIC;
		logdest = dest;
	}
	else {
		flags = LOG_MSGS;
		logdest = "*";
	}
	if (!strcasecmp(cmd, "ACTION")) putlog(flags, logdest, "Action: %s %s", nick, text);
	else putlog(flags, logdest, "CTCP%s %s: %s from %s (%s)", ctcptype, cmd, text, nick, dest);

	return(1);
}

static int check_global_notice(char *from_nick, char *from_uhost, char *dest, char *trailing)
{
	if (*dest == '$') {
		putlog(LOG_MSGS | LOG_SERV, "*", "[%s!%s to %s] %s", from_nick, from_uhost, dest, trailing);
		return(1);
	}
	return(0);
}

/* Got a private (or public) message.
	:nick!uhost PRIVMSG dest :msg
 */
static int gotmsg(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	char *dest, *trailing, *first, *space, *text;
	struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
	int to_channel;	/* Is it going to a channel? */
	int r;

	dest = args[0];
	trailing = args[1];

	/* Check if it's a global message. */
	r = check_global_notice(from_nick, from_uhost, dest, trailing);
	if (r) return(0);

	/* Check if it's an op/voice message. */
	if ((*dest == '@' || *dest == '+') && strchr(CHANMETA, *(dest+1))) {
		to_channel = 1;
		dest++;
	}
	else if (strchr(CHANMETA, *dest)) to_channel = 1;
	else to_channel = 0;

	/* Check if it's a ctcp. */
	r = check_ctcp_ctcr(0, to_channel, u, from_nick, from_uhost, dest, trailing);
	if (r) return(0);


	/* If it's a message, it goes to msg/msgm or pub/pubm. */
	/* Get the first word so we can do msg or pub. */
	first = trailing;
	space = strchr(trailing, ' ');
	if (space) {
		*space = 0;
		text = space+1;
	}
	else text = "";

	get_user_flagrec(u, &fr, dest);
	if (to_channel) {
		r = check_bind(BT_pub, first, &fr, from_nick, from_uhost, u, dest, text);
		if (r & BIND_RET_LOG) {
			putlog(LOG_CMDS, dest, "<<%s>> !%s! %s %s", from_nick, u ? u->handle : "*", first, text);
		}
	}
	else {
		r = check_bind(BT_msg, first, &fr, from_nick, from_uhost, u, text);
		if (r & BIND_RET_LOG) {
			putlog(LOG_CMDS, "*", "(%s!%s) !%s! %s %s", from_nick, from_uhost, u ? u->handle : "*", first, text);
		}
	}

	if (space) *space = ' ';

	if (r & BIND_RET_BREAK) return(0);

	/* And now the stackable version. */
	if (to_channel) {
		r = check_bind(BT_pubm, trailing, &fr, from_nick, from_uhost, u, dest, trailing);
	}
	else {
		r = check_bind(BT_msg, trailing, &fr, from_nick, from_uhost, u, trailing);
	}

	if (!(r & BIND_RET_BREAK)) {
		/* This should probably go in the partyline module later. */
		if (to_channel) {
			putlog(LOG_PUBLIC, dest, "<%s> %s", from_nick, trailing);
		}
		else {
			putlog(LOG_MSGS, "*", "[%s (%s)] %s", from_nick, from_uhost, trailing);
		}
	}
	return(0);
}

/* Got a private notice.
	:nick!uhost NOTICE dest :hello there
 */
static int gotnotice(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	char *dest, *trailing;
	struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
	int r, to_channel;

	dest = args[0];
	trailing = args[1];

	/* See if it's a server notice. */
	if (!from_uhost) {
		putlog(LOG_SERV, "*", "-NOTICE- %s", trailing);
		return(0);
	}

	/* Check if it's a global notice. */
	r = check_global_notice(from_nick, from_uhost, dest, trailing);
	if (r) return(0);

	if ((*dest == '@' || *dest == '+') && strchr(CHANMETA, *(dest+1))) {
		to_channel = 1;
		dest++;
	}
	else if (strchr(CHANMETA, *dest)) to_channel = 1;
	else to_channel = 0;

	/* Check if it's a ctcp. */
	r = check_ctcp_ctcr(1, to_channel, u, from_nick, from_uhost, dest, trailing);
	if (r) return(0);

	get_user_flagrec(u, &fr, NULL);
	r = check_bind(BT_notice, trailing, &fr, from_nick, from_uhost, u, dest, trailing);

	if (!(r & BIND_RET_BREAK)) {
		/* This should probably go in the partyline module later. */
		if (to_channel) {
			putlog(LOG_PUBLIC, dest, "-%s:%s- %s", from_nick, dest, trailing);
		}
		else {
			putlog(LOG_MSGS, "*", "-%s (%s)- %s", from_nick, from_uhost, trailing);
		}
	}
	return(0);
}

/* WALLOPS: oper's nuisance
	:csd.bu.edu WALLOPS :Connect '*.uiuc.edu 6667' from Joshua
 */
static int gotwall(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	char *msg;
	int r;

	msg = args[1];
	r = check_bind(BT_wall, msg, NULL, from_nick, msg);
	if (!(r & BIND_RET_BREAK)) {
		if (from_uhost) putlog(LOG_WALL, "*", "!%s (%s)! %s", from_nick, from_uhost, msg);
		else putlog(LOG_WALL, "*", "!%s! %s", from_nick, msg);
	}
	return 0;
}

/* Called once a minute... but if we're the only one on the
 * channel, we only wanna send out "lusers" once every 5 mins.
 */
static void minutely_checks()
{
  char *alt;

  /* Only check if we have already successfully logged in.  */
  if (!server_online)
    return;
  if (keepnick) {
    /* NOTE: now that botname can but upto NICKLEN bytes long,
     * check that it's not just a truncation of the full nick.
     */
    if (strncmp(botname, origbotname, strlen(botname))) {
      /* See if my nickname is in use and if if my nick is right.  */
	alt = get_altbotnick();
	if (alt[0] && strcasecmp (botname, alt))
	  dprintf(DP_SERVER, "ISON :%s %s %s\n", botname, origbotname, alt);
	else
          dprintf(DP_SERVER, "ISON :%s %s\n", botname, origbotname);
    }
  }
}

/* Pong from server.
	:liberty.nj.us.dal.net PONG liberty.nj.us.dal.net :12345 67890
 */
static int gotpong(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	unsigned int sec, usec;

	if (sscanf(args[1], "%u %u", &sec, &usec) != 2) return(0);

	server_lag.sec = egg_timeval_now.sec - sec;
	server_lag.usec = egg_timeval_now.usec - usec;

	return(0);
}

/* This is a reply on ISON :<current> <orig> [<alt>]
 */
static int got303(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	char *nicks, *space, *alt;
	int ison_orig = 0, ison_alt = 0;

	if (!keepnick || !strcmp(botname, origbotname)) return(0);
	alt = get_altbotnick();
	nicks = args[1];
	for (;;) {
		space = strchr(nicks, ' ');
		if (space) *space = 0;
		if (!irccmp(origbotname, nicks)) ison_orig = 1;
		else if (!irccmp(alt, nicks)) ison_alt = 1;
		if (space) {
			*space = ' ';
			nicks = space+1;
		}
		else break;
	}

	if (!ison_orig) {
		putlog(LOG_MISC, "*", _("Switching back to nick %s"), origbotname);
		dprintf(DP_SERVER, "NICK %s\n", origbotname);
	}
	else if (!ison_alt && strcmp(botname, alt)) {
		putlog(LOG_MISC, "*", _("Switching back to altnick %s"), alt);
		dprintf(DP_SERVER, "NICK %s\n", alt);
	}
	return(0);
}

/* 432 : Bad nickname
 */
static int got432(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	char *badnick;
	char newnick[10];

	badnick = args[1];
	if (server_online) putlog(LOG_MISC, "*", "NICK IS INVALID: %s (keeping '%s').", badnick, botname);
	else {
		putlog(LOG_MISC, "*", _("Server says my nickname is invalid, choosing new random nick."));
		if (!keepnick) {
			make_rand_str(newnick, 9);
			newnick[9] = 0;
			dprintf(DP_MODE, "NICK %s\n", newnick);
		}
	}
	return(0);
}

/* 433 : Nickname in use
 * Change nicks till we're acceptable or we give up
 */
static int got433(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	char *badnick;

	badnick = args[1];
	if (server_online) {
		/* We are online and have a nickname, we'll keep it */
		putlog(LOG_MISC, "*", "NICK IN USE: %s (keeping '%s').", badnick, botname);
		nick_juped = 0;
		return(0);
	}
	choose_altnick();
	return(0);
}

/* 437 : Nickname juped (IRCnet)
 */
static int got437(char *from, char *ignore, char *msg)
{
  char *s;
  struct chanset_t *chan;

  newsplit(&msg);
  s = newsplit(&msg);
  if (s[0] && (strchr(CHANMETA, s[0]) != NULL)) {
    chan = findchan(s);
    if (chan) {
      if (chan->status & CHAN_ACTIVE) {
	putlog(LOG_MISC, "*", _("Cant change nickname on %s.  Is my nickname banned?"), s);
      } else {
	if (!channel_juped(chan)) {
	  putlog(LOG_MISC, "*", _("Channel %s is juped. :("), s);
	  chan->status |= CHAN_JUPED;
	}
      }
    }
  } else if (server_online) {
    if (!nick_juped)
      putlog(LOG_MISC, "*", "NICK IS JUPED: %s (keeping '%s').", s, botname);
    if (!irccmp(s, origbotname))
      nick_juped = 1;
  } else {
    putlog(LOG_MISC, "*", "%s: %s", _("Nickname has been juped"), s);
    choose_altnick();
  }
  return 0;
}

/* 438 : Nick change too fast
 */
static int got438(char *from, char *ignore, char *msg)
{
	putlog(LOG_MISC, "*", "%s", _("Nick change was too fast."));
	return(0);
}

static int got451(char *from, char *ignore, char *msg)
{
  /* Usually if we get this then we really messed up somewhere
   * or this is a non-standard server, so we log it and kill the socket
   * hoping the next server will work :) -poptix
   */
  /* Um, this does occur on a lagged anti-spoof server connection if the
   * (minutely) sending of joins occurs before the bot does its ping reply.
   * Probably should do something about it some time - beldin
   */
  putlog(LOG_MISC, "*", _("%s says Im not registered, trying next one."), from);
  nuke_server(_("The server says we are not registered yet."));
  return 0;
}

/* Got error notice
 */
static int goterror(char *from, char *ignore, char *msg)
{
  fixcolon(msg);
  putlog(LOG_SERV | LOG_MSGS, "*", "-ERROR from server- %s", msg);
  putlog(LOG_SERV, "*", "Disconnecting from server.");
  nuke_server("Bah, stupid error messages.");
  return 1;
}

/* Got nick change.
 */
static int gotnick(char *from, char *ignore, char *msg)
{
  char *nick, *alt = get_altbotnick();
  char buf[512];
  struct userrec *u;

  fixcolon(msg);
  u = get_user_by_host(from);
  strlcpy(buf, from, sizeof(buf));
  nick = strtok(buf, "!");
  check_queues(nick, msg);
  if (match_my_nick(nick)) {
    /* Regained nick! */
    str_redup(&botname, msg);
    altnick_char = -1;
    if (!strcmp(msg, origbotname)) {
      putlog(LOG_SERV | LOG_MISC, "*", "Regained nickname '%s'.", msg);
      nick_juped = 0;
    } else if (alt[0] && !strcmp(msg, alt))
      putlog(LOG_SERV | LOG_MISC, "*", "Regained alternate nickname '%s'.",
	     msg);
    else if (keepnick && strcmp(nick, msg)) {
      putlog(LOG_SERV | LOG_MISC, "*", "Nickname changed to '%s'???", msg);
      if (!irccmp(nick, origbotname)) {
        putlog(LOG_MISC, "*", _("Switching back to nick %s"), origbotname);
        dprintf(DP_SERVER, "NICK %s\n", origbotname);
      } else if (alt[0] && !irccmp(nick, alt)
		 && strcasecmp(botname, origbotname)) {
        putlog(LOG_MISC, "*", _("Switching back to altnick %s"), alt);
        dprintf(DP_SERVER, "NICK %s\n", alt);
      }
    } else
      putlog(LOG_SERV | LOG_MISC, "*", "Nickname changed to '%s'???", msg);
  } else if ((keepnick) && (irccmp(nick, msg))) {
    /* Only do the below if there was actual nick change, case doesn't count */
    if (!irccmp(nick, origbotname)) {
      putlog(LOG_MISC, "*", _("Switching back to nick %s"), origbotname);
      dprintf(DP_SERVER, "NICK %s\n", origbotname);
    } else if (alt[0] && !irccmp(nick, alt) &&
	    strcasecmp(botname, origbotname)) {
      putlog(LOG_MISC, "*", _("Switching back to altnick %s"), altnick);
      dprintf(DP_SERVER, "NICK %s\n", altnick);
    }
  }
  return 0;
}

static int gotmode(char *from, char *ignore, char *msg)
{
  char *ch;
  char buf[512];

  strlcpy(buf, msg, sizeof(buf));
  msg = buf;
  ch = newsplit(&msg);
  /* Usermode changes? */
  if (strchr(CHANMETA, ch[0]) == NULL) {
    if (match_my_nick(ch) && check_mode_r) {
      /* umode +r? - D0H dalnet uses it to mean something different */
      fixcolon(msg);
      if ((msg[0] == '+') && strchr(msg, 'r')) {
	putlog(LOG_MISC | LOG_JOIN, "*",
	       "%s has me i-lined (jumping)", dcc[servidx].host);
	nuke_server("i-lines suck");
      }
    }
  }
  return 0;
}

static void disconnect_server()
{
	int idx;

	if (server_online > 0) check_bind_event("disconnect-server");

	server_online = 0;
	if (servidx != -1 && dcc[servidx].sock >= 0) {
		killsock(dcc[servidx].sock);
		dcc[servidx].sock = (-1);
	}
	serv = -1;
	botuserhost[0] = 0;
	idx = servidx;
	servidx = -1;
	lostdcc(idx);
}

static void eof_server(int idx)
{
  putlog(LOG_SERV, "*", "%s %s", _("Disconnected from"), dcc[idx].host);
  disconnect_server();
}

static void display_server(int idx, char *buf)
{
  sprintf(buf, "%s  (lag: %d.%02d)", trying_server ? "conn" : "serv",
	  server_lag.sec, server_lag.usec / 10000);
}

static void connect_server(void);

static void kill_server(int idx, void *x)
{
  module_entry *me;

  disconnect_server();
  if ((me = module_find("channels", 0, 0)) && me->funcs) {
    struct chanset_t *chan;

    for (chan = chanset; chan; chan = chan->next)
      (me->funcs[CHANNEL_CLEAR]) (chan, 1);
  }
  /* A new server connection will be automatically initiated in
     about 2 seconds. */
}

static void timeout_server(int idx)
{
  putlog(LOG_SERV, "*", "Timeout: connect to %s", dcc[idx].host);
  disconnect_server();
}

static void server_activity(int idx, char *msg, int len);

static struct dcc_table SERVER_SOCKET =
{
  "SERVER",
  0,
  eof_server,
  server_activity,
  NULL,
  timeout_server,
  display_server,
  kill_server,
  NULL
};

static void add_arg(int *nargs, char ***args, char *static_args[], char *arg)
{
	if (*nargs >= 10) {
		if (*nargs == 10) {
			*args = (char **)malloc(sizeof(char *) * 11);
			memcpy(*args, static_args, sizeof(char *) * 10);
		}
		else *args = (char **)realloc(*args, sizeof(char *) * (*nargs+1));
	}
	(*args)[*nargs] = arg;
	(*nargs)++;
}

static void server_activity(int idx, char *msg, int len)
{
	/* The components of any irc message. */
	char *prefix = NULL, *cmd = NULL;
	char *space;
	char *from_nick = NULL, *from_uhost = NULL;
	char *static_args[10], **args = static_args;
	char *remainder;
	int nargs = 0;
	struct userrec *u = NULL;

	if (trying_server) {
		strcpy(dcc[idx].nick, "(server)");
		putlog(LOG_SERV, "*", "Connected to %s", dcc[idx].host);
		trying_server = 0;
		SERVER_SOCKET.timeout_val = 0;
	}
	waiting_for_awake = 0;

	if (!len) return;

	remainder = strdup("");

	/* This would be a good place to put an SFILT bind, so that scripts
		and other modules can modify text sent from the server. */
	if (debug_output) {
		putlog(LOG_RAW, "*", "[@] %s", msg);
	}

	/* First word is the prefix, or command if it doesn't start with ':' */
	if (*msg == ':') {
		prefix = msg+1;
		msg = strchr(msg, ' ');
		if (!msg) goto done_parsing;
		*msg = 0;
		msg++;
	}

	cmd = msg;
	msg = strchr(msg, ' ');
	if (!msg) goto done_parsing;
	*msg = 0;
	msg++;

	/* Save the remainder to call the old binds (temporary). */
	str_redup(&remainder, msg);

	/* Next comes the args to the command. */
	while ((*msg != ':') && (space = strchr(msg, ' '))) {
		add_arg(&nargs, &args, static_args, msg);
		*space = 0;
		msg = space+1;
	}

	if (*msg == ':') add_arg(&nargs, &args, static_args, msg+1);

done_parsing:

	if (prefix) {
		u = get_user_by_host(prefix);
		from_nick = prefix;
		from_uhost = strchr(prefix, '!');
		if (from_uhost) {
			*from_uhost = 0;
			from_uhost++;
		}
	}

	check_bind(BT_new_raw, cmd, NULL, from_nick, from_uhost, u, cmd, nargs, args);

	if (args != static_args) free(args);

	/* For now, let's emulate the old style. */

	if (from_nick && from_uhost) prefix = msprintf("%s!%s", from_nick, from_uhost);
	else if (from_nick) prefix = strdup(from_nick);
	else prefix = strdup("");

	check_bind(BT_raw, cmd, NULL, prefix, cmd, remainder);
	free(prefix);
	free(remainder);
}

static int gotping(char *from, char *ignore, char *msg)
{
  fixcolon(msg);
  dprintf(DP_MODE, "PONG :%s\n", msg);
  return 0;
}

static int gotkick(char *from, char *ignore, char *msg)
{
  char *nick;

  nick = from;
  if (irccmp(nick, botname))
    /* Not my kick, I don't need to bother about it. */
    return 0;
  if (use_penalties) {
    last_time += 2;
    if (debug_output)
      putlog(LOG_SRVOUT, "*", "adding 2secs penalty (successful kick)");
  }
  return 0;
}

/* Another sec penalty if bot did a whois on another server.
 */
static int whoispenalty(char *from, char *msg)
{
  struct server_list *x = serverlist;
  int i, ii;

  if (x && use_penalties) {
    i = ii = 0;
    for (; x; x = x->next) {
      if (i == curserv)
          if (strcmp(x->name, from) || strcmp(x->realname, from))
            ii = 1;
      i++;
    }
    if (ii) {
      last_time += 1;
      if (debug_output)
        putlog(LOG_SRVOUT, "*", "adding 1sec penalty (remote whois)");
    }
  }
  return 0;
}

static int got311(char *from, char *ignore, char *msg)
{
  char *n1, *n2, *u, *h;
  
  n1 = newsplit(&msg);
  n2 = newsplit(&msg);
  u = newsplit(&msg);
  h = newsplit(&msg);
  
  if (!n1 || !n2 || !u || !h)
    return 0;
    
  if (match_my_nick(n2))
    snprintf(botuserhost, sizeof botuserhost, "%s@%s", u, h);
  
  return 0;
}

static cmd_t my_new_raw_binds[] = {
	{"001", "", (Function) got001, NULL},
	{"PRIVMSG", "", (Function) gotmsg, NULL},
	{"NOTICE", "", (Function) gotnotice, NULL},
	{"WALLOPS", "", (Function) gotwall, NULL},
	{"PONG", "", (Function) gotpong, NULL},
	{"303", "", (Function) got303, NULL},
	{"432",	"", (Function) got432, NULL},
	{"433",	"", (Function) got433, NULL},
	{"438", "", (Function) got438, NULL},
	{0}
};

static cmd_t my_raw_binds[] =
{
  {"MODE",	"",	(Function) gotmode,		NULL},
  {"PING",	"",	(Function) gotping,		NULL},
  {"437",	"",	(Function) got437,		NULL},
  {"451",	"",	(Function) got451,		NULL},
  {"442",	"",	(Function) got442,		NULL},
  {"NICK",	"",	(Function) gotnick,		NULL},
  {"ERROR",	"",	(Function) goterror,		NULL},
  {"KICK",	"",	(Function) gotkick,		NULL},
  {"318",	"",	(Function) whoispenalty,	NULL},
  {"311",	"",	(Function) got311,		NULL},
  {NULL,	NULL,	NULL,				NULL}
};

static void server_resolve_success(int);
static void server_resolve_failure(int);

/* Hook up to a server
 */
static void connect_server(void)
{
  char pass[121], botserver[UHOSTLEN];
  static int oldserv = -1;
  unsigned int botserverport = 0;

  waiting_for_awake = 0;
  trying_server = now;
  empty_msgq();

  if (oldserv < 0)
    oldserv = curserv;
  if (newserverport) {		/* Jump to specified server */
    curserv = (-1);		/* Reset server list */
    strcpy(botserver, newserver);
    botserverport = newserverport;
    strcpy(pass, newserverpass);
    newserver[0] = 0;
    newserverport = 0;
    newserverpass[0] = 0;
    oldserv = (-1);
  } else
    pass[0] = 0;
  if (!cycle_time) {
    struct chanset_t *chan;
    struct server_list *x = serverlist;

    if (!x) {
      putlog(LOG_SERV, "*", "No servers in server list");
      cycle_time = 300;
      return;
    }
 
    servidx = new_dcc(&DCC_DNSWAIT, sizeof(struct dns_info));
    if (servidx < 0) {
      putlog(LOG_SERV, "*",
	     "NO MORE DCC CONNECTIONS -- Can't create server connection.");
      return;
    }

    check_bind_event("connect-server");
    next_server(&curserv, botserver, &botserverport, pass);
    putlog(LOG_SERV, "*", "%s %s %d", _("Trying server"), botserver, botserverport);

    dcc[servidx].port = botserverport;
    strcpy(dcc[servidx].nick, "(server)");
    strlcpy(dcc[servidx].host, botserver, UHOSTLEN);

    botuserhost[0] = 0;

    nick_juped = 0;
    for (chan = chanset; chan; chan = chan->next)
      chan->status &= ~CHAN_JUPED;

    dcc[servidx].timeval = now;
    dcc[servidx].sock = -1;
    dcc[servidx].u.dns->host = calloc(1, strlen(dcc[servidx].host) + 1);
    strcpy(dcc[servidx].u.dns->host, dcc[servidx].host);
    dcc[servidx].u.dns->cbuf = calloc(1, strlen(pass) + 1);
    strcpy(dcc[servidx].u.dns->cbuf, pass);
    dcc[servidx].u.dns->dns_success = server_resolve_success;
    dcc[servidx].u.dns->dns_failure = server_resolve_failure;
    dcc[servidx].u.dns->dns_type = RES_IPBYHOST;
    dcc[servidx].u.dns->type = &SERVER_SOCKET;

    if (server_cycle_wait)
      /* Back to 1st server & set wait time.
       * Note: Put it here, just in case the server quits on us quickly
       */
      cycle_time = server_cycle_wait;
    else
      cycle_time = 0;

    /* I'm resolving... don't start another server connect request */
    resolvserv = 1;
    /* Resolve the hostname. */
    dcc_dnsipbyhost(dcc[servidx].host);
  }
}

static void server_resolve_failure(int servidx)
{
  serv = -1;
  servidx = -1;
  resolvserv = 0;
  putlog(LOG_SERV, "*", "%s %s (%s)", _("Failed connect to"), dcc[servidx].host,
	 _("DNS lookup failed"));
  lostdcc(servidx);
}

static void server_resolve_success(int servidx)
{
  char s[121], pass[121];

  resolvserv = 0;
  
  strcpy(dcc[servidx].addr, dcc[servidx].u.dns->host);
  strcpy(pass, dcc[servidx].u.dns->cbuf);
  changeover_dcc(servidx, &SERVER_SOCKET, 0);
  serv = open_telnet(dcc[servidx].addr, dcc[servidx].port);
  if (serv < 0) {
    neterror(s);
    putlog(LOG_SERV, "*", "%s %s (%s)", _("Failed connect to"), dcc[servidx].host,
	   s);
    lostdcc(servidx);
    servidx = -1;
  } else {
    dcc[servidx].sock = serv;
    /* Queue standard login */
    dcc[servidx].timeval = now;
    SERVER_SOCKET.timeout_val = &server_timeout;
    /* Another server may have truncated it, so use the original */
    str_redup(&botname, origbotname);
    /* Start alternate nicks from the beginning */
    altnick_char = -1;
    if (pass[0]) dprintf(DP_MODE, "PASS %s\r\n", pass);
    dprintf(DP_MODE, "NICK %s\r\n", botname);
    dprintf(DP_MODE, "USER %s localhost %s :%s\r\n", botuser, dcc[servidx].host, botrealname);
    /* Wait for async result now */
  }
}
