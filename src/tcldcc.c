/*
 * tcldcc.c --
 *
 *	Tcl stubs for the dcc commands
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
static const char rcsid[] = "$Id: tcldcc.c,v 1.70 2003/01/18 22:36:52 wcc Exp $";
#endif

#include "main.h"
#include "modules.h"
#include "logfile.h"
#include "misc.h"
#include "cmdt.h"		/* cmd_t				*/
#include "chanprog.h"		/* masktype, logmodes			*/
#include "dccutil.h"		/* chatout, chanout_but, lostdcc
				   not_away, set_away, new_dcc		*/
#include "net.h"		/* tputs, sockoptions, killsock,
				   getsock, open_telnet_raw, neterror,
				   open_listen				*/
#include "userrec.h"		/* write_userfile			*/
#include "traffic.h" 		/* egg_traffic_t			*/

extern struct dcc_t	*dcc;
extern int		 dcc_total, backgrd, parties, make_userfile,
			 do_restart, remote_boots, max_dcc;
extern char		 botnetnick[];
extern time_t		 now;

/* Traffic stuff. */
extern egg_traffic_t traffic;

#ifndef MAKING_MODS
extern struct dcc_table	DCC_CHAT, DCC_SCRIPT, DCC_TELNET, DCC_SOCKET; 
#endif /* MAKING_MODS   */

/***********************************************************************/

static int script_putdcc(int idx, char *text)
{
  if (idx < 0 ||  idx >= dcc_total || !dcc[idx].type) return(1);
  dumplots(-(dcc[idx].sock), "", text);
  return(0);
}

/* Allows tcl scripts to send out raw data. Can be used for fast server
 * write (idx=-1)
 *
 * usage:
 * 	putdccraw <idx> <size> <rawdata>
 * example:
 * 	putdccraw 6 13 "eggdrop rulz\n"
 *
 * (added by drummer@sophia.jpte.hu)
 */

static int script_putdccraw(int idx, int len, char *text)
{
  int i;

  if (idx == -1) {
    /* -1 means search for the server's idx. */
    for (i = 0; i < dcc_total; i++) {
      if (dcc[i].type && !strcmp(dcc[i].nick, "(server)")) {
        idx = i;
        break;
      }
    }
  }
  if (idx < 0 || idx >= dcc_total || !dcc[idx].type) return(1);
  tputs(dcc[idx].sock, text, len);
  return(0);
}

static int script_dccsimul(int idx, char *cmd)
{
  int len;
  if (idx < 0 || !dcc->type || !(dcc[idx].type->flags & DCT_SIMUL)
    || !(dcc[idx].type->activity)) return(1);

  len = strlen(cmd);
  if (len > 510) len = 510;

  dcc[idx].type->activity(idx, cmd, len);
  return(0);
}

static int script_dccbroadcast(char *msg)
{
  chatout("*** %s\n", msg);
  return(0);
}

static int script_hand2idx(char *nick)
{
  int i;

  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type) && (dcc[i].type->flags & DCT_SIMUL) &&
        !strcasecmp(nick, dcc[i].nick)) {
      return(i);
    }
  }
  return(-1);
}

static int script_console(script_var_t *retval, int nargs, int idx, char *what)
{
	static char *view[2];
	char str[2];
	int plus;

	str[1] = 0;

	if (idx < 0 || idx >= dcc_total || !dcc[idx].type || dcc[idx].type != &DCC_CHAT) {
		retval->value = "invalid idx";
		retval->len = 10;
		retval->type = SCRIPT_ERROR | SCRIPT_STRING;
	}

	retval->type = SCRIPT_ARRAY | SCRIPT_STRING;
	retval->len = 2;
	view[0] = dcc[idx].u.chat->con_chan;
	view[1] = masktype(dcc[idx].u.chat->con_flags);
	retval->value = (void *)view;

	if (nargs != 2) {
		view[1] = masktype(dcc[idx].u.chat->con_flags);
		return(0); /* Done. */
	}

	/* They want to change something. */
	if (strchr(CHANMETA, what[0]) != NULL) {
		/* The channel. */
		strlcpy(dcc[idx].u.chat->con_chan, what, 80);
		return(0);
	}

	/* The flags. */
	if (*what != '+' && *what != '-') dcc[idx].u.chat->con_flags = 0;
	for (plus = 1; *what; what++) {
		if (*what == '-') plus = 0;
		else if (*what == '+') plus = 1;
		else {
			str[0] = *what;
			if (plus) dcc[idx].u.chat->con_flags |= logmodes(str);
			else dcc[idx].u.chat->con_flags &= (~logmodes(str));
		}
	}
	view[1] = masktype(dcc[idx].u.chat->con_flags);
	return(0);
}

static int script_echo(int nargs, int idx, int status)
{
	if (idx < 0 || idx >= dcc_total || !dcc[idx].type || dcc[idx].type != &DCC_CHAT) return(0);
	if (nargs == 2) {
		if (status) dcc[idx].status |= STAT_ECHO;
		else dcc[idx].status &= (~STAT_ECHO);
	}

	if (dcc[idx].status & STAT_ECHO) return(1);
	return(0);
}

static int script_page(int nargs, int idx, int status)
{
	if (idx < 0 || idx >= dcc_total || !dcc[idx].type || dcc[idx].type != &DCC_CHAT) return(0);

	if (nargs == 2) {
		if (status) {
			dcc[idx].status |= STAT_PAGE;
			dcc[idx].u.chat->max_line = status;
		}
		else dcc[idx].status &= (~STAT_PAGE);
	}
	if (dcc[idx].status & STAT_PAGE) return(dcc[idx].u.chat->max_line);
	return(0);
}

static int script_control(int idx, script_callback_t *callback)
{
  void *hold;

  if (idx < 0 || idx >= max_dcc) {
	callback->syntax = strdup("");
	callback->del(callback);
		return(-1);
	}

  if (dcc[idx].type && dcc[idx].type->flags & DCT_CHAT) {
    if (dcc[idx].u.chat->channel >= 0) {
      chanout_but(idx, dcc[idx].u.chat->channel, "*** %s has gone.\n",
		  dcc[idx].nick);
    }
  }

	if (dcc[idx].type == &DCC_SCRIPT) {
		script_callback_t *old_callback;

		/* If it's already controlled, overwrite the old handler. */
		old_callback = dcc[idx].u.script->callback;
		if (old_callback) old_callback->del(old_callback);
		dcc[idx].u.script->callback = callback;
	}
	else {
		/* Otherwise we have to save the old handler. */
		hold = dcc[idx].u.other;
		dcc[idx].u.script = calloc(1, sizeof(struct script_info));
		dcc[idx].u.script->u.other = hold;
		dcc[idx].u.script->type = dcc[idx].type;
		dcc[idx].u.script->callback = callback;
		dcc[idx].type = &DCC_SCRIPT;

		/* Do not buffer data anymore. All received and stored data
			is passed over to the dcc functions from now on.  */
		sockoptions(dcc[idx].sock, EGG_OPTION_UNSET, SOCK_BUFFER);
	}

	callback->syntax = strdup("is");
	return(0);
}

static int script_valididx(int idx)
{
	if (idx < 0 || idx >= dcc_total || !dcc[idx].type || !(dcc[idx].type->flags & DCT_VALIDIDX)) return(0);
	return(1);
}

/*
	idx - idx to kill
	reason - optional message to send with the kill
*/
static int script_killdcc(int idx, char *reason)
{
	if (idx < 0 || idx >= dcc_total || !dcc[idx].type || !(dcc[idx].type->flags & DCT_VALIDIDX)) return(-1);

	/* Don't kill terminal socket */
	if ((dcc[idx].sock == STDOUT) && !backgrd) return(0);

	/* Make sure 'whom' info is updated for other bots */
	if (dcc[idx].type->flags & DCT_CHAT) chanout_but(idx, dcc[idx].u.chat->channel, "*** %s has left the %s%s%s\n", dcc[idx].nick, dcc[idx].u.chat ? "channel" : "partyline", reason ? ": " : "", reason ? reason : "");
	/* Notice is sent to the party line, the script can add a reason. */

	killsock(dcc[idx].sock);
	killtransfer(idx);
	lostdcc(idx);
	return(TCL_OK);
}

static char *script_idx2hand(int idx)
{
	if (idx < 0 || idx >= dcc_total || !(dcc[idx].type) || !(dcc[idx].nick)) return("");
	return(dcc[idx].nick);
}

/* list of { idx nick host type {other}  timestamp}
 */
static int script_dcclist(script_var_t *retval, char *match)
{
	int i;
	script_var_t *sublist, *idx, *nick, *host, *type, *othervar, *timestamp;
	char other[160];

	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
	retval->len = 0;
	retval->value = NULL;
	for (i = 0; i < dcc_total; i++) {
		if (!(dcc[i].type)) continue;
		if (match && strcasecmp(dcc[i].type->name, match)) continue;

		idx = script_int(i);
		nick = script_string(dcc[i].nick, -1);
		host = script_string(dcc[i].host, -1);
		type = script_string(dcc[i].type->name, -1);
		if (dcc[i].type->display) dcc[i].type->display(i, other);
		else sprintf(other, "?:%X !! ERROR !!", (int) dcc[i].type);
		othervar = script_copy_string(other, -1);
		othervar->type |= SCRIPT_FREE;
		timestamp = script_int(dcc[i].timeval);

		sublist = script_list(6, idx, nick, host, type, othervar, timestamp);
		script_list_append(retval, sublist);
	}
	return(0);
}

static int script_dccused()
{
	return(dcc_total);
}

static int script_getdccidle(int idx)
{
	if (idx < 0 || idx >= dcc_total || !(dcc->type)) return(-1);
	return(now - dcc[idx].timeval);
}

static char *script_getdccaway(int idx)
{
	if (idx < 0 || idx >= dcc_total || !(dcc[idx].type) || dcc[idx].type != &DCC_CHAT) return("");
	if (dcc[idx].u.chat->away == NULL) return("");
	return(dcc[idx].u.chat->away);
}

static int script_setdccaway(int idx, char *text)
{
	if (idx < 0 || idx >= dcc_total || !(dcc[idx].type) || dcc[idx].type != &DCC_CHAT) return(1);
	if (!text || !text[0]) {
		/* un-away */
		if (dcc[idx].u.chat->away != NULL) not_away(idx);
	}
	else set_away(idx, text);
	return(0);
}

static int script_connect(char *hostname, int port)
{
  int i, z, sock;

  sock = getsock(0);
  if (sock < 0) return(-1);
  z = open_telnet_raw(sock, hostname, port);
  if (z < 0) {
    killsock(sock);
    return(-1);
  }

  /* Well well well... it worked! */
  i = new_dcc(&DCC_SOCKET, 0);
  dcc[i].sock = sock;
  dcc[i].port = port;
  strcpy(dcc[i].nick, "*");
  strlcpy(dcc[i].host, hostname, UHOSTMAX);
  return(i);
}

/* Create a new listening port (or destroy one)
 *
 * listen <port> bots/all/users [mask]
 * listen <port> script <proc> [flag]
 * listen <port> off
 */
/* These multi-purpose commands are silly. Let's break it up. */

static int find_idx_by_port(int port)
{
	int idx;
	for (idx = 0; idx < dcc_total; idx++) {
		if ((dcc[idx].type == &DCC_TELNET) && (dcc[idx].port == port)) break;
	}
	if (idx == dcc_total) return(-1);
	return(idx);
}

static int script_listen_off(int port)
{
	int idx;

	idx = find_idx_by_port(port);
	if (idx < 0) return(-1);
	killsock(dcc[idx].sock);
	lostdcc(idx);
	return(0);
}

static int get_listen_dcc(char *port)
{
	int af = AF_INET;
	int portnum;
	int sock;
	int idx;

	if (!strncasecmp(port, "ipv6%", 5)) {
		#ifdef AF_INET6
			port += 5;
			af = AF_INET6;
		#else
			return(-1);
		#endif
	}
	else if (!strncasecmp(port, "ipv4%", 5)) {
		port += 5;
	}

	portnum = atoi(port);

	idx = find_idx_by_port(portnum);

	if (idx == -1) {
		sock = open_listen(&portnum, af);
		if (sock == -1) {
			putlog(LOG_MISC, "*", "Requested port is in use.");
			return(-1);
		}
		else if (sock == -2) {
			putlog(LOG_MISC, "*", "Couldn't assign the requested IP. Please make sure 'my_ip' and/or 'my_ip6' are set properly.");
			return(-1);
		}

		idx = new_dcc(&DCC_TELNET, 0);
		strcpy(dcc[idx].addr, "*"); /* who cares? */
		dcc[idx].port = portnum;
		dcc[idx].sock = sock;
		dcc[idx].timeval = now;
	}
	return(idx);
}

static int script_listen_script(char *port, script_callback_t *callback, char *flags)
{
	int idx;

	if (flags && strcasecmp(flags, "pub")) return(-1);

	idx = get_listen_dcc(port);
	if (idx < 0) return(idx);

	strcpy(dcc[idx].nick, "(script)");
	if (flags) dcc[idx].status = LSTN_PUBLIC;
	strlcpy(dcc[idx].host, callback->name, UHOSTMAX);

	return(dcc[idx].port);
}

static int script_listen(char *port, char *type, char *mask)
{
	int idx;

	if (strcmp(type, "bots") && strcmp(type, "users") && strcmp(type, "all")) return(-1);

	idx = get_listen_dcc(port);
	if (idx < 0) return(idx);

	/* bots/users/all */
	sprintf(dcc[idx].nick, "(%s)", type);

	if (mask) strlcpy(dcc[idx].host, mask, UHOSTMAX);
	else strcpy(dcc[idx].host, "*");

	putlog(LOG_MISC, "*", "Listening at telnet port %d (%s)", dcc[idx].port, type);
	return(dcc[idx].port);
}

static int script_boot(char *user_bot, char *reason)
{
  char who[NOTENAMELEN + 1];
  int i;

  strlcpy(who, user_bot, sizeof who);
  if (strchr(who, '@') != NULL) {
    char whonick[HANDLEN + 1];

    splitc(whonick, who, '@');
    whonick[HANDLEN] = 0;
    if (!strcasecmp(who, botnetnick))
       strlcpy(who, whonick, sizeof who);
    else
        return(0);
  }
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type) && (dcc[i].type->flags & DCT_CANBOOT) &&
        !strcasecmp(dcc[i].nick, who)) {
      do_boot(i, botnetnick, reason ? reason : "");
      break;
    }
  return(0);
}

static int script_rehash()
{
  if (make_userfile) {
    putlog(LOG_MISC, "*", _("Userfile creation not necessary--skipping"));
    make_userfile = 0;
  }
  write_userfile(-1);
  putlog(LOG_MISC, "*", _("Rehashing..."));
  do_restart = -2;
  return(0);
}

static int script_restart()
{
  if (!backgrd) return(1);
  if (make_userfile) {
    putlog(LOG_MISC, "*", _("Userfile creation not necessary--skipping"));
    make_userfile = 0;
  }
  write_userfile(-1);
  putlog(LOG_MISC, "*", _("Restarting..."));
  do_restart = -1;
  return(0);
}

static int script_traffic(script_var_t *retval)
{
	unsigned long out_total_today, out_total;
	unsigned long in_total_today, in_total;
	script_var_t *sublist;

	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
	retval->len = 0;
	retval->value = NULL;

	sublist = script_list(5, script_string("irc", -1),
		script_int(traffic.in_today.irc),
		script_int(traffic.in_today.irc + traffic.in_total.irc),
		script_int(traffic.out_today.irc),
		script_int(traffic.out_today.irc + traffic.out_total.irc));
	script_list_append(retval, sublist);
/*
	sublist = script_list(5, script_string("botnet", -1),
		script_int(traffic.in_today.bn),
		script_int(traffic.in_today.bn + traffic.in_total.bn),
		script_int(traffic.out_today.bn),
		script_int(traffic.out_today.bn + traffic.out_total.bn));
	script_list_append(retval, sublist);
*/
	sublist = script_list(5, script_string("dcc", -1),
		script_int(traffic.in_today.dcc),
		script_int(traffic.in_today.dcc + traffic.in_total.dcc),
		script_int(traffic.out_today.dcc),
		script_int(traffic.out_today.dcc + traffic.out_total.dcc));
	script_list_append(retval, sublist);

	sublist = script_list(5, script_string("transfer", -1),
		script_int(traffic.in_today.trans),
		script_int(traffic.in_today.trans + traffic.in_total.trans),
		script_int(traffic.out_today.trans),
		script_int(traffic.out_today.trans + traffic.out_total.trans));
	script_list_append(retval, sublist);

	sublist = script_list(5, script_string("filesys", -1),
		script_int(traffic.in_today.filesys),
		script_int(traffic.in_today.filesys + traffic.in_total.filesys),
		script_int(traffic.out_today.filesys),
		script_int(traffic.out_today.filesys + traffic.out_total.filesys));
	script_list_append(retval, sublist);

	sublist = script_list(5, script_string("unknown", -1),
		script_int(traffic.in_today.unknown),
		script_int(traffic.in_today.unknown + traffic.in_total.unknown),
		script_int(traffic.out_today.unknown),
		script_int(traffic.out_today.unknown + traffic.out_total.unknown));
	script_list_append(retval, sublist);

	/* Totals */
	in_total_today = traffic.in_today.irc + traffic.in_today.bn + traffic.in_today.dcc + traffic.in_today.trans + traffic.in_today.unknown;

	in_total = in_total_today + traffic.in_total.irc + traffic.in_total.bn + traffic.in_total.dcc + traffic.in_total.trans + traffic.in_total.unknown;

	out_total_today = traffic.out_today.irc + traffic.out_today.bn + traffic.out_today.dcc + traffic.in_today.trans + traffic.out_today.unknown;

	out_total = out_total_today + traffic.out_total.irc + traffic.out_total.bn + traffic.out_total.dcc + traffic.out_total.trans + traffic.out_total.unknown;	  

	sublist = script_list(5, script_string("total", -1),
		script_int(in_total_today),
		script_int(in_total),
		script_int(out_total_today),
		script_int(out_total));
	script_list_append(retval, sublist);

	return(0);
}

script_command_t script_dcc_cmds[] = {
	{"", "putdcc", script_putdcc, NULL, 2, "is", "idx text", SCRIPT_INTEGER, 0},
	{"", "putdccraw", script_putdccraw, NULL, 3, "iis", "idx len text", SCRIPT_INTEGER, 0},
	{"", "dccsimul", script_dccsimul, NULL, 2, "is", "idx command", SCRIPT_INTEGER, 0},
	{"", "dccbroadcast", script_dccbroadcast, NULL, 1, "s", "text", SCRIPT_INTEGER, 0},
	{"", "hand2idx", script_hand2idx, NULL, 1, "s", "handle", SCRIPT_INTEGER, 0},
	{"", "valididx", script_valididx, NULL, 1, "i", "idx", SCRIPT_INTEGER, 0},
	{"", "idx2hand", script_idx2hand, NULL, 1, "i", "idx", SCRIPT_INTEGER, 0},
	{"", "dccused", script_dccused, NULL, 0, "", "", SCRIPT_INTEGER, 0},
	{"", "getdccidle", script_getdccidle, NULL, 1, "i", "idx", SCRIPT_INTEGER, 0},
	{"", "getdccaway", script_getdccaway, NULL, 1, "i", "idx", SCRIPT_STRING, 0},
	{"", "setdccaway", script_setdccaway, NULL, 2, "is", "idx msg", SCRIPT_INTEGER, 0},
	{"", "boot", script_boot, NULL, 2,"ss", "user@bot reason", SCRIPT_INTEGER, 0},
	{"", "rehash", script_rehash, NULL, 0,"", "", SCRIPT_INTEGER, 0},
	{"", "restart", script_restart, NULL, 0, "", "", SCRIPT_INTEGER, 0},
	{"", "console", script_console, NULL, 1, "is", "idx ?changes?", 0, SCRIPT_PASS_RETVAL|SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{"", "echo", script_echo, NULL, 1, "ii", "idx ?status?", SCRIPT_INTEGER, SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{"", "page", script_page, NULL, 1, "ii", "idx ?status?", SCRIPT_INTEGER, SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{"", "dcclist", script_dcclist, NULL, 0, "s", "?match?", 0, SCRIPT_PASS_RETVAL|SCRIPT_VAR_ARGS},
	{"", "control", script_control, NULL, 1, "ic", "idx ?callback?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "killdcc", script_killdcc, NULL, 1, "i", "idx", SCRIPT_INTEGER, 0},
	{"", "connect", script_connect, NULL, 2, "si", "host port", SCRIPT_INTEGER, 0},
	{"", "listen_off", script_listen_off, NULL, 1, "i", "port", SCRIPT_INTEGER, 0},
	{"", "listen_script", script_listen_script, NULL, 2, "scs", "port callback ?flags?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "listen", script_listen, NULL, 2, "sss", "port type ?mask?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "traffic", script_traffic, NULL, 0, "", "", 0, SCRIPT_PASS_RETVAL},
	{0}
};
