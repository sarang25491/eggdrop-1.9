/*
 * flags.c --
 *
 *	all the flag matching/conversion functions in one neat package :)
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
static const char rcsid[] = "$Id: flags.c,v 1.37 2003/02/03 10:43:36 wcc Exp $";
#endif

#include <ctype.h>
#include "main.h"
#include "logfile.h"
#include "chan.h"		/* findchan_by_dname, channel_shared	*/
#include "users.h"		/* USERENTRY_BOTFL, get_user		*/
#include "dccutil.h"		/* shareout, dprintf_eggdrop		*/
#include "userent.h"		/* list_type_kill			*/
#include "irccmp.h"		/* irccmp				*/
#include "flags.h"		/* prototypes				*/

extern int raw_log, noshare;

typedef struct {
	int flag;
	char c;
	char *type;
} logmode_mapping_t;

static logmode_mapping_t logmode_mappings[] = {
	/*
	{LOG_MSGS, 'm', "msgs"},
	{LOG_PUBLIC, 'p', "public"},
	{LOG_JOIN, 'j', "joins"},
	{LOG_MODES, 'k', "kicks/modes"},
	{LOG_CMDS, 'c', "cmds"},
	{LOG_MISC, 'o', "misc"},
	{LOG_BOTS, 'b', "bots"},
	{LOG_RAW, 'r', "raw"},
	{LOG_WALL, 'w', "wallops"},
	{LOG_FILES, 'x', "files"},
	{LOG_SERV, 's', "server"},
	{LOG_DEBUG, 'd', "debug"},
	{LOG_SRVOUT, 'v', "server output"},
	{LOG_BOTNET, 't', "botnet traffic"},
	{LOG_BOTSHARE, 'h', "share traffic"},
	{LOG_LEV1, '1', "level 1"},
	{LOG_LEV2, '2', "level 2"},
	{LOG_LEV3, '3', "level 3"},
	{LOG_LEV4, '4', "level 4"},
	{LOG_LEV5, '5', "level 5"},
	{LOG_LEV6, '6', "level 6"},
	{LOG_LEV7, '7', "level 7"},
	{LOG_LEV8, '8', "level 8"},
	*/
	{0}
};

#define NEEDS_RAW_LOG (LOG_RAW|LOG_SRVOUT|LOG_BOTNET|LOG_BOTSHARE)

int logmodes(char *s)
{
	logmode_mapping_t *mapping;
	int modes = 0;

	return(0x7fffffff);
	/*
	while (*s) {
		if (*s == '*') return(LOG_ALL);
		for (mapping = logmode_mappings; mapping->type; mapping++) {
			if (mapping->c == tolower(*s)) break;
		}
		if (mapping->type) modes |= mapping->flag;
		s++;
	}
	return(modes);
	*/
}

char *masktype(int x)
{
	static char s[30] = "abcdefghijklmnopqrstuvwxyz";
	return(s);
	/*
	char *p = s;
	logmode_mapping_t *mapping;

	for (mapping = logmode_mappings; mapping->type; mapping++) {
		if (x & mapping->flag) {
			if ((mapping->flag & NEEDS_RAW_LOG) && !raw_log) continue;
			*p++ = mapping->c;
		}
	}
	if (p == s) *p++ = '-';
	*p = 0;
	return(s);
	*/
}

char *maskname(int x)
{
	static char s[207];	/* Change this if you change the levels */
	s[0] = 0;
	return(s);
	/*
	logmode_mapping_t *mapping;
	int len;

	*s = 0;
	for (mapping = logmode_mappings; mapping->type; mapping++) {
		if (x & mapping->flag) {
			if ((mapping->flag & NEEDS_RAW_LOG) && !raw_log) continue;
			strcat(s, mapping->type);
			strcat(s, ", ");
		}
	}
	len = strlen(s);
	if (len) s[len-2] = 0;
	else strcpy(s, "none");
	return(s);
	*/
}

/* Some flags are mutually exclusive -- this roots them out
 */
int sanity_check(int atr)
{
  if ((atr & USER_BOT) &&
      (atr & (USER_PARTY | USER_MASTER | USER_OWNER)))
    atr &= ~(USER_PARTY | USER_MASTER | USER_OWNER);
  /* Can't be owner without also being master */
  if (atr & USER_OWNER)
    atr |= USER_MASTER;
  /* Master implies botmaster, op and janitor */
  if (atr & USER_MASTER)
    atr |= USER_BOTMAST | USER_OP | USER_JANITOR;
  /* Can't be botnet master without party-line access */
  if (atr & USER_BOTMAST)
    atr |= USER_PARTY;
  /* Janitors can use the file area */
  if (atr & USER_JANITOR)
    atr |= USER_XFER;
  return atr;
}

/* Sanity check on channel attributes
 */
int chan_sanity_check(int chatr, int atr)
{
  /* Can't be channel owner without also being channel master. */
  if (chatr & USER_OWNER)
    chatr |= USER_MASTER;
  /* Master implies op */
  if (chatr & USER_MASTER)
    chatr |= USER_OP ;
  /* Can't be +s on chan unless you're a bot */
  if (!(atr & USER_BOT))
    chatr &= ~BOT_SHARE;
  return chatr;
}

/* Get icon symbol for a user (depending on access level)
 *
 * (*)owner on any channel
 * (+)master on any channel
 * (%) botnet master
 * (@) op on any channel
 * (-) other
 */
char geticon(struct userrec *user)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  if (!user)
    return '-';
  get_user_flagrec(user, &fr, 0);
  if (chan_owner(fr))
    return '*';
  if (chan_master(fr))
    return '+';
  if (glob_botmast(fr))
    return '%';
  if (chan_op(fr))
    return '@';
  return '-';
}

void break_down_flags(const char *string, struct flag_record *plus,
		      struct flag_record *minus)
{
  struct flag_record	*which = plus;
  int			 mode = 0;	/* 0 = glob, 1 = chan, 2 = bot */
  int			 flags = plus->match;

  if (!(flags & FR_GLOBAL)) {
    if (flags & FR_BOT)
      mode = 2;
    else if (flags & FR_CHAN)
      mode = 1;
    else
      return;			/* We dont actually want any..huh? */
  }
  memset(plus, 0, sizeof(struct flag_record));

  if (minus)
    memset(minus, 0, sizeof(struct flag_record));

  plus->match = FR_OR;		/* Default binding type OR */
  while (*string) {
    switch (*string) {
    case '+':
      which = plus;
      break;
    case '-':
      which = minus ? minus : plus;
      break;
    case '|':
    case '&':
      if (!mode) {
	if (*string == '|')
	  plus->match = FR_OR;
	else
	  plus->match = FR_AND;
      }
      which = plus;
      mode++;
      if ((mode == 2) && !(flags & (FR_CHAN | FR_BOT)))
	string = "";
      else if (mode == 3)
	mode = 1;
      break;
    default:
      if ((*string >= 'a') && (*string <= 'z')) {
	switch (mode) {
	case 0:
	  which->global |=1 << (*string - 'a');

	  break;
	case 1:
	  which->chan |= 1 << (*string - 'a');
	  break;
	case 2:
	  which->bot |= 1 << (*string - 'a');
	}
      } else if ((*string >= 'A') && (*string <= 'Z')) {
	switch (mode) {
	case 0:
	  which->udef_global |= 1 << (*string - 'A');
	  break;
	case 1:
	  which->udef_chan |= 1 << (*string - 'A');
	  break;
	}
      } else if ((*string >= '0') && (*string <= '9')) {
	switch (mode) {
	  /* Map 0->9 to A->K for glob/chan so they are not lost */
	case 0:
	  which->udef_global |= 1 << (*string - '0');
	  break;
	case 1:
	  which->udef_chan |= 1 << (*string - '0');
	  break;
	case 2:
	  which->bot |= BOT_FLAG0 << (*string - '0');
	  break;
	}
      }
    }
    string++;
  }
  for (which = plus; which; which = (which == plus ? minus : 0)) {
    which->global &=USER_VALID;

    which->udef_global &= 0x03ffffff;
    which->chan &= CHAN_VALID;
    which->udef_chan &= 0x03ffffff;
    which->bot &= BOT_VALID;
  }
  plus->match |= flags;
  if (minus) {
    minus->match |= flags;
    if (!(plus->match & (FR_AND | FR_OR)))
      plus->match |= FR_OR;
  }
}

static int flag2str(char *string, int bot, int udef)
{
  char x = 'a', *old = string;

  while (bot && (x <= 'z')) {
    if (bot & 1)
      *string++ = x;
    x++;
    bot = bot >> 1;
  }
  x = 'A';
  while (udef && (x <= 'Z')) {
    if (udef & 1)
      *string++ = x;
    udef = udef >> 1;
    x++;
  }
  if (string == old)
    *string++ = '-';
  return string - old;
}

static int bot2str(char *string, int bot)
{
  char x = 'a', *old = string;

  while (x < 'v') {
    if (bot & 1)
      *string++ = x;
    x++;
    bot >>= 1;
  }
  x = '0';
  while (x <= '9') {
    if (bot & 1)
      *string++ = x;
    x++;
    bot >>= 1;
  }
  return string - old;
}

int build_flags(char *string, struct flag_record *plus,
		struct flag_record *minus)
{
  char *old = string;

  if (plus->match & FR_GLOBAL) {
    if (minus && (plus->global || plus->udef_global))
      *string++ = '+';
    string += flag2str(string, plus->global, plus->udef_global);

    if (minus && (minus->global || minus->udef_global)) {
      *string++ = '-';
      string += flag2str(string, minus->global, minus->udef_global);
    }
  } else if (plus->match & FR_BOT) {
    if (minus && plus->bot)
      *string++ = '+';
    string += bot2str(string, plus->bot);
    if (minus && minus->bot) {
      *string++ = '-';
      string += bot2str(string, minus->bot);
    }
  }
  if (plus->match & FR_CHAN) {
    if (plus->match & (FR_GLOBAL | FR_BOT))
      *string++ = (plus->match & FR_AND) ? '&' : '|';
    if (minus && (plus->chan || plus->udef_chan))
      *string++ = '+';
    string += flag2str(string, plus->chan, plus->udef_chan);
    if (minus && (minus->chan || minus->udef_chan)) {
      *string++ = '-';
      string += flag2str(string, minus->global, minus->udef_chan);
    }
  }
  if ((plus->match & (FR_BOT | FR_CHAN)) == (FR_BOT | FR_CHAN)) {
    *string++ = (plus->match & FR_AND) ? '&' : '|';
    if (minus && plus->bot)
      *string++ = '+';
    string += bot2str(string, plus->bot);
    if (minus && minus->bot) {
      *string++ = '-';
      string += bot2str(string, minus->bot);
    }
  }
  if (string == old) {
    *string++ = '-';
    *string = 0;
    return 0;
  }
  *string = 0;
  return string - old;
}

/* Returns 1 if flags match, 0 if they don't. */
int flagrec_ok(struct flag_record *req,
	       struct flag_record *have)
{
  if (req->match & FR_AND) {
    return flagrec_eq(req, have);
  } else if (req->match & FR_OR) {
    int hav = have->global;

    /* Exception 1 - global +d/+k cant use -|-, unless they are +p */
    if (!req->chan && !req->global && !req->udef_global &&
	!req->udef_chan) {
      if (glob_party(*have))
        return 1;
      return 1;
    }
    if (hav & req->global)
      return 1;
    if (have->chan & req->chan)
      return 1;
    if (have->udef_global & req->udef_global)
      return 1;
    if (have->udef_chan & req->udef_chan)
      return 1;
    return 0;
  }
  return 0;			/* fr0k3 binding, dont pass it */
}

/* Returns 1 if flags match, 0 if they don't. */
int flagrec_eq(struct flag_record *req, struct flag_record *have)
{
  if (req->match & FR_AND) {
    if (req->match & FR_GLOBAL) {
      if ((req->global &have->global) !=req->global)
	return 0;
      if ((req->udef_global & have->udef_global) != req->udef_global)
	return 0;
    }
    if (req->match & FR_BOT)
      if ((req->bot & have->bot) != req->bot)
	return 0;
    if (req->match & FR_CHAN) {
      if ((req->chan & have->chan) != req->chan)
	return 0;
      if ((req->udef_chan & have->udef_chan) != req->udef_chan)
	return 0;
    }
    return 1;
  } else if (req->match & FR_OR) {
    if (!req->chan && !req->global && !req->udef_chan &&
	!req->udef_global && !req->bot)
      return 1;
    if (req->match & FR_GLOBAL) {
      if (have->global &req->global)
	return 1;
      if (have->udef_global & req->udef_global)
	return 1;
    }
    if (req->match & FR_BOT)
      if (have->bot & req->bot)
	return 1;
    if (req->match & FR_CHAN) {
      if (have->chan & req->chan)
	return 1;
      if (have->udef_chan & req->udef_chan)
	return 1;
    }
    return 0;
  }
  return 0;			/* fr0k3 binding, dont pass it */
}

void set_user_flagrec(struct userrec *u, struct flag_record *fr,
		      const char *chname)
{
  struct chanuserrec *cr = NULL;
  int oldflags = fr->match;
  char buffer[100];
  struct chanset_t *ch;

  if (!u)
    return;
  if (oldflags & FR_GLOBAL) {
    u->flags = fr->global;

    u->flags_udef = fr->udef_global;
    if (!noshare && !(u->flags & USER_UNSHARED)) {
      fr->match = FR_GLOBAL;
      build_flags(buffer, fr, NULL);
      shareout(NULL, "a %s %s\n", u->handle, buffer);
    }
  }
  if ((oldflags & FR_BOT) && (u->flags & USER_BOT))
    set_user(&USERENTRY_BOTFL, u, (void *) fr->bot);
  /* Don't share bot attrs */
  if ((oldflags & FR_CHAN) && chname) {
    for (cr = u->chanrec; cr; cr = cr->next)
      if (!irccmp(chname, cr->channel))
	break;
    ch = findchan_by_dname(chname);
    if (!cr && ch) {
      cr = calloc(1, sizeof(struct chanuserrec));
      cr->next = u->chanrec;
      u->chanrec = cr;
      strlcpy(cr->channel, chname, sizeof cr->channel);
    }
    if (cr && ch) {
      cr->flags = fr->chan;
      cr->flags_udef = fr->udef_chan;
      if (!noshare && !(u->flags & USER_UNSHARED) && channel_shared(ch)) {
	fr->match = FR_CHAN;
	build_flags(buffer, fr, NULL);
	shareout(ch, "a %s %s %s\n", u->handle, buffer, chname);
      }
    }
  }
  fr->match = oldflags;
}

/* Always pass the dname (display name) to this function for chname <cybah>
 */
void get_user_flagrec(struct userrec *u, struct flag_record *fr,
		      const char *chname)
{
  struct chanuserrec *cr = NULL;

  if (!u) {
    fr->global = fr->udef_global = fr->chan = fr->udef_chan = fr->bot = 0;
    return;
  }
  if (fr->match & FR_GLOBAL) {
    fr->global = u->flags;
    fr->udef_global = u->flags_udef;
  } else {
    fr->global = 0;
    fr->udef_global = 0;
  }
  if (fr->match & FR_BOT) {
    fr->bot = (long) get_user(&USERENTRY_BOTFL, u);
  } else
    fr->bot = 0;
  if (fr->match & FR_CHAN) {
    if (fr->match & FR_ANYWH) {
      fr->chan = u->flags;
      fr->udef_chan = u->flags_udef;
      for (cr = u->chanrec; cr; cr = cr->next)
	if (findchan_by_dname(cr->channel)) {
	  fr->chan |= cr->flags;
	  fr->udef_chan |= cr->flags_udef;
	}
    } else {
      if (chname)
	for (cr = u->chanrec; cr; cr = cr->next)
	  if (!irccmp(chname, cr->channel))
	    break;
      if (cr) {
	fr->chan = cr->flags;
	fr->udef_chan = cr->flags_udef;
      } else {
	fr->chan = 0;
	fr->udef_chan = 0;
      }
    }
  }
}

static int botfl_unpack(struct userrec *u, struct user_entry *e)
{
  struct flag_record fr = {FR_BOT, 0, 0, 0, 0, 0};

  break_down_flags(e->u.list->extra, &fr, NULL);
  list_type_kill(e->u.list);
  e->u.ulong = fr.bot;
  return 1;
}

static int botfl_pack(struct userrec *u, struct user_entry *e)
{
  char x[100];
  struct flag_record fr = {FR_BOT, 0, 0, 0, 0, 0};

  fr.bot = e->u.ulong;
  e->u.list = malloc(sizeof(struct list_type));
  e->u.list->next = NULL;
  e->u.list->extra = malloc(build_flags(x, &fr, NULL) + 1);
  strcpy(e->u.list->extra, x);
  return 1;
}

static int botfl_kill(struct user_entry *e)
{
  free(e);
  return 1;
}

static int botfl_write_userfile(FILE *f, struct userrec *u,
				struct user_entry *e)
{
  char x[100];
  struct flag_record fr = {FR_BOT, 0, 0, 0, 0, 0};

  fr.bot = e->u.ulong;
  build_flags(x, &fr, NULL);
  if (fprintf(f, "--%s %s\n", e->type->name, x) == EOF)
    return 0;
  return 1;
}

static int botfl_set(struct userrec *u, struct user_entry *e, void *buf)
{
  register long atr = ((long) buf & BOT_VALID);

  if (!(u->flags & USER_BOT))
    return 1;			/* Don't even bother trying to set the
				   flags for a non-bot */

  if ((atr & BOT_HUB) && (atr & BOT_ALT))
    atr &= ~BOT_ALT;
  if (atr & BOT_REJECT) {
    if (atr & BOT_SHARE)
      atr &= ~(BOT_SHARE | BOT_REJECT);
    if (atr & BOT_HUB)
      atr &= ~(BOT_HUB | BOT_REJECT);
    if (atr & BOT_ALT)
      atr &= ~(BOT_ALT | BOT_REJECT);
  }
  if (!(atr & BOT_SHARE))
    atr &= ~BOT_GLOBAL;
  e->u.ulong = atr;
  return 1;
}

static int botfl_tcl_get(Tcl_Interp *interp, struct userrec *u,
			 struct user_entry *e, int argc, char **argv)
{
  char x[100];
  struct flag_record fr = {FR_BOT, 0, 0, 0, 0, 0};

  fr.bot = e->u.ulong;
  build_flags(x, &fr, NULL);
  Tcl_AppendResult(interp, x, NULL);
  return TCL_OK;
}

static int botfl_tcl_set(Tcl_Interp *irp, struct userrec *u,
			 struct user_entry *e, int argc, char **argv)
{
  struct flag_record fr = {FR_BOT, 0, 0, 0, 0, 0};

  BADARGS(4, 4, " handle BOTFL flags");
  if (u->flags & USER_BOT) {
    /* Silently ignore for users */
    break_down_flags(argv[3], &fr, NULL);
    botfl_set(u, e, (void *) fr.bot);
  }
  return TCL_OK;
}

static void botfl_display(int idx, struct user_entry *e)
{
  struct flag_record fr = {FR_BOT, 0, 0, 0, 0, 0};
  char x[100];

  fr.bot = e->u.ulong;
  build_flags(x, &fr, NULL);
  dprintf(idx, "  BOT FLAGS: %s\n", x);
}

struct user_entry_type USERENTRY_BOTFL =
{
  0,				/* always 0 ;) */
  0,
  def_dupuser,
  botfl_unpack,
  botfl_pack,
  botfl_write_userfile,
  botfl_kill,
  def_get,
  botfl_set,
  botfl_tcl_get,
  botfl_tcl_set,
  botfl_display,
  "BOTFL"
};
