/*
 * tcldcc.c -- handles:
 *   Tcl stubs for the dcc commands
 *
 * $Id: tcldcc.c,v 1.57 2002/04/28 03:13:39 ite Exp $
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

#include "main.h"
#include "tandem.h"
#include "modules.h"
#include "logfile.h"
#include "misc.h"

extern struct dcc_t	*dcc;
extern int		 dcc_total, backgrd, parties, make_userfile,
			 do_restart, remote_boots, max_dcc;
extern char		 botnetnick[];
extern party_t		*party;
extern tand_t		*tandbot;
extern time_t		 now;

/* Traffic stuff. */
extern unsigned long otraffic_irc, otraffic_irc_today, itraffic_irc, itraffic_irc_today, otraffic_bn, otraffic_bn_today, itraffic_bn, itraffic_bn_today, otraffic_dcc, otraffic_dcc_today, itraffic_dcc, itraffic_dcc_today, otraffic_trans, otraffic_trans_today, itraffic_trans, itraffic_trans_today, otraffic_unknown, otraffic_unknown_today, itraffic_unknown, itraffic_unknown_today;

static struct portmap	*root = NULL;


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
  botnet_send_chat(-1, botnetnick, msg);
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

static int script_getchan(int idx)
{
  if (idx < 0 || !(dcc[idx].type) ||
      (dcc[idx].type != &DCC_CHAT && dcc[idx].type != &DCC_SCRIPT)) {
    return(-2);
  }
  if (dcc[idx].type == &DCC_SCRIPT)
    return(dcc[idx].u.script->u.chat->channel);
  else
    return(dcc[idx].u.chat->channel);
}

static int script_setchan(int idx, int chan)
{
  int oldchan;

  if (idx < 0 || !(dcc[idx].type) ||
      (dcc[idx].type != &DCC_CHAT && dcc[idx].type != &DCC_SCRIPT)) {
    return(1);
  }

  if ((chan < -1) || (chan > 199999)) {
    return(1);
  }
  if (dcc[idx].type == &DCC_SCRIPT) {
    dcc[idx].u.script->u.chat->channel = chan;
    return(0);
  }

  oldchan = dcc[idx].u.chat->channel;

  if (oldchan >= 0) {
    if ((chan >= GLOBAL_CHANS) && (oldchan < GLOBAL_CHANS)) botnet_send_part_idx(idx, "*script*");
    check_tcl_chpt(botnetnick, dcc[idx].nick, idx, oldchan);
  }
  dcc[idx].u.chat->channel = chan;
  if (chan < GLOBAL_CHANS) botnet_send_join_idx(idx, oldchan);
  check_tcl_chjn(botnetnick, dcc[idx].nick, chan, geticon(dcc[idx].user),
	   idx, dcc[idx].host);
  return(0);
}

static int script_dccputchan(int chan, char *msg)
{
  if ((chan < 0) || (chan > 199999)) return(1);
  chanout_but(-1, chan, "*** %s\n", msg);
  botnet_send_chan(-1, botnetnick, NULL, chan, msg);
  check_tcl_bcst(botnetnick, chan, msg);
  return(0);
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

static int script_strip(script_var_t *retval, int nargs, int idx, char *what)
{
	char str[2];
	int plus;

	str[1] = 0;

	if (idx < 0 || idx >= dcc_total || !dcc[idx].type || dcc[idx].type != &DCC_CHAT) {
		retval->value = "invalid idx";
		retval->len = 10;
		retval->type = SCRIPT_ERROR | SCRIPT_STRING;
	}

	retval->len = -1;
	retval->type = SCRIPT_STRING;

	if (nargs == 1) {
		retval->value = stripmasktype(dcc[idx].u.chat->strip_flags);
		return(0);
	}

	/* The flags. */
	if (*what != '+' && *what != '-') dcc[idx].u.chat->strip_flags = 0;
	for (plus = 1; *what; what++) {
		if (*what == '-') plus = 0;
		else if (*what == '+') plus = 1;
		else {
			str[0] = *what;
			if (plus) dcc[idx].u.chat->con_flags |= stripmodes(str);
			else dcc[idx].u.chat->con_flags &= (~stripmodes(str));
		}
	}

	retval->value = stripmasktype(dcc[idx].u.chat->strip_flags);
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

static int tcl_control STDVAR
{
  int idx;
  void *hold;

  BADARGS(3, 3, " idx command");
  idx = atoi(argv[1]);
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[idx].type && dcc[idx].type->flags & DCT_CHAT) {
    if (dcc[idx].u.chat->channel >= 0) {
      chanout_but(idx, dcc[idx].u.chat->channel, "*** %s has gone.\n",
		  dcc[idx].nick);
      check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
		     dcc[idx].u.chat->channel);
      botnet_send_part_idx(idx, "gone");
    }
    check_tcl_chof(dcc[idx].nick, dcc[idx].sock);
  }
  hold = dcc[idx].u.other;
  dcc[idx].u.script = calloc(1, sizeof(struct script_info));
  dcc[idx].u.script->u.other = hold;
  dcc[idx].u.script->type = dcc[idx].type;
  dcc[idx].type = &DCC_SCRIPT;
  /* Do not buffer data anymore. All received and stored data is passed
     over to the dcc functions from now on.  */
  sockoptions(dcc[idx].sock, EGG_OPTION_UNSET, SOCK_BUFFER);
  strlcpy(dcc[idx].u.script->command, argv[2], 120);
  return TCL_OK;
}

static int script_valididx(int idx)
{
	if (idx < 0 || idx >= dcc_total || !dcc[idx].type || !(dcc[idx].type->flags & DCT_VALIDIDX)) return(0);
	return(1);
}

static int tcl_killdcc STDVAR
{
  int idx;

  BADARGS(2, 3, " idx ?reason?");
  idx = atoi(argv[1]);
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  /* Don't kill terminal socket */
  if ((dcc[idx].sock == STDOUT) && !backgrd)
    return TCL_OK;
  /* Make sure 'whom' info is updated for other bots */
  if (dcc[idx].type->flags & DCT_CHAT) {
    chanout_but(idx, dcc[idx].u.chat->channel, "*** %s has left the %s%s%s\n",
		dcc[idx].nick, dcc[idx].u.chat ? "channel" : "partyline",
		argc == 3 ? ": " : "", argc == 3 ? argv[2] : "");
    botnet_send_part_idx(idx, argc == 3 ? argv[2] : "");
    if ((dcc[idx].u.chat->channel >= 0) && (dcc[idx].u.chat->channel < GLOBAL_CHANS)) {
      check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock, dcc[idx].u.chat->channel);
    }
    check_tcl_chof(dcc[idx].nick, dcc[idx].sock);
    /* Notice is sent to the party line, the script can add a reason. */
  }
  killsock(dcc[idx].sock);
  lostdcc(idx);
  return TCL_OK;
}

static int script_putbot(char *target, char *text)
{
  int i;

  i = nextbot(target);
  if (i < 0) return(1);
  botnet_send_zapf(i, botnetnick, target, text);
  return(0);
}

static int script_putallbots(char *text)
{
	botnet_send_zapf_broad(-1, botnetnick, NULL, text);
	return(0);
}

static char *script_idx2hand(int idx)
{
	if (idx < 0 || idx >= dcc_total || !(dcc[idx].type) || !(dcc[idx].nick)) return("");
	return(dcc[idx].nick);
}

static int script_islinked(char *bot)
{
	return nextbot(bot);
}

static int script_bots(script_var_t *retval)
{
	char **botlist = NULL;
	int nbots = 0;
	tand_t *bot;

	for (bot = tandbot; bot; bot = bot->next) {
		nbots++;
		botlist = (char **)realloc(botlist, sizeof(char *) * nbots);
		botlist[nbots-1] = strdup(bot->bot);
	}
	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_STRING;
	retval->value = (void *)botlist;
	retval->len = nbots;
	return(0);
}

static int script_botlist(script_var_t *retval)
{
	tand_t *bot;
	script_var_t *sublist, *nick, *uplink, *version, *share;
	char sharestr[2];

	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
	retval->len = 0;
	retval->value = NULL;
	sharestr[1] = 0;
	for (bot = tandbot; bot; bot = bot->next) {
		nick = script_string(bot->bot, -1);
		uplink = script_string((bot->uplink == (tand_t *)1) ? botnetnick : bot->uplink->bot, -1);
		version = script_int(bot->ver);
		sharestr[0] = bot->share;
		share = script_string(strdup(sharestr), -1);

		sublist = script_list(4, nick, uplink, version, share);
		script_list_append(retval, sublist);
	}
	return(0);
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
		othervar = script_string(strdup(other), -1);
		othervar->type |= SCRIPT_FREE;
		timestamp = script_int(dcc[i].timeval);

		sublist = script_list(6, idx, nick, host, type, othervar, timestamp);
		script_list_append(retval, sublist);
	}
	return(0);
}

static void whom_entry(script_var_t *retval, char *nick, char *bot, char *host ,char icon, int idletime, char *away, int chan)
{
	script_var_t *sublist, *vnick, *vbot, *vhost, *vflag, *vidle, *vaway, *vchan;
	char flag[2];

	vnick = script_string(nick, -1);
	vbot = script_string(bot, -1);
	vhost = script_string(host, -1);

	flag[0] = icon;
	flag[1] = 0;
	vflag = script_string(strdup(flag), 1);
	vflag->type |= SCRIPT_FREE;

	vidle = script_int((now - idletime) / 60);
	vaway = script_string(away ? away : "", -1);
	vchan = script_int(chan);

	sublist = script_list(7, vnick, vbot, vhost, vflag, vidle, vaway, vchan);
	script_list_append(retval, sublist);
}

/* list of {nick bot host flag idletime awaymsg channel}
 */
static int script_whom(script_var_t *retval, int nargs, int which_chan)
{
	int i;

	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
	retval->len = 0;
	retval->value = NULL;

	if (nargs == 0) which_chan = -1;

	for (i = 0; i < dcc_total; i++) {
		if (dcc[i].type != &DCC_CHAT) continue;
		if (which_chan != -1 && dcc[i].u.chat->channel != which_chan) continue;
		whom_entry(retval, dcc[i].nick, botnetnick, dcc[i].host,
		geticon(dcc[i].user), dcc[i].timeval, dcc[i].u.chat->away,
		dcc[i].u.chat->channel);
	}
	for (i = 0; i < parties; i++) {
		if (which_chan != -1 && party[i].chan != which_chan) continue;
		whom_entry(retval, party[i].nick, party[i].bot, party[i].from, party[i].flag, party[i].timer, party[i].status & PLSTAT_AWAY ? party[i].away : "", party[i].chan);
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

static int script_link(char *via, char *target)
{
	int x, i;

	if (!via) x = botlink("", -2, target);
	else {
		x = 1;
		i = nextbot(via);
		if (i < 0) x = 0;
		else botnet_send_link(i, botnetnick, via, target);
	}
	return(x);
}

static int script_unlink(char *bot, char *comment)
{
	int i, x;

	i = nextbot(bot);
	if (i < 0) return(0);
	if (!strcasecmp(bot, dcc[i].nick)) x = botunlink(-2, bot, comment);
	else {
		x = 1;
		botnet_send_unlink(i, botnetnick, lastbot(bot), bot, comment);
	}
	return(x);
}

static int tcl_connect STDVAR
{
  int i, z, sock;
  char s[81];

  BADARGS(3, 3, " hostname port");
  if (dcc_total == max_dcc) {
    Tcl_AppendResult(irp, "out of dcc table space", NULL);
    return TCL_ERROR;
  }
  sock = getsock(0);
  if (sock < 0) {
    Tcl_AppendResult(irp, _("No free sockets available."), NULL);
    return TCL_ERROR;
  }
  z = open_telnet_raw(sock, argv[1], atoi(argv[2]));
  if (z < 0) {
    killsock(sock);
    if (z == (-2))
      strlcpy(s, "DNS lookup failed", sizeof s);
    else
      neterror(s);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_ERROR;
  }
  /* Well well well... it worked! */
  i = new_dcc(&DCC_SOCKET, 0);
  dcc[i].sock = sock;
  dcc[i].port = atoi(argv[2]);
  strcpy(dcc[i].nick, "*");
  strlcpy(dcc[i].host, argv[1], UHOSTMAX);
  snprintf(s, sizeof s, "%d", i);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

/* Create a new listening port (or destroy one)
 *
 * listen <port> bots/all/users [mask]
 * listen <port> script <proc> [flag]
 * listen <port> off
 */
static int tcl_listen STDVAR
{
  int i, j, idx = (-1), port, realport;
  char s[11];
  struct portmap *pmap = NULL, *pold = NULL;
  int af = AF_INET /* af_preferred */;

  BADARGS(3, 6, " ?-4/-6? port type ?mask?/?proc ?flag??");
  if (!strcmp(argv[1], "-4") || !strcmp(argv[1], "-6")) {
      if (argv[1][1] == '4')
          af = AF_INET;
      else
	  af = AF_INET6;
      argv[1] = argv[0]; /* UGLY! */
      argv++;
      argc--;
  }
  BADARGS(3, 6, " ?-4/-6? port type ?mask?/?proc ?flag??");

  port = realport = atoi(argv[1]);
  for (pmap = root; pmap; pold = pmap, pmap = pmap->next)
    if (pmap->realport == port) {
      port = pmap->mappedto;
      break;
    }
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_TELNET) && (dcc[i].port == port))
      idx = i;
  if (!strcasecmp(argv[2], "off")) {
    if (pmap) {
      if (pold)
	pold->next = pmap->next;
      else
	root = pmap->next;
      free(pmap);
    }
    /* Remove */
    if (idx < 0) {
      Tcl_AppendResult(irp, "no such listen port is open", NULL);
      return TCL_ERROR;
    }
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return TCL_OK;
  }
  if (idx < 0) {
    /* Make new one */
    if (dcc_total >= max_dcc) {
      Tcl_AppendResult(irp, "no more DCC slots available", NULL);
      return TCL_ERROR;
    }
    /* Try to grab port */
    j = port + 20;
    i = (-1);
    while (port < j && i < 0) {
      i = open_listen(&port, af);
      if (i == -1)
	port++;
      else if (i == -2)
        break;
    }
    if (i == -1) {
      Tcl_AppendResult(irp, "Couldn't grab nearby port", NULL);
      return TCL_ERROR;
    } else if (i == -2) {
      Tcl_AppendResult(irp, "Couldn't assign the requested IP. Please make sure 'my-ip' and/or 'my-ip6' is set properly.", NULL);
      return TCL_ERROR;
    }
    idx = new_dcc(&DCC_TELNET, 0);
    strcpy(dcc[idx].addr, "*"); /* who cares? */
    dcc[idx].port = port;
    dcc[idx].sock = i;
    dcc[idx].timeval = now;
  }
  /* script? */
  if (!strcmp(argv[2], "script")) {
    strcpy(dcc[idx].nick, "(script)");
    if (argc < 4) {
      Tcl_AppendResult(irp, "must give proc name for script listen", NULL);
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return TCL_ERROR;
    }
    if (argc == 5) {
      if (strcmp(argv[4], "pub")) {
	Tcl_AppendResult(irp, "unknown flag: ", argv[4], ". allowed flags: pub",
		         NULL);
	killsock(dcc[idx].sock);
	lostdcc(idx);
	return TCL_ERROR;
      }
      dcc[idx].status = LSTN_PUBLIC;
    }
    strlcpy(dcc[idx].host, argv[3], UHOSTMAX);
    snprintf(s, sizeof s, "%d", port);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  }
  /* bots/users/all */
  if (!strcmp(argv[2], "bots"))
    strcpy(dcc[idx].nick, "(bots)");
  else if (!strcmp(argv[2], "users"))
    strcpy(dcc[idx].nick, "(users)");
  else if (!strcmp(argv[2], "all"))
    strcpy(dcc[idx].nick, "(telnet)");
  if (!dcc[idx].nick[0]) {
    Tcl_AppendResult(irp, "illegal listen type: must be one of ",
		     "bots, users, all, off, script", NULL);
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return TCL_ERROR;
  }
  if (argc == 4) {
    strlcpy(dcc[idx].host, argv[3], UHOSTMAX);
  } else
    strcpy(dcc[idx].host, "*");
  snprintf(s, sizeof s, "%d", port);
  Tcl_AppendResult(irp, s, NULL);
  if (!pmap) {
    pmap = malloc(sizeof(struct portmap));
    pmap->next = root;
    root = pmap;
  }
  pmap->realport = realport;
  pmap->mappedto = port;
  putlog(LOG_MISC, "*", "Listening at telnet port %d (%s)", port, argv[2]);
  return TCL_OK;
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
    else if (remote_boots > 0) {
      i = nextbot(who);
      if (i < 0) return(0);
      botnet_send_reject(i, botnetnick, NULL, whonick, who, reason ? reason : "");
    }
    else return(0);
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

static int tcl_traffic STDVAR
{
  char buf[1024];
  unsigned long out_total_today, out_total;
  unsigned long in_total_today, in_total;

  /* IRC traffic */
  sprintf(buf, "irc %ld %ld %ld %ld", itraffic_irc_today,
	  itraffic_irc + itraffic_irc_today, otraffic_irc_today,
	  otraffic_irc + otraffic_irc_today);
  Tcl_AppendElement(irp, buf);  

  /* Botnet traffic */
  sprintf(buf, "botnet %ld %ld %ld %ld", itraffic_bn_today,
	  itraffic_bn + itraffic_bn_today, otraffic_bn_today,
	  otraffic_bn + otraffic_bn_today);
  Tcl_AppendElement(irp, buf);

  /* Partyline */
  sprintf(buf, "partyline %ld %ld %ld %ld", itraffic_dcc_today,
	  itraffic_dcc + itraffic_dcc_today, otraffic_dcc_today,
	  otraffic_dcc + otraffic_dcc_today);    
  Tcl_AppendElement(irp, buf);

  /* Transfer */
  sprintf(buf, "transfer %ld %ld %ld %ld", itraffic_trans_today,
          itraffic_trans + itraffic_trans_today, otraffic_trans_today,
	  otraffic_trans + otraffic_trans_today);    
  Tcl_AppendElement(irp, buf);

  /* Misc traffic */
  sprintf(buf, "misc %ld %ld %ld %ld", itraffic_unknown_today,
	  itraffic_unknown + itraffic_unknown_today, otraffic_unknown_today,
	  otraffic_unknown + otraffic_unknown_today);    
  Tcl_AppendElement(irp, buf);

  /* Totals */
  in_total_today = itraffic_irc_today + itraffic_bn_today + itraffic_dcc_today
		 + itraffic_trans_today + itraffic_unknown_today,
  in_total = in_total_today + itraffic_irc + itraffic_bn + itraffic_dcc
	   + itraffic_trans + itraffic_unknown;
  out_total_today = otraffic_irc_today + otraffic_bn_today + otraffic_dcc_today
	          + itraffic_trans_today + otraffic_unknown_today,
  out_total = out_total_today + otraffic_irc + otraffic_bn + otraffic_dcc
            + otraffic_trans + otraffic_unknown;	  
  sprintf(buf, "total %ld %ld %ld %ld",
	  in_total_today, in_total, out_total_today, out_total);
  Tcl_AppendElement(irp, buf);
  return(TCL_OK);
}

script_simple_command_t script_dcc_cmds[] = {
	{"", NULL, NULL, NULL, 0},
	{"putdcc", script_putdcc, "is", "idx text", SCRIPT_INTEGER},
	{"putdccraw", script_putdccraw, "iis", "idx len text", SCRIPT_INTEGER},
	{"dccsimul", script_dccsimul, "is", "idx command", SCRIPT_INTEGER},
	{"dccbroadcast", script_dccbroadcast, "s", "text", SCRIPT_INTEGER},
	{"hand2idx", script_hand2idx, "s", "handle", SCRIPT_INTEGER},
	{"getchan", script_getchan, "i", "idx", SCRIPT_INTEGER},
	{"setchan", script_setchan, "ii", "idx chan", SCRIPT_INTEGER},
	{"dccputchan", script_dccputchan, "is", "chan text", SCRIPT_INTEGER},
	{"valididx", script_valididx, "i", "idx", SCRIPT_INTEGER},
	{"putbot", script_putbot, "ss", "bot text", SCRIPT_INTEGER},
	{"putallbots", script_putallbots, "s", "text", SCRIPT_INTEGER},
	{"idx2hand", (Function) script_idx2hand, "i", "idx", SCRIPT_INTEGER},
	{"islinked", script_islinked, "s", "bot", SCRIPT_INTEGER},
	{"dccused", script_dccused, "", "", SCRIPT_INTEGER},
	{"getdccidle", script_getdccidle, "i", "idx", SCRIPT_INTEGER},
	{"getdccaway", (Function) script_getdccaway, "i", "idx", SCRIPT_STRING},
	{"setdccaway", script_setdccaway, "is", "idx msg", SCRIPT_INTEGER},
	{"unlink", script_unlink, "ss", "bot comment", SCRIPT_INTEGER},
	{"boot", script_boot, "ss", "user@bot reason", SCRIPT_INTEGER},
	{"rehash", script_rehash, "", "", SCRIPT_INTEGER},
	{"restart", script_restart, "", "", SCRIPT_INTEGER},
	{0}
};

script_command_t script_full_dcc_cmds[] = {
	{"", "console", script_console, NULL, 1, "is", "idx ?changes?", 0, SCRIPT_PASS_RETVAL|SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{"", "strip", script_strip, NULL, 1, "is", "idx ?change?", 0, SCRIPT_PASS_RETVAL|SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{"", "echo", script_echo, NULL, 1, "ii", "idx ?status?", SCRIPT_INTEGER, SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{"", "page", script_page, NULL, 1, "ii", "idx ?status?", SCRIPT_INTEGER, SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{"", "bots", script_bots, NULL, 0, "", "", 0, SCRIPT_PASS_RETVAL},
	{"", "botlist", script_botlist, NULL, 0, "", "", 0, SCRIPT_PASS_RETVAL},
	{"", "dcclist", script_dcclist, NULL, 0, "s", "?match?", 0, SCRIPT_PASS_RETVAL|SCRIPT_VAR_ARGS},
	{"", "link", script_link, NULL, 1, "ss", "?via-bot? target-bot", 0, SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "whom", script_whom, NULL, 0, "i", "?channel?", 0, SCRIPT_PASS_RETVAL|SCRIPT_PASS_COUNT|SCRIPT_VAR_ARGS},
	{0}
};

tcl_cmds tcldcc_cmds[] =
{
  {"control",		tcl_control},
  {"killdcc",		tcl_killdcc},
  {"connect",		tcl_connect},
  {"listen",		tcl_listen},
  {"traffic",		tcl_traffic},
  {NULL,		NULL}
};
