/*
 * dccutil.c --
 *
 *	lots of little functions to send formatted text to
 *	varying types of connections
 *	'.who', '.whom', and '.dccstat' code
 *	memory management for dcc structures
 *	timeout checking for dcc connections
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
static const char rcsid[] = "$Id: dccutil.c,v 1.52 2002/05/12 06:12:07 stdarg Exp $";
#endif

#include <sys/stat.h>
#include "main.h"
#include <errno.h>
#include "chan.h"
#include "modules.h"
#include "tandem.h"
#include "logfile.h"
#include "misc.h"
#include "cmdt.h"		/* cmd_t				*/
#include "net.h"		/* tputs, killsock			*/
#include "tclhash.h"		/* check_tcl_chon, check_tcl_chjn,
				   check_tcl_chof, check_tcl_away	*/
#include "dccutil.h"		/* prototypes				*/

extern struct dcc_t	*dcc;
extern int		 dcc_total, max_dcc, dcc_flood_thr, backgrd, MAXSOCKS;
extern char		 botnetnick[], spaces[], version[];
extern time_t		 now;
extern sock_list	*socklist;

#ifndef MAKING_MODS
extern struct dcc_table	DCC_CHAT, DCC_LOST;
#endif /* MAKING_MODS   */

char	motdfile[121] = "text/motd";	/* File where the motd is stored */
int	connect_timeout = 15;		/* How long to wait before a telnet
					   connection times out */

int reserved_port_min = 0;
int reserved_port_max = 0;

void init_dcc_max(int newmax)
{
  int osock = MAXSOCKS;

  if (newmax < 1) newmax = 1;
  dcc = realloc(dcc, sizeof(struct dcc_t) * newmax);
  if (newmax > max_dcc) memset(dcc+max_dcc, 0, (newmax-max_dcc) * sizeof(*dcc));

  MAXSOCKS = newmax + 10;
  socklist = (sock_list *)realloc(socklist, sizeof(sock_list) * MAXSOCKS);
  if (newmax > max_dcc) memset(socklist+max_dcc+10, 0, (newmax-max_dcc) * sizeof(*socklist));

  for (; osock < MAXSOCKS; osock++)
    socklist[osock].flags = SOCK_UNUSED;

  max_dcc = newmax;
}


/* Replace \n with \r\n */
char *add_cr(char *buf)
{
  static char WBUF[1024];
  char *p, *q;

  for (p = buf, q = WBUF; *p; p++, q++) {
    if (*p == '\n')
      *q++ = '\r';
    *q = *p;
  }
  *q = *p;
  return WBUF;
}

extern void (*qserver) (int, char *, int);

void dprintf EGG_VARARGS_DEF(int, arg1)
{
  static char buf[1024];
  char *format;
  int idx, len;
  va_list va;

  idx = EGG_VARARGS_START(int, arg1, va);
  format = va_arg(va, char *);
  len = vsnprintf(buf, 1023, format, va);
  va_end(va);
  if (len < 0 || len >= sizeof(buf)) len = sizeof(buf)-1;
  buf[len] = 0;

  if (idx < 0) {
    tputs(-idx, buf, len);
  } else if (idx > 0x7FF0) {
    switch (idx) {
    case DP_LOG:
      putlog(LOG_MISC, "*", "%s", buf);
      break;
    case DP_STDOUT:
      tputs(STDOUT, buf, len);
      break;
    case DP_STDERR:
      tputs(STDERR, buf, len);
      break;
    case DP_SERVER:
    case DP_HELP:
    case DP_MODE:
    case DP_MODE_NEXT:
    case DP_SERVER_NEXT:
    case DP_HELP_NEXT:
      qserver(idx, buf, len);
      break;
    }
    return;
  } else {
    if (len > 500) {		/* Truncate to fit */
      buf[500] = 0;
      strcat(buf, "\n");
      len = 501;
    }
    if (dcc[idx].type && ((long) (dcc[idx].type->output) == 1)) {
      char *p = add_cr(buf);

      tputs(dcc[idx].sock, p, strlen(p));
    } else if (dcc[idx].type && dcc[idx].type->output) {
      dcc[idx].type->output(idx, buf, dcc[idx].u.other);
    } else
      tputs(dcc[idx].sock, buf, len);
  }
}

void chatout EGG_VARARGS_DEF(char *, arg1)
{
  int i, len;
  char *format;
  char s[601];
  va_list va;

  format = EGG_VARARGS_START(char *, arg1, va);
  vsnprintf(s, 511, format, va);
  va_end(va);
  len = strlen(s);
  if (len > 511)
    len = 511;
  s[len + 1] = 0;

  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_CHAT)
      if (dcc[i].u.chat->channel >= 0)
        dprintf(i, "%s", s);

}

/* Print to all on this channel but one.
 */
void chanout_but EGG_VARARGS_DEF(int, arg1)
{
  int i, x, chan, len;
  char *format;
  char s[601];
  va_list va;

  x = EGG_VARARGS_START(int, arg1, va);
  chan = va_arg(va, int);
  format = va_arg(va, char *);
  vsnprintf(s, 511, format, va);
  va_end(va);
  len = strlen(s);
  if (len > 511)
    len = 511;
  s[len + 1] = 0;

  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_CHAT) && (i != x))
      if (dcc[i].u.chat->channel == chan)
        dprintf(i, "%s", s);

}

void dcc_chatter(int idx)
{
  int i, j;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  get_user_flagrec(dcc[idx].user, &fr, NULL);
  show_motd(idx);
  i = dcc[idx].u.chat->channel;
  dcc[idx].u.chat->channel = 234567;
  j = dcc[idx].sock;
  strcpy(dcc[idx].u.chat->con_chan, "***");
  check_tcl_chon(dcc[idx].nick, dcc[idx].sock);
  /* Still there? */
  if ((idx >= dcc_total) || (dcc[idx].sock != j))
    return;			/* Nope */
  /* Tcl script may have taken control */
  if (dcc[idx].type == &DCC_CHAT) {
    if (!strcmp(dcc[idx].u.chat->con_chan, "***"))
      strcpy(dcc[idx].u.chat->con_chan, "*");
    if (dcc[idx].u.chat->channel == 234567) {
      /* If the chat channel has already been altered it's *highly*
       * probably join/part messages have been broadcast everywhere,
       * so dont bother sending them
       */
      if (i == -2)
	i = 0;
      dcc[idx].u.chat->channel = i;
      if (dcc[idx].u.chat->channel >= 0) {
	if (dcc[idx].u.chat->channel < 100000) {
	  botnet_send_join_idx(idx, -1);
	}
      }
      check_tcl_chjn(botnetnick, dcc[idx].nick, dcc[idx].u.chat->channel,
		     geticon(dcc[idx].user), dcc[idx].sock, dcc[idx].host);
    }
    /* But *do* bother with sending it locally */
    if (!dcc[idx].u.chat->channel) {
      chanout_but(-1, 0, "*** %s joined the party line.\n", dcc[idx].nick);
    } else if (dcc[idx].u.chat->channel > 0) {
      chanout_but(-1, dcc[idx].u.chat->channel,
		  "*** %s joined the channel.\n", dcc[idx].nick);
    }
  }
}

/* Mark an entry as lost and deconstruct it's contents. It will be securely
 * removed from the dcc list in the main loop.
 */
void lostdcc(int n)
{
  /* Make sure it's a valid dcc index. */
  if (n < 0 || n >= max_dcc) return;

  if (dcc[n].type && dcc[n].type->kill)
    dcc[n].type->kill(n, dcc[n].u.other);
  else if (dcc[n].u.other)
    free(dcc[n].u.other);
  memset(&dcc[n], 0, sizeof(struct dcc_t));

  dcc[n].sock = (-1);
  dcc[n].type = &DCC_LOST;
}

/* Remove entry from dcc list. Think twice before using this function,
 * because it invalidates any variables that point to a specific dcc
 * entry!
 *
 * Note: The entry will be deconstructed if it was not deconstructed
 *       already. This case should normally not occur.
 */
static void removedcc(int n)
{
  if (dcc[n].type && dcc[n].type->kill)
    dcc[n].type->kill(n, dcc[n].u.other);
  else if (dcc[n].u.other)
    free(dcc[n].u.other);

  /* if we are removing the last entry let's decrease dcc_total */
  if(n + 1 == dcc_total) dcc_total--;

  memset(&dcc[n], 0, sizeof(struct dcc_t)); /* drummer */
}

/* Clean up sockets that were just left for dead.
 */
void dcc_remove_lost(void)
{
  int i;

  
  for (i = dcc_total; i > 0; --i) {
    if (dcc[i].type == &DCC_LOST) {
      dcc[i].type = NULL;
      removedcc(i);
    }
  }
}

/* Show list of current dcc's to a dcc-chatter
 * positive value: idx given -- negative value: sock given
 */
void tell_dcc(int zidx)
{
  int i, j, k;
  char other[160];

  spaces[HANDLEN - 9] = 0;
  dprintf(zidx, "SOCK PORT  NICK     %s HOST                       TYPE\n"
	  ,spaces);
  dprintf(zidx, "---- ----- ---------%s -------------------------- ----\n"
	  ,spaces);
  spaces[HANDLEN - 9] = ' ';
  /* Show server */
  for (i = 0; i < dcc_total; i++) {
    if (!dcc[i].type) continue;
    j = strlen(dcc[i].host);
    if (j > 26)
      j -= 26;
    else
      j = 0;
    if (dcc[i].type && dcc[i].type->display)
      dcc[i].type->display(i, other);
    else {
      sprintf(other, "?:%lX  !! ERROR !!", (long) dcc[i].type);
      break;
    }
    k = HANDLEN - strlen(dcc[i].nick);
    spaces[k] = 0;
    dprintf(zidx, "%-4d %5d %s%s %-26s %s\n", i,
	    dcc[i].port, dcc[i].nick, spaces, dcc[i].host + j, other);
    spaces[k] = ' ';
  }
}

/* Mark someone on dcc chat as no longer away
 */
void not_away(int idx)
{
  if (dcc[idx].u.chat->away == NULL) {
    dprintf(idx, "You weren't away!\n");
    return;
  }
  if (dcc[idx].u.chat->channel >= 0) {
    chanout_but(-1, dcc[idx].u.chat->channel,
		"*** %s is no longer away.\n", dcc[idx].nick);
    if (dcc[idx].u.chat->channel < 100000) {
      botnet_send_away(-1, botnetnick, dcc[idx].sock, NULL, idx);
    }
  }
  dprintf(idx, "You're not away any more.\n");
  free_null(dcc[idx].u.chat->away);
  check_tcl_away(botnetnick, dcc[idx].sock, NULL);
}

void set_away(int idx, char *s)
{
  if (s == NULL) {
    not_away(idx);
    return;
  }
  if (!s[0]) {
    not_away(idx);
    return;
  }
  if (dcc[idx].u.chat->away != NULL)
    free(dcc[idx].u.chat->away);
  dcc[idx].u.chat->away = strdup(s);
  if (dcc[idx].u.chat->channel >= 0) {
    chanout_but(-1, dcc[idx].u.chat->channel,
		"*** %s is now away: %s\n", dcc[idx].nick, s);
    if (dcc[idx].u.chat->channel < 100000) {
      botnet_send_away(-1, botnetnick, dcc[idx].sock, s, idx);
    }
  }
  dprintf(idx, "You are now away.\n");
  check_tcl_away(botnetnick, dcc[idx].sock, s);
}

/* Make a password, 10-15 random letters and digits
 */
void makepass(char *s)
{
  int i;

  i = 10 + (random() % 6);
  make_rand_str(s, i);
}

void flush_lines(int idx, struct chat_info *ci)
{
  int c = ci->line_count;
  struct msgq *p = ci->buffer, *o;

  while (p && c < (ci->max_line)) {
    ci->current_lines--;
    tputs(dcc[idx].sock, p->msg, p->len);
    free(p->msg);
    o = p->next;
    free(p);
    p = o;
    c++;
  }
  if (p != NULL) {
    if (dcc[idx].status & STAT_TELNET)
      tputs(dcc[idx].sock, "[More]: ", 8);
    else
      tputs(dcc[idx].sock, "[More]\n", 7);
  }
  ci->buffer = p;
  ci->line_count = 0;
}

int new_dcc(struct dcc_table *type, int xtra_size)
{
  int i = dcc_total;

  for (i = 0; i < dcc_total; i++)
    if (!dcc[i].type) break;

  if (i == max_dcc)
    return -1;

  if (i == dcc_total)
    dcc_total++;
  memset(&dcc[i], 0, sizeof(*dcc));

  dcc[i].type = type;
  if (xtra_size)
    dcc[i].u.other = calloc(1, xtra_size);
  return i;
}

/* Changes the given dcc entry to another type.
 */
void changeover_dcc(int i, struct dcc_table *type, int xtra_size)
{
  /* Free old structure. */
  if (dcc[i].type && dcc[i].type->kill)
    dcc[i].type->kill(i, dcc[i].u.other);
  else if (dcc[i].u.other)
    free_null(dcc[i].u.other);

  dcc[i].type = type;
  if (xtra_size)
    dcc[i].u.other = calloc(1, xtra_size);
}

int detect_dcc_flood(time_t * timer, struct chat_info *chat, int idx)
{
  time_t t;

  if (!dcc_flood_thr)
    return 0;
  t = now;
  if (*timer != t) {
    *timer = t;
    chat->msgs_per_sec = 0;
  } else {
    chat->msgs_per_sec++;
    if (chat->msgs_per_sec > dcc_flood_thr) {
      /* FLOOD */
      dprintf(idx, "*** FLOOD: %s.\n", _("Goodbye"));
      /* Evil assumption here that flags&DCT_CHAT implies chat type */
      if ((dcc[idx].type->flags & DCT_CHAT) && chat &&
	  (chat->channel >= 0)) {
	char x[1024];

	snprintf(x, sizeof x, _("%s has been forcibly removed for flooding.\n"), dcc[idx].nick);
	chanout_but(idx, chat->channel, "*** %s", x);
	if (chat->channel < 100000)
	  botnet_send_part_idx(idx, x);
      }
      check_tcl_chof(dcc[idx].nick, dcc[idx].sock);
      if ((dcc[idx].sock != STDOUT) || backgrd) {
	killsock(dcc[idx].sock);
	lostdcc(idx);
      } else {
	dprintf(DP_STDOUT, "\n### SIMULATION RESET ###\n\n");
	dcc_chatter(idx);
      }
      return 1;			/* <- flood */
    }
  }
  return 0;
}

/* Handle someone being booted from dcc chat.
 */
void do_boot(int idx, char *by, char *reason)
{
  int files = (dcc[idx].type != &DCC_CHAT);

  dprintf(idx, _("-=- poof -=-\n"));
  dprintf(idx, _("Youve been booted from the %s by %s%s%s\n"), files ? "file section" : "bot",
          by, reason[0] ? ": " : ".", reason);
  /* If it's a partyliner (chatterer :) */
  /* Horrible assumption that DCT_CHAT using structure uses same format
   * as DCC_CHAT */
  if ((dcc[idx].type->flags & DCT_CHAT) &&
      (dcc[idx].u.chat->channel >= 0)) {
    char x[1024];

    snprintf(x, sizeof x, _("%s booted %s from the party line%s%s\n"), by, dcc[idx].nick,
		 reason[0] ? ": " : "", reason);
    chanout_but(idx, dcc[idx].u.chat->channel, "*** %s.\n", x);
    if (dcc[idx].u.chat->channel < 100000)
      botnet_send_part_idx(idx, x);
  }
  check_tcl_chof(dcc[idx].nick, dcc[idx].sock);
  if ((dcc[idx].sock != STDOUT) || backgrd) {
    killsock(dcc[idx].sock);
    lostdcc(idx);
    /* Entry must remain in the table so it can be logged by the caller */
  } else {
    dprintf(DP_STDOUT, "\n### SIMULATION RESET\n\n");
    dcc_chatter(idx);
  }
  return;
}
