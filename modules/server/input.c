/*
 * servmsg.c --
 */

#include "lib/eggdrop/module.h"
#include "server.h"
#include "channels.h"
#include "parse.h"
#include "output.h"
#include "binds.h"
#include "servsock.h"
#include "nicklist.h"

/* 001: welcome to IRC */
static int got001(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	current_server.registered = 1;

	/* First arg is what server decided our nick is. */
	str_redup(&current_server.nick, args[0]);

	/* Save the name the server calls itself. */
	str_redup(&current_server.server_self, from_nick);

	check_bind_event("init-server");

	/* If the init-server bind made us leave the server, stop processing. */
	if (!current_server.registered) return BIND_RET_BREAK;

	/* Send a whois request so we can see our nick!user@host */
	printserv(SERVER_MODE, "WHOIS %s", current_server.nick);

	/* Join all our channels. */
	channels_join_all();

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
      printserv(SERVER_MODE, "JOIN %s %s", chan->name,
	      chan->channel.key[0] ? chan->channel.key : chan->key_prot);
  }

  return 0;
}

static int check_ctcp_ctcr(int which, int to_channel, struct userrec *u, char *nick, char *uhost, char *dest, char *trailing)
{
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

	r = bind_check(table, cmd, nick, uhost, u, dest, cmd, text);

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
		r = bind_check(BT_pub, first, from_nick, from_uhost, u, dest, text);
		if (r & BIND_RET_LOG) {
			putlog(LOG_CMDS, dest, "<<%s>> !%s! %s %s", from_nick, u ? u->handle : "*", first, text);
		}
	}
	else {
		r = bind_check(BT_msg, first, from_nick, from_uhost, u, text);
		if (r & BIND_RET_LOG) {
			putlog(LOG_CMDS, "*", "(%s!%s) !%s! %s %s", from_nick, from_uhost, u ? u->handle : "*", first, text);
		}
	}

	if (space) *space = ' ';

	if (r & BIND_RET_BREAK) return(0);

	/* And now the stackable version. */
	if (to_channel) {
		r = bind_check(BT_pubm, trailing, from_nick, from_uhost, u, dest, trailing);
	}
	else {
		r = bind_check(BT_msg, trailing, from_nick, from_uhost, u, trailing);
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
	r = bind_check(BT_notice, trailing, from_nick, from_uhost, u, dest, trailing);

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
	r = bind_check(BT_wall, msg, from_nick, msg);
	if (!(r & BIND_RET_BREAK)) {
		if (from_uhost) putlog(LOG_WALL, "*", "!%s (%s)! %s", from_nick, from_uhost, msg);
		else putlog(LOG_WALL, "*", "!%s! %s", from_nick, msg);
	}
	return 0;
}


/* 432 : Bad nickname
 * If we're registered already, then just inform the user and keep the current
 * nick. Otherwise, generate a random nick so that we can get registered. */
static int got432(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	char *badnick;

	badnick = args[1];
	if (current_server.registered) {
		putlog(LOG_MISC, "*", "NICK IS INVALID: %s (keeping '%s').", badnick, botname);
	}
	else {
		putlog(LOG_MISC, "*", "Server says my nickname ('%s') is invalid, trying new random nick.", badnick);
		try_random_nick();
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
	if (current_server.registered) {
		/* We are online and have a nickname, we'll keep it */
		putlog(LOG_MISC, "*", "Nick is in use: '%s' (keeping '%s')", badnick, botname);
	}
	else {
		putlog(LOG_MISC, "*", "Nick is in use: '%s' (trying next one)", badnick, botname);
		try_next_nick();
	}
	return(0);
}

/* 435 : Cannot change to a banned nickname. */
static int got435(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	char *banned_nick, *chan;

	banned_nick = args[1];
	chan = args[2];
	putlog(LOG_MISC, "*", "Cannot change to banned nickname (%s on %s)", banned_nick, chan);
	return(0);
}

/* 437 : Nickname juped (IRCnet) */
static int got437(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	char *chan;

	chan = args[1];
	putlog(LOG_MISC, "*", "Can't change nickname to %s. Is my nickname banned?", chan);

	/* If we aren't registered yet, try another nick. Otherwise just keep
	 * our current nick (no action). */
	if (!current_server.registered) {
		try_next_nick();
	}
	return(0);
}

/* 438 : Nick change too fast */
static int got438(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	putlog(LOG_MISC, "*", "%s", _("Nick change was too fast."));
	return(0);
}

static int got451(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
  /* Usually if we get this then we really messed up somewhere
   * or this is a non-standard server, so we log it and kill the socket
   * hoping the next server will work :) -poptix
   */
  putlog(LOG_MISC, "*", _("%s says I'm not registered, trying next one."), from_nick);
  kill_server("The server says we are not registered yet.");
  return 0;
}

/* Got error */
static int goterror(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
  putlog(LOG_SERV | LOG_MSGS, "*", "-ERROR from server- %s", args[0]);
  putlog(LOG_SERV, "*", "Disconnecting from server.");
  kill_server("disconnecting due to error");
  return 1;
}

/* Got nick change.  */
static int gotnick(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	char *newnick = args[0];

	if (match_my_nick(from_nick)) str_redup(&botname, newnick);
	return(0);
}

/* Pings are immediately returned, no queue. */
static int gotping(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	printserv(SERVER_NOQUEUE, "PONG :%s", args[0]);
	return(0);
}

/* 311 : save our user@host from whois reply */
static int got311(char *from_nick, char *from_uhost, struct userrec *u, char *cmd, int nargs, char *args[])
{
	char *nick, *user, *host, *realname;
  
	if (nargs < 6) return(0);

	nick = args[1];
	user = args[2];
	host = args[3];
	realname = args[5];
  
	if (match_my_nick(nick)) {
		str_redup(&current_server.user, user);
		str_redup(&current_server.host, host);
		str_redup(&current_server.real_name, realname);
	}
	return(0);
}

bind_list_t my_new_raw_binds[] = {
	{"PRIVMSG", (Function) gotmsg},
	{"NOTICE", (Function) gotnotice},
	{"WALLOPS", (Function) gotwall},
	{"PING", (Function) gotping},
	{"NICK", (Function) gotnick},
	{"ERROR", (Function) goterror},
	{"001", (Function) got001},
	{"432",	(Function) got432},
	{"433",	(Function) got433},
	{"435", (Function) got435},
	{"438", (Function) got438},
	{"437",	(Function) got437},
	{"451",	(Function) got451,},
	{"442",	(Function) got442,},
	{"311", (Function) got311,},
	{NULL, NULL, NULL, NULL}
};
