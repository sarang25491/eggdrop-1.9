/*
 * tclchan.c --
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
static const char rcsid[] = "$Id: tclchan.c,v 1.22 2002/11/24 04:50:33 wcc Exp $";
#endif
*/

/* flagmaps.c defines flag name to flag value structures. */
#include "flagmaps.c"

/* Names of channel info members. */
char *channel_info_names[] = {
	"chanmode",
	"idle-kick",
	"stop-net-hack",
	"revenge-mode",
	"flood-pub",
	"flood-ctcp",
	"flood-join",
	"flood-kick",
	"flood-deop",
	"flood-nick",
	"aop-delay",
	NULL
};

static int lookup_flag_by_name(channel_flag_map_t *map, char *name, int *flagval);

/* Kill a ban/exempt/invite either globally or from a channel. */
static int script_killsomething(void *type, char *chan_name, char *mask)
{
	struct chanset_t *chan = NULL;

	if (chan_name && chan_name[0]) {
		chan = findchan_by_dname(chan_name);
		if (!chan) return(-1);
	}

	if (u_delmask((int) type, chan, mask, 1) > 0) {
		if (!chan) {
			for (chan = chanset; chan; chan = chan->next) {
				add_mode(chan, '-', (int) type, mask);
			}
		}
		else add_mode(chan, '-', (int) type, mask);
	}
	return(0);
}

/* Stick a ban/exempt/invite globally or to a chanel. */
static int script_sticksomething(void *type, char *chan_name, char *mask)
{
	struct chanset_t *chan = NULL;

	if (chan_name && chan_name[0]) {
		chan = findchan_by_dname(chan_name);
		if (!chan) return(-1);
	}

	/* If type > 255, then we're unsticking. The mask type is type & 255. */
	return u_setsticky_mask((int)type & 255, chan, mask, ((int)type > 255) ? 0 : 1);
}

static int get_maskrec_and_alt(char type, char *chan_name, struct maskrec **u, struct maskrec **alt)
{
	struct chanset_t *chan = NULL;

	if (chan_name && chan_name[0]) {
		chan = findchan_by_dname(chan_name);
		if (!chan) return(1);
	}

	*u = NULL;
	*alt = NULL;
	if (type == 'b') {
		if (chan) {
			*u = chan->bans;
			if (channel_honor_global_bans(chan)) *alt = global_bans;
		}
		else *u = global_bans;
	}
	else if (type == 'I') {
		if (chan) {
			*u = chan->invites;
			if (channel_honor_global_invites(chan)) *alt = global_invites;
		}
		else *u = global_invites;
	}
	else {
		if (chan) {
			*u = chan->exempts;
			if (channel_honor_global_exempts(chan)) *alt = global_exempts;
		}
		else *u = global_exempts;
	}
	return(0);
}

static int script_issomething(void *type, char *chan_name, char *mask)
{
	maskrec *u, *alt;
	int matches;

	get_maskrec_and_alt((int) type, chan_name, &u, &alt);

	matches = u_equals_mask(u, mask);
	if (!matches && alt) matches = u_equals_mask(alt, mask);
	return(matches ? 1 : 0);
}

static int script_isstickysomething(void *type, char *chan_name, char *mask)
{
	maskrec *u, *alt;
	int sticky;

	if (get_maskrec_and_alt((int) type, chan_name, &u, &alt)) return(-1);

	sticky = u_sticky_mask(u, mask);
	if (!sticky && alt) sticky = u_sticky_mask(alt, mask);
	return(sticky ? 1 : 0);
}

static int script_ispermsomething(void *type, char *chan_name, char *mask)
{
	maskrec *u, *alt;

	if (get_maskrec_and_alt((int) type, chan_name, &u, &alt)) return(-1);

	if (u_equals_mask(u, mask) == 2) return(1);
	if (alt && (u_equals_mask(alt, mask) == 2)) return(1);
	return(0);
}

static int script_matchsomething(void *type, char *chan_name, char *mask)
{
	maskrec *u, *alt;

	if (get_maskrec_and_alt((int) type, chan_name, &u, &alt)) return(-1);

	if (u_match_mask(u, mask)) return(1);
	if (alt && u_match_mask(alt, mask)) return(1);
	return(0);
}

static int script_newsomething(void *type, char *chan_name, char *mask, char *creator, char *comment, char *lifetime, char *options)
{
	time_t expire_time;
	struct chanset_t *chan = NULL;
	int sticky = 0;
	int r;
	module_entry *me;

	if (chan_name[0]) {
		chan = findchan_by_dname(chan_name);
		if (!chan) return(-1);
	}

	if (lifetime) {
		expire_time = atoi(lifetime);
		if (expire_time) expire_time = expire_time * 60 + now;
	}
	expire_time = now + (60 * (((int) type == 'b') ?
			    ((chan->ban_time == 0) ? 0L : chan->ban_time) :
			    (((int) type == 'e') ? ((chan->exempt_time == 0) ?
			    0L : chan->exempt_time) :
			    ((chan->invite_time == 0) ?
			    0L : chan->invite_time))));

	if (options && !strcasecmp(options, "sticky")) sticky = 1;

	r = u_addmask((int) type, chan, mask, creator, comment, expire_time, sticky);
	if (chan && !r) return(-1);

	if ((int) type == 'b') {
		me = module_find("irc", 0, 0);
		if (me) {
			if (chan) (me->funcs[IRC_CHECK_THIS_BAN])(chan, mask, sticky);
			else for (chan = chanset; chan; chan = chan->next) {
				(me->funcs[IRC_CHECK_THIS_BAN])(chan, mask, sticky);
			}
		}
		return(0);
	}

	if (chan) add_mode(chan, '+', (int) type, mask);
	else {
		for (chan = chanset; chan; chan = chan->next) {
			add_mode(chan, '+', (int) type, mask);
		}
	}
	return(0);
}

static int script_channel_info(script_var_t *retval)
{
	int i;
	channel_flag_map_t *flag_map;
	struct udef_struct *ul;

	retval->type = SCRIPT_ARRAY | SCRIPT_VAR | SCRIPT_FREE;
	retval->len = 0;

	for (i = 0; channel_info_names[i]; i++) {
		script_list_append(retval, script_string(channel_info_names[i], -1));
	}

	for (flag_map = normal_flag_map; flag_map->name; flag_map++) {
		script_list_append(retval, script_string(flag_map->name, -1));
	}

	for (flag_map = stupid_ircnet_flag_map; flag_map->name; flag_map++) {
		script_list_append(retval, script_string(flag_map->name, -1));
	}

	for (ul = udef; ul; ul = ul->next) {
		/* If it's undefined, skip it. */
		if (!ul->defined || !ul->name) continue;
		script_list_append(retval, script_string(ul->name, -1));
	}

	return(0);
}

static int script_channel_get(script_var_t *retval, char *channel_name, char *setting)
{
	char s[121];
	struct chanset_t *chan;
	struct udef_struct *ul;
	int flagval;

	retval->type = SCRIPT_STRING;
	retval->len = -1;

	chan = findchan_by_dname(channel_name);
	if (!chan) return(-1);

#define CHECK(x) !strcmp(setting, x)
	if (CHECK("chanmode")) get_mode_protect(chan, s);
	else if (CHECK("idle-kick")) sprintf(s, "%d", chan->idle_kick);
	else if (CHECK("stop-net-hack")) sprintf(s, "%d", chan->stopnethack_mode);
	else if (CHECK("revenge-mode")) sprintf(s, "%d", chan->revenge_mode);
	else if (CHECK("flood-pub")) sprintf(s, "%d %d", chan->flood_pub_thr, chan->flood_pub_time);
	else if (CHECK("flood-ctcp")) sprintf(s, "%d %d", chan->flood_ctcp_thr, chan->flood_ctcp_time);
	else if (CHECK("flood-join")) sprintf(s, "%d %d", chan->flood_join_thr, chan->flood_join_time);
	else if (CHECK("flood-kick")) sprintf(s, "%d %d", chan->flood_kick_thr, chan->flood_kick_time);
	else if (CHECK("flood-deop")) sprintf(s, "%d %d", chan->flood_deop_thr, chan->flood_deop_time);
	else if (CHECK("flood-nick")) sprintf(s, "%d %d", chan->flood_nick_thr, chan->flood_nick_time);
	else if (CHECK("aop-delay")) sprintf(s, "%d %d", chan->aop_min, chan->aop_max);
	else if (CHECK("ban-time"))  sprintf(s, "%d", chan->ban_time);
	else if (CHECK("exempt-time"))  sprintf(s, "%d", chan->exempt_time);
	else if (CHECK("invite-time"))  sprintf(s, "%d", chan->invite_time);

	else if (lookup_flag_by_name(normal_flag_map, setting, &flagval)) sprintf(s, "%d", chan->status & flagval);
	else if (lookup_flag_by_name(stupid_ircnet_flag_map, setting, &flagval)) sprintf(s, "%d", chan->ircnet_status & flagval);
	else {
		/* Hopefully it's a user-defined flag. */
		for (ul = udef; ul && ul->name; ul = ul->next) {
			if (!strcmp(setting, ul->name)) break;
		}
		if (!ul || !ul->name) {
			/* Error if it wasn't found. */
			return(-1);
		}
		if (ul->type == UDEF_STR) {
			char *value;

			/* If it's unset then give them an empty string. */
			value = (char *)getudef(ul->values, chan->dname);
			if (!value) value = "";

			retval->value = value;
			return(0);
		}
		else {
			/* Flag or int, all the same. */
			sprintf(s, "%d", getudef(ul->values, chan->dname));
		}
	}
	/* Ok, if we make it this far, the result is "s". */
	retval->value = strdup(s);
	retval->type |= SCRIPT_FREE;
	return(0);
}

static int tcl_channel STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 999, " command ?options?");
  if (!strcmp(argv[1], "add")) {
    BADARGS(3, 4, " add channel-name ?options-list?");
    if (argc == 3)
      return tcl_channel_add(irp, argv[2], "");
    return tcl_channel_add(irp, argv[2], argv[3]);
  }
  if (!strcmp(argv[1], "set")) {
    BADARGS(3, 999, " set channel-name ?options?");
    chan = findchan_by_dname(argv[2]);
    if (chan == NULL) {
      if (chan_hack == 1)
	return TCL_OK;		/* Ignore channel settings for a static
				 * channel which has been removed from
				 * the config */
      Tcl_AppendResult(irp, "no such channel record", NULL);
      return TCL_ERROR;
    }
    return tcl_channel_modify(irp, chan, argc - 3, &argv[3]);
  }
  if (!strcmp(argv[1], "remove")) {
    BADARGS(3, 3, " remove channel-name");
    chan = findchan_by_dname(argv[2]);
    if (chan == NULL) {
      Tcl_AppendResult(irp, "no such channel record", NULL);
      return TCL_ERROR;
    }
    remove_channel(chan);
    return TCL_OK;
  }
  Tcl_AppendResult(irp, "unknown channel command: should be one of: ",
		   "add, set, info, remove", NULL);
  return TCL_ERROR;
}

static int lookup_flag_by_name(channel_flag_map_t *map, char *name, int *flagval)
{
	channel_flag_map_t *flagmap;

	for (flagmap = map; flagmap->name; flagmap++) {
		if (!strcmp(flagmap->name, name)) {
			*flagval = flagmap->flagval;
			return(1);
		}
	}
	return(0);
}

/* Parse options for a channel.
 */
static int tcl_channel_modify(Tcl_Interp * irp, struct chanset_t *chan,
			      int items, char **item)
{
  int i, x = 0, found,
      old_status = chan->status,
      old_mode_mns_prot = chan->mode_mns_prot,
      old_mode_pls_prot = chan->mode_pls_prot;
  struct udef_struct *ul = udef;
  char s[121];
  module_entry *me;

  for (i = 0; i < items; i++) {
    if (!strcmp(item[i], "chanmode")) {
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, "channel chanmode needs argument", NULL);
	return TCL_ERROR;
      }
      strncpy(s, item[i], 120);
      s[120] = 0;
      set_mode_protect(chan, s);
    } else if (!strcmp(item[i], "idle-kick")) {
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, "channel idle-kick needs argument", NULL);
	return TCL_ERROR;
      }
      chan->idle_kick = atoi(item[i]);
    } else if (!strcmp(item[i], "dont-idle-kick"))
      chan->idle_kick = 0;
    else if (!strcmp(item[i], "stopnethack-mode")) {
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, "channel stopnethack-mode needs argument", NULL);
	return TCL_ERROR;
      }
      chan->stopnethack_mode = atoi(item[i]);
    } else if (!strcmp(item[i], "revenge-mode")) {
      i++;
      if (i >= items) {
        if (irp)
          Tcl_AppendResult(irp, "channel revenge-mode needs argument", NULL);
        return TCL_ERROR;
      }
      chan->revenge_mode = atoi(item[i]);
    } else if (!strcmp(item[i], "ban-time")) {
      i++;
      if (i >= items) {
        if (irp)
          Tcl_AppendResult(irp, "channel ban-time needs argument", NULL);
        return TCL_ERROR;
      }
      chan->ban_time = atoi(item[i]);
    } else if (!strcmp(item[i], "exempt-time")) {
      i++;
      if (i >= items) {
        if (irp)
          Tcl_AppendResult(irp, "channel exempt-time needs argument", NULL);
        return TCL_ERROR;
      }
      chan->exempt_time = atoi(item[i]);
    } else if (!strcmp(item[i], "invite-time")) {
      i++;
      if (i >= items) {
        if (irp)
          Tcl_AppendResult(irp, "channel invite-time needs argument", NULL);
        return TCL_ERROR;
      }
      chan->invite_time = atoi(item[i]);
    }
    else if (item[i][0] == '+' || item[i][0] == '-') {
      int flagval;

      if (lookup_flag_by_name(normal_flag_map, item[i]+1, &flagval)) {
        if (item[i][0] == '-') chan->status &= ~flagval;
        else chan->status |= flagval;
      }
      else if (lookup_flag_by_name(stupid_ircnet_flag_map, item[i]+1, &flagval)) {
        if (item[i][0] == '-') chan->ircnet_status &= ~flagval;
        else chan->ircnet_status |= flagval;
      }
      else {
        /* Hopefully it's a user-defined flag! */
        goto check_for_udef_flags;
      }
    }
    else if (!strncmp(item[i], "flood-", 6)) {
      int *pthr = 0, *ptime;
      char *p;

      if (!strcmp(item[i] + 6, "chan")) {
	pthr = &chan->flood_pub_thr;
	ptime = &chan->flood_pub_time;
      } else if (!strcmp(item[i] + 6, "join")) {
	pthr = &chan->flood_join_thr;
	ptime = &chan->flood_join_time;
      } else if (!strcmp(item[i] + 6, "ctcp")) {
	pthr = &chan->flood_ctcp_thr;
	ptime = &chan->flood_ctcp_time;
      } else if (!strcmp(item[i] + 6, "kick")) {
	pthr = &chan->flood_kick_thr;
	ptime = &chan->flood_kick_time;
      } else if (!strcmp(item[i] + 6, "deop")) {
	pthr = &chan->flood_deop_thr;
	ptime = &chan->flood_deop_time;
      } else if (!strcmp(item[i] + 6, "nick")) {
	pthr = &chan->flood_nick_thr;
	ptime = &chan->flood_nick_time;
      } else {
	if (irp)
	  Tcl_AppendResult(irp, "illegal channel flood type: ", item[i], NULL);
	return TCL_ERROR;
      }
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, item[i - 1], " needs argument", NULL);
	return TCL_ERROR;
      }
      p = strchr(item[i], ':');
      if (p) {
	*p++ = 0;
	*pthr = atoi(item[i]);
	*ptime = atoi(p);
	*--p = ':';
      } else {
	*pthr = atoi(item[i]);
	*ptime = 1;
      }
    } else if (!strncmp(item[i], "aop-delay", 9)) {
      char *p;
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, item[i - 1], " needs argument", NULL);
	return TCL_ERROR;
      }
      p = strchr(item[i], ':');
      if (p) {
	p++;
	chan->aop_min = atoi(item[i]);
	chan->aop_max = atoi(p);
      } else {
	chan->aop_min = atoi(item[i]);
	chan->aop_max = chan->aop_min;
      }
    } else {
      if (!strncmp(item[i] + 1, "udef-flag-", 10))
        initudef(UDEF_FLAG, item[i] + 11, 0);
      else if (!strncmp(item[i], "udef-int-", 9))
        initudef(UDEF_INT, item[i] + 9, 0);
	else if (!strncmp(item[i], "udef-str-", 9))
		initudef(UDEF_STR, item[i] + 9, 0);
check_for_udef_flags:
      found = 0;
      for (ul = udef; ul; ul = ul->next) {
        if (ul->type == UDEF_FLAG &&
	     /* Direct match when set during .chanset ... */
	    (!strcasecmp(item[i] + 1, ul->name) ||
	     /* ... or with prefix when set during chanfile load. */
	     (!strncmp(item[i] + 1, "udef-flag-", 10) &&
	      !strcasecmp(item[i] + 11, ul->name)))) {
          if (item[i][0] == '+')
            setudef(ul, chan->dname, 1);
          else
            setudef(ul, chan->dname, 0);
          found = 1;
	  break;
        } else if (ul->type == UDEF_INT &&
		    /* Direct match when set during .chanset ... */
		   (!strcasecmp(item[i], ul->name) ||
		    /* ... or with prefix when set during chanfile load. */
		    (!strncmp(item[i], "udef-int-", 9) &&
		     !strcasecmp(item[i] + 9, ul->name)))) {
          i++;
          if (i >= items) {
            if (irp)
              Tcl_AppendResult(irp, "this setting needs an argument", NULL);
            return TCL_ERROR;
          }
          setudef(ul, chan->dname, atoi(item[i]));
          found = 1;
	  break;
        }
	else if (ul->type == UDEF_STR && (!strcasecmp(item[i], ul->name) || (!strncmp(item[i], "udef-str-", 9) && !strcasecmp(item[i] + 9, ul->name)))) {
		char *val;
		i++;
		if (i >= items) {
			if (irp) Tcl_AppendResult(irp, "this setting needs an aargument", NULL);
			return TCL_ERROR;
		}
		val = (char *)getudef(ul->values, chan->dname);
		if (val) free(val);
		/* Get extra room for new braces, etc */
		val = malloc(3 * strlen(item[i]) + 10);
		convert_element(item[i], val);
		val = realloc(val, strlen(val)+1);
		setudef(ul, chan->dname, (int)val);
		found = 1;
		break;
	}
      }
      if (!found) {
        if (irp && item[i][0]) /* ignore "" */
      	  Tcl_AppendResult(irp, "illegal channel option: ", item[i], NULL);
      	x++;
      }
    }
  }
  /* If protect_readonly == 0 and chan_hack == 0 then
   * bot is now processing the configfile, so dont do anything,
   * we've to wait the channelfile that maybe override these settings
   * (note: it may cause problems if there is no chanfile!)
   * <drummer/1999/10/21>
   */
  if (protect_readonly || chan_hack) {
    if (((old_status ^ chan->status) & CHAN_INACTIVE) &&
	module_find("irc", 0, 0)) {
      if (channel_inactive(chan) &&
	  (chan->status & (CHAN_ACTIVE | CHAN_PEND)))
	dprintf(DP_SERVER, "PART %s\n", chan->name);
      if (!channel_inactive(chan) &&
	  !(chan->status & (CHAN_ACTIVE | CHAN_PEND)))
	dprintf(DP_SERVER, "JOIN %s %s\n", (chan->name[0]) ?
					   chan->name : chan->dname,
					   chan->channel.key[0] ?
					   chan->channel.key : chan->key_prot);
    }
    if ((old_status ^ chan->status) &
	(CHAN_ENFORCEBANS | CHAN_OPONJOIN | CHAN_BITCH | CHAN_AUTOVOICE)) {
      if ((me = module_find("irc", 0, 0)))
	(me->funcs[IRC_RECHECK_CHANNEL])(chan, 1);
    } else if (old_mode_pls_prot != chan->mode_pls_prot ||
	       old_mode_mns_prot != chan->mode_mns_prot)
      if ((me = module_find("irc", 1, 2)))
	(me->funcs[IRC_RECHECK_CHANNEL_MODES])(chan);
  }
  if (x > 0)
    return TCL_ERROR;
  return TCL_OK;
}

static int script_channels(script_var_t *retval)
{
	struct chanset_t *chan;

	retval->type = SCRIPT_ARRAY | SCRIPT_VAR | SCRIPT_FREE;
	retval->len = 0;

	for (chan = chanset; chan; chan = chan->next) {
		script_list_append(retval, script_string(chan->dname, -1));
	}
	return(0);
}

static int script_listmask(void *type, script_var_t *retval, char *channel_name)
{
	script_var_t *mlist;
	struct chanset_t *chan = NULL;
	struct maskrec *m, *masks;

	retval->type = SCRIPT_ARRAY | SCRIPT_VAR | SCRIPT_FREE;
	retval->len = 0;

	if (channel_name) {
		chan = findchan_by_dname(channel_name);
		if (!chan) return(-1);
	}
	if ((int) type == 'b') masks = (chan ? chan->bans : global_bans);
	else if ((int) type == 'I') masks = (chan ? chan->invites : global_invites);
	else if ((int) type == 'e') masks = (chan ? chan->exempts : global_exempts);
	else return(-1);

	for (m = masks; m; m = m->next) {
		mlist = script_list(6, script_string(m->mask, -1),
			script_string(m->desc, -1),
			script_int(m->expire),
			script_int(m->added),
			script_int(m->lastactive),
			script_string(m->user, -1)
		);
		script_list_append(retval, mlist);
	}
	return(0);
}

/* Should be removed when the new config system is in place. */
static int tcl_savechannels STDVAR
{
  BADARGS(1, 1, "");
  if (!chanfile[0]) {
    Tcl_AppendResult(irp, "no channel file");
    return TCL_ERROR;
  }
  write_channels();
  return TCL_OK;
}

/* Should be removed when the new config system is in place. */
static int tcl_loadchannels STDVAR
{
  BADARGS(1, 1, "");
  if (!chanfile[0]) {
    Tcl_AppendResult(irp, "no channel file");
    return TCL_ERROR;
  }
  setstatic = 0;
  read_channels(1);
  return TCL_OK;
}

static int script_channel_valid(char *channel_name)
{
	return findchan_by_dname(channel_name) ? 1 : 0;
}

/* Should be removed when the new config system is in place. */
static int tcl_isdynamic STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan_by_dname(argv[1]);
  if (chan != NULL)
    if (!channel_static(chan)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static char *script_channel_getinfo(struct userrec *u, char *channel_name)
{
	struct chanuserrec *ch;

	ch = get_chanrec(u, channel_name);
	if (ch && ch->info) return ch->info;
	return "";
}

static int script_channel_setinfo(struct userrec *u, char *channel_name, char *info)
{
	if (!findchan_by_dname(channel_name)) return(-1);
	set_handle_chaninfo(userlist, u->handle, channel_name, info);
	return(0);
}

static int script_setlaston(struct userrec *u, int when, char *chan)
{
	if (!when) when = now;

	if (!chan || !findchan_by_dname(chan)) chan = "*";
	set_handle_laston(chan, u, when);
	return(0);
}

static int script_chanrec_add(struct userrec *u, char *chan)
{
	if (!findchan_by_dname(chan)) return(-1);
	if (get_chanrec(u, chan) != NULL) return(-1);
	add_chanrec(u, chan);
	return(0);
}

static int script_chanrec_del(struct userrec *u, char *chan)
{
	if (get_chanrec(u, chan) != NULL) return(-1);
	del_chanrec(u, chan);
	return(0);
}

static int script_chanrec_exists(struct userrec *u, char *chan)
{
	if (!findchan_by_dname(chan)) return(0);
	if (get_chanrec(u, chan) != NULL) return(1);
	return(0);
}

static void init_masklist(masklist *m)
{
  m->mask = calloc(1, 1);
  m->who = NULL;
  m->next = NULL;
}

/* Initialize out the channel record.
 */
static void init_channel(struct chanset_t *chan, int reset)
{
  chan->channel.maxmembers = 0;
  chan->channel.mode = 0;
  chan->channel.members = 0;
  if (!reset)
    chan->channel.key = calloc(1, 1);

  chan->channel.ban = (masklist *) malloc(sizeof(masklist));
  init_masklist(chan->channel.ban);

  chan->channel.exempt = (masklist *) malloc(sizeof(masklist));
  init_masklist(chan->channel.exempt);

  chan->channel.invite = (masklist *) malloc(sizeof(masklist));
  init_masklist(chan->channel.invite);

  chan->channel.member = (memberlist *) malloc(sizeof(memberlist));
  chan->channel.member->nick[0] = 0;
  chan->channel.member->next = NULL;
  chan->channel.topic = NULL;
}

static void clear_masklist(masklist *m)
{
  masklist *temp;

  for (; m; m = temp) {
    temp = m->next;
    if (m->mask)
      free(m->mask);
    if (m->who)
      free(m->who);
    free(m);
  }
}

/* Clear out channel data from memory.
 */
static void clear_channel(struct chanset_t *chan, int reset)
{
  memberlist *m, *m1;

  if (chan->channel.topic)
    free(chan->channel.topic);
  for (m = chan->channel.member; m; m = m1) {
    m1 = m->next;
    free(m);
  }

  clear_masklist(chan->channel.ban);
  chan->channel.ban = NULL;
  clear_masklist(chan->channel.exempt);
  chan->channel.exempt = NULL;
  clear_masklist(chan->channel.invite);
  chan->channel.invite = NULL;

  if (reset)
    init_channel(chan, 1);
}

/* Create new channel and parse commands.
 */
static int tcl_channel_add(Tcl_Interp *irp, char *newname, char *options)
{
  struct chanset_t *chan;
  int items;
  int ret = TCL_OK;
  int join = 0;
  char **item;
  char buf[2048], buf2[256];

  if (!newname || !newname[0] || !strchr(CHANMETA, newname[0])) {
    if (irp)
      Tcl_AppendResult(irp, "invalid channel prefix", NULL);
    return TCL_ERROR;
  }

  if (strchr(newname, ',') != NULL) {
    if (irp)
      Tcl_AppendResult(irp, "invalid channel name", NULL);
     return TCL_ERROR;
   }

  convert_element(glob_chanmode, buf2);
  simple_sprintf(buf, "chanmode %s ", buf2);
  strncat(buf, glob_chanset, 2047 - strlen(buf));
  strncat(buf, options, 2047 - strlen(buf));
  buf[2047] = 0;

  if (Tcl_SplitList(NULL, buf, &items, &item) != TCL_OK)
    return TCL_ERROR;
  if ((chan = findchan_by_dname(newname))) {
    /* Already existing channel, maybe a reload of the channel file */
    chan->status &= ~CHAN_FLAGGED;	/* don't delete me! :) */
  } else {
    chan = calloc(1, sizeof(struct chanset_t));
    chan->limit_prot = 0;
    chan->limit = 0;

    /* Anyone know why all this crap isn't in a struct so we can memcpy it? */
    chan->flood_pub_thr = gfld_chan_thr;
    chan->flood_pub_time = gfld_chan_time;
    chan->flood_ctcp_thr = gfld_ctcp_thr;
    chan->flood_ctcp_time = gfld_ctcp_time;
    chan->flood_join_thr = gfld_join_thr;
    chan->flood_join_time = gfld_join_time;
    chan->flood_deop_thr = gfld_deop_thr;
    chan->flood_deop_time = gfld_deop_time;
    chan->flood_kick_thr = gfld_kick_thr;
    chan->flood_kick_time = gfld_kick_time;
    chan->flood_nick_thr = gfld_nick_thr;
    chan->flood_nick_time = gfld_nick_time;
    chan->stopnethack_mode = global_stopnethack_mode;
    chan->revenge_mode = global_revenge_mode;
    chan->ban_time = global_ban_time;
    chan->exempt_time = global_exempt_time;
    chan->invite_time = global_invite_time;
    chan->idle_kick = global_idle_kick;
    chan->aop_min = global_aop_min;
    chan->aop_max = global_aop_max;

    /* We _only_ put the dname (display name) in here so as not to confuse
     * any code later on. chan->name gets updated with the channel name as
     * the server knows it, when we join the channel. <cybah>
     */
    strncpy(chan->dname, newname, 81);
    chan->dname[80] = 0;

    /* Initialize chan->channel info */
    init_channel(chan, 0);
    list_append((struct list_type **) &chanset, (struct list_type *) chan);
    /* Channel name is stored in xtra field for sharebot stuff */
    join = 1;
  }
  if (setstatic)
    chan->status |= CHAN_STATIC;
  /* If chan_hack is set, we're loading the userfile. Ignore errors while
   * reading userfile and just return TCL_OK. This is for compatability
   * if a user goes back to an eggdrop that no-longer supports certain
   * (channel) options.
   */
  if ((tcl_channel_modify(irp, chan, items, item) != TCL_OK) && !chan_hack) {
    ret = TCL_ERROR;
  }
  Tcl_Free((char *) item);
  if (join && !channel_inactive(chan) && module_find("irc", 0, 0))
    dprintf(DP_SERVER, "JOIN %s %s\n", chan->dname, chan->key_prot);
  return ret;
}

static int tcl_setudef STDVAR
{
  int type;

  BADARGS(3, 3, " type name");
  if (!strcasecmp(argv[1], "flag"))
    type = UDEF_FLAG;
  else if (!strcasecmp(argv[1], "int"))
    type = UDEF_INT;
	/*## ADD CODE FOR STRING SETTINGS*/
	else if (!strcasecmp(argv[1], "str"))
		type = UDEF_STR;
  else {
    Tcl_AppendResult(irp, "invalid type. Must be one of: flag, int, str", NULL);
    return TCL_ERROR;
  }
  initudef(type, argv[2], 1);
  return TCL_OK;
}

static int tcl_renudef STDVAR
{
  struct udef_struct *ul;
  int type, found = 0;

  BADARGS(4, 4, " type oldname newname");
  if (!strcasecmp(argv[1], "flag"))
    type = UDEF_FLAG;
  else if (!strcasecmp(argv[1], "int"))
    type = UDEF_INT;
	/*## ADD CODE FOR STRING SETTINGS*/
	else if (!strcasecmp(argv[1], "str"))
		type = UDEF_STR;
  else {
    Tcl_AppendResult(irp, "invalid type. Must be one of: flag, int, str", NULL);
    return TCL_ERROR;
  }
  for (ul = udef; ul; ul = ul->next) {
    if (ul->type == type && !strcasecmp(ul->name, argv[2])) {
      free(ul->name);
      ul->name = strdup(argv[3]);
      found = 1;
    }
  }
  if (!found) {
    Tcl_AppendResult(irp, "not found", NULL);
    return TCL_ERROR;
  } else
    return TCL_OK;
}

static int tcl_deludef STDVAR
{
  struct udef_struct *ul, *ull;
  int type, found = 0;

  BADARGS(3, 3, " type name");
  if (!strcasecmp(argv[1], "flag"))
    type = UDEF_FLAG;
  else if (!strcasecmp(argv[1], "int"))
    type = UDEF_INT;
	/*## ADD CODE FOR STRING SETTINGS*/
	else if (!strcasecmp(argv[1], "str"))
		type = UDEF_STR;
  else {
    Tcl_AppendResult(irp, "invalid type. Must be one of: flag, int, str", NULL);
    return TCL_ERROR;
  }
  for (ul = udef; ul; ul = ul->next) {
    ull = ul->next;
    if (!ull)
      break;
    if (ull->type == type && !strcasecmp(ull->name, argv[2])) {
      ul->next = ull->next;
      free(ull->name);
      free_udef_chans(ull->values, ull->type);
      free(ull);
      found = 1;
    }
  }
  if (udef) {
    if (udef->type == type && !strcasecmp(udef->name, argv[2])) {
      ul = udef->next;
      free(udef->name);
      free_udef_chans(udef->values, udef->type);
      free(udef);
      udef = ul;
      found = 1;
    }
  }
  if (!found) {
    Tcl_AppendResult(irp, "not found", NULL);
    return TCL_ERROR;
  } else
    return TCL_OK;
}

static script_command_t channel_script_cmds[] = {
	{"", "newban", script_newsomething, (void *)'b', 1, "ssssss", "channel mask creator comment ?lifetime? ?sticky?", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS},
	{"", "newinvite", script_newsomething, (void *)'I', 1, "ssssss", "channel mask creator comment ?lifetime? ?sticky?", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS},
	{"", "newexempt", script_newsomething, (void *)'e', 1, "ssssss", "channel mask creator comment ?lifetime? ?sticky?", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS},

	{"", "killban", script_killsomething, (void *)'b', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "killinvite", script_killsomething, (void *)'I', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "killexempt", script_killsomething, (void *)'e', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},

	{"", "matchban", script_matchsomething, (void *)'b', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "matchinvite", script_matchsomething, (void *)'I', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "matchexempt", script_matchsomething, (void *)'e', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},

	{"", "isban", script_issomething, (void *)'b', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "isinvite", script_issomething, (void *)'I', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "isexempt", script_issomething, (void *)'e', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},

	{"", "ispermban", script_ispermsomething, (void *)'b', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "isperminvite", script_ispermsomething, (void *)'I', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "ispermexempt", script_ispermsomething, (void *)'e', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},


	{"", "isbansticky", script_isstickysomething, (void *)'b', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "isinvitesticky", script_isstickysomething, (void *)'I', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "isexemptsticky", script_isstickysomething, (void *)'e', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},

	{"", "stickban", script_sticksomething, (void *)'b', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "stickinvite", script_sticksomething, (void *)'I', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "stickexempt", script_sticksomething, (void *)'e', 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},

	{"", "unstickban", script_sticksomething, (void *)('b'+256), 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "unstickinvite", script_sticksomething, (void *)('I'+256), 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "unstickexempt", script_sticksomething, (void *)('e'+256), 1, "ss", "?channel? mask", SCRIPT_INTEGER, SCRIPT_PASS_CDATA | SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},

	{"", "listbans", script_listmask, (void *)'b', 0, "s", "?channel?", 0, SCRIPT_VAR_ARGS | SCRIPT_PASS_RETVAL | SCRIPT_PASS_CDATA},
	{"", "listinvites", script_listmask, (void *)'I', 0, "s", "?channel?", 0, SCRIPT_VAR_ARGS | SCRIPT_PASS_RETVAL | SCRIPT_PASS_CDATA},
	{"", "listexempts", script_listmask, (void *)'e', 0, "s", "?channel?", 0, SCRIPT_VAR_ARGS | SCRIPT_PASS_RETVAL | SCRIPT_PASS_CDATA},

	{"", "channel_get", script_channel_get, NULL, 2, "ss", "channel setting", 0, SCRIPT_PASS_RETVAL},
	{"", "channel_info", script_channel_info, NULL, 0, "", "", 0, SCRIPT_PASS_RETVAL},
	{"", "channel_valid", script_channel_valid, NULL, 1, "s", "channel", SCRIPT_INTEGER, 0},
	{"", "channel_setinfo", script_channel_setinfo, NULL, 2, "Uss", "handle channel ?info?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "channel_getinfo", script_channel_getinfo, NULL, 2, "Us", "handle channel", SCRIPT_STRING, 0},
	{"", "channels", script_channels, NULL, 0, "", "", 0, SCRIPT_PASS_RETVAL},

	{"", "chanrec_add", script_chanrec_add, NULL, 2, "Us", "handle channel", SCRIPT_INTEGER, 0},
	{"", "chanrec_del", script_chanrec_del, NULL, 2, "Us", "handle channel", SCRIPT_INTEGER, 0},
	{"", "chanrec_exists", script_chanrec_exists, NULL, 2, "Us", "handle channel", SCRIPT_INTEGER, 0},

	{"", "setlaston", script_setlaston, NULL, 2, "Uis", "handle when ?channel?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},

	{0}
};

static tcl_cmds channels_cmds[] =
{
  {"channel",		tcl_channel},
  {"savechannels",	tcl_savechannels},
  {"loadchannels",	tcl_loadchannels},
  {"isdynamic",		tcl_isdynamic},
  {"setudef",		tcl_setudef},
  {"renudef",		tcl_renudef},
  {"deludef",		tcl_deludef},
  {NULL,		NULL}
};
