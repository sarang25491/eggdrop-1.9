/*
 * channels.c
 *
 * 1. Basic channel services (auto-join, re-join)
 * 2. Script services for channels (channel_set/get)
 *
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Eggheads Development Team
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

#define MODULE_NAME "channels"

#define start channels_LTX_start

static eggdrop_t *egg = NULL;

static int setstatic, use_info, chan_hack, global_aop_min, global_aop_max,
           global_ban_time, global_exempt_time, global_invite_time;

/*
 * Note:
 *  - We write chanmode "" too, so that the bot won't use default-chanmode
 *    instead of ""
 */
static void write_channels()
{
  FILE *f;
  char s[121], w[1024], w2[1024], name[163];
  struct chanset_t *chan;
  struct udef_struct *ul;

  if (!chanfile[0])
    return;
  sprintf(s, "%s~new", chanfile);
  f = fopen(s, "w");
  chmod(s, userfile_perm);
  if (f == NULL) {
    putlog(LOG_MISC, "*", "ERROR writing channel file.");
    return;
  }
  fprintf(f, "#Dynamic Channel File for %s (%s) -- written %s\n", myname, ver,
          ctime(&now));
  for (chan = chanset; chan; chan = chan->next) {
    convert_element(chan->dname, name);
    get_mode_protect(chan, w);
    convert_element(w, w2);
    fprintf(f,
            "channel %s %s%schanmode %s aop_delay %d:%d ban_time %d "
            "exempt_time %d invite_time %d %cenforcebans %cdynamicbans "
            "%cautoop %cgreet %cdontkickops %cstatuslog %cautovoice %csecret "
            "%ccycle %cinactive %cdynamicexempts %chonor-global-bans "
            "%chonor-global-exempts %chonor-global-invites %cdynamicinvites "
            "%cnodesynch ", channel_static(chan) ? "set" : "add", name,
            channel_static(chan) ? " " : " { ", w2, chan->aop_min,
            chan->aop_max, chan->ban_time, chan->exempt_time,
            chan->invite_time,
            PLSMNS(channel_enforcebans(chan)),
            PLSMNS(channel_dynamicbans(chan)),
            PLSMNS(channel_autoop(chan)),
            PLSMNS(channel_greet(chan)),
            PLSMNS(channel_dontkickops(chan)),
            PLSMNS(channel_logstatus(chan)),
            PLSMNS(channel_autovoice(chan)),
            PLSMNS(channel_secret(chan)),
            PLSMNS(channel_cycle(chan)),
            PLSMNS(channel_inactive(chan)),
            PLSMNS(channel_dynamicexempts(chan)),
            PLSMNS(channel_honor_global_bans(chan)),
            PLSMNS(channel_honor_global_exempts(chan)),
            PLSMNS(channel_honor_global_invites(chan)),
            PLSMNS(channel_dynamicinvites(chan)),
            PLSMNS(channel_nodesynch(chan)));
    for (ul = udef; ul; ul = ul->next) {
      if (ul->defined && ul->name) {
	if (ul->type == UDEF_FLAG)
	  fprintf(f, "%c%s%s ", getudef(ul->values, chan->dname) ? '+' : '-',
		  "udef_flag_", ul->name);
	else if (ul->type == UDEF_INT)
	  fprintf(f, "%s%s %d ", "udef_int_", ul->name, getudef(ul->values,
		  chan->dname));
	else if (ul->type == UDEF_STR) {
		char *p;
		p = (char *)getudef(ul->values, chan->dname);
		if (!p) strcpy(p, "{}");
		fprintf(f, "udef_str_%s %s ", ul->name, p);
	}
	else
          putlog(LOG_DEBUG, "*", "UDEF-ERROR: unknown type %d", ul->type);
      }
    }
    fprintf(f, "%s\n", channel_static(chan) ? "" : "}");
    if (fflush(f)) {
      putlog(LOG_MISC, "*", "ERROR writing channel file.");
      fclose(f);
      return;
    }
  }
  fclose(f);
  unlink(chanfile);
  movefile(s, chanfile);
}

static void channels_report(int idx, int details)
{
  struct chanset_t *chan;
  int i;
  char s[1024], s2[100];
  struct flag_record fr = {FR_CHAN | FR_GLOBAL, 0, 0, 0, 0, 0};

  for (chan = chanset; chan; chan = chan->next) {
    if (idx != DP_STDOUT)
      get_user_flagrec(dcc[idx].user, &fr, chan->dname);
    if ((idx == DP_STDOUT) || glob_master(fr) || chan_master(fr)) {
      s[0] = 0;
      if (channel_greet(chan))
	strcat(s, "greet, ");
      if (channel_autoop(chan))
	strcat(s, "auto-op, ");
      if (s[0])
	s[strlen(s) - 2] = 0;
      if (!s[0])
	strcpy(s, _("lurking"));
      get_mode_protect(chan, s2);
      if (!channel_inactive(chan)) {
	if (channel_active(chan)) {
	  /* If it's a !chan, we want to display it's unique name too <cybah> */
	  if (chan->dname[0]=='!') {
	    dprintf(idx, "    %-10s: %2d member%s enforcing \"%s\" (%s), "
	            "unique name %s\n", chan->dname, chan->channel.members,
	            (chan->channel.members==1) ? "," : "s,", s2, s, chan->name);
	  } else {
	    dprintf(idx, "    %-10s: %2d member%s enforcing \"%s\" (%s)\n",
	            chan->dname, chan->channel.members,
	            chan->channel.members == 1 ? "," : "s,", s2, s);
	  }
	} else {
	  dprintf(idx, "    %-10s: (%s), enforcing \"%s\"  (%s)\n", chan->dname,
		  channel_pending(chan) ? "pending" : "not on channel", s2, s);
	}
      } else {
	dprintf(idx, "    %-10s: channel is set +inactive\n",
		chan->dname);
      }
      if (details) {
	s[0] = 0;
	i = 0;
	if (channel_enforcebans(chan))
	  i += my_strcpy(s + i, "enforce-bans ");
	if (channel_dynamicbans(chan))
	  i += my_strcpy(s + i, "dynamic-bans ");
	if (channel_autoop(chan))
	  i += my_strcpy(s + i, "op-on-join ");
	if (channel_greet(chan))
	  i += my_strcpy(s + i, "greet ");
	if (channel_dontkickops(chan))
	  i += my_strcpy(s + i, "dont-kick-ops ");
	if (channel_logstatus(chan))
	  i += my_strcpy(s + i, "log-status ");
	if (channel_secret(chan))
	  i += my_strcpy(s + i, "secret ");
	if (!channel_static(chan))
	  i += my_strcpy(s + i, "dynamic ");
	if (channel_autovoice(chan))
	  i += my_strcpy(s + i, "autovoice ");
	if (channel_cycle(chan))
	  i += my_strcpy(s + i, "cycle ");
	if (channel_dynamicexempts(chan))
	  i += my_strcpy(s + i, "dynamic-exempts ");
	if (channel_dynamicinvites(chan))
	  i += my_strcpy(s + i, "dynamic-invites ");
	if (channel_inactive(chan))
	  i += my_strcpy(s + i, "inactive ");
	if (channel_nodesynch(chan))
	  i += my_strcpy(s + i, "nodesynch ");
	if (channel_honor_global_bans(chan))
	  i += my_strcpy(s + i, "honor-global-bans ");
	if (channel_honor_global_exempts(chan))
	  i += my_strcpy(s + i, "honor-global-exempts ");
	if (channel_honor_global_invites(chan))
	  i += my_strcpy(s + i, "honor-global-invites ");
	dprintf(idx, "      Options: %s\n", s);
	if (details) {
          dprintf(idx, "    Bans last %d mins.\n", chan->ban_time);
          dprintf(idx, "    Exemptions last %d mins.\n", chan->exempt_time);
          dprintf(idx, "    Invitations last %d mins.\n", chan->invite_time);
	}
      }
    }
  }
}

static char *channels_close()
{
  write_channels();
  free_udef(udef);
  if (lastdeletedmask)
    free(lastdeletedmask);
  rem_builtins("chon", my_chon);
  rem_builtins("dcc", C_dcc_irc);
  script_delete_commands(channel_script_cmds);
  rem_tcl_commands(channels_cmds);
  rem_tcl_strings(my_tcl_strings);
  rem_tcl_ints(my_tcl_ints);
  rem_tcl_coups(mychan_tcl_coups);
  del_hook(HOOK_USERFILE, (Function) channels_writeuserfile);
  del_hook(HOOK_BACKUP, (Function) backup_chanfile);
  del_hook(HOOK_REHASH, (Function) channels_rehash);
  del_hook(HOOK_PRE_REHASH, (Function) channels_prerehash);
  del_hook(HOOK_MINUTELY, (Function) check_expired_bans);
  del_hook(HOOK_MINUTELY, (Function) check_expired_exempts);
  del_hook(HOOK_MINUTELY, (Function) check_expired_invites);
  Tcl_UntraceVar(interp, "global-chanset",
		 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		 traced_globchanset, NULL);
  rem_help_reference("channels.help");
  rem_help_reference("chaninfo.help");
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *start();

char *start(eggdrop_t *eggdrop)
{
  egg = eggdrop;
  global_aop_min = 5;
  global_aop_max = 30;
  setstatic = 0;
  lastdeletedmask = 0;
  use_info = 1;
  strcpy(chanfile, "chanfile");
  chan_hack = 0;
  strcpy(glob_chanmode, "nt");
  udef = NULL;
  global_ban_time = 120;
  global_exempt_time = 60;
  global_invite_time = 60;
  strcpy(glob_chanset,
         "-enforcebans "
	 "+dynamicbans "
	 "-autoop "
	 "+greet "
	 "+statuslog "
	 "-secret "
	 "-autovoice "
	 "+cycle "
	 "+dontkickops "
	 "-inactive "
	 "+dynamicexempts "
	 "+dynamicinvites "
	 "+honor-global-bans "
         "+honor-global-exempts "
         "+honor-global-invites "
	 "-nodesynch ");
  module_register(MODULE_NAME, channels_table, 1, 0);
  if (!module_depend(MODULE_NAME, "eggdrop", 107, 0)) {
    module_undepend(MODULE_NAME);
    return "This module needs eggdrop1.7.0 or later";
  }
  add_hook(HOOK_MINUTELY, (Function) check_expired_bans);
  add_hook(HOOK_MINUTELY, (Function) check_expired_exempts);
  add_hook(HOOK_MINUTELY, (Function) check_expired_invites);
  add_hook(HOOK_USERFILE, (Function) channels_writeuserfile);
  add_hook(HOOK_BACKUP, (Function) backup_chanfile);
  add_hook(HOOK_REHASH, (Function) channels_rehash);
  add_hook(HOOK_PRE_REHASH, (Function) channels_prerehash);
  Tcl_TraceVar(interp, "global-chanset",
	       TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
	       traced_globchanset, NULL);
  add_builtins("chon", my_chon);
  add_builtins("dcc", C_dcc_irc);
  script_create_commands(channel_script_cmds);
  add_tcl_commands(channels_cmds);
  add_tcl_strings(my_tcl_strings);
  add_help_reference("channels.help");
  add_help_reference("chaninfo.help");
  add_tcl_ints(my_tcl_ints);
  add_tcl_coups(mychan_tcl_coups);
  read_channels(0);
  setstatic = 1;
  return NULL;
}
