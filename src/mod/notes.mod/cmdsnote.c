/*
 * cmdsnote.c -- part of notes.mod
 *   handles all notes interaction over the party line
 *
 * $Id: cmdsnote.c,v 1.13 2001/08/10 23:51:21 ite Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001 Eggheads Development Team
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

static void cmd_pls_noteign(struct userrec *u, int idx, char *par)
{
  struct userrec *u2;
  char *handle, *mask, *buf, *p;
  if (!par[0]) {
    dprintf(idx, "%s: +noteign [handle] <ignoremask>\n", _("Usage"));
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# +noteign %s", dcc[idx].nick, par);

  p = buf = nmalloc(strlen(par)+1);
  strcpy(p, par);
  handle = newsplit(&p);
  mask = newsplit(&p);
  if (mask[0]) {
    u2 = get_user_by_handle(userlist, handle);
    if (u != u2) {
      struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
      get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
      if (!(glob_master(fr) || glob_owner(fr))) {
	dprintf(idx, _("You are not allowed to change note ignores for %s\n"), handle);
	nfree(buf);
        return;
      }
    }
    if (!u2) {
      dprintf(idx, _("User %s does not exist.\n"), handle);
      nfree(buf);
      return;
    }
  } else {
    u2 = u;
    mask = handle;
  }
  if (add_note_ignore(u2, mask))
    dprintf(idx, _("Now ignoring notes from %s\n"), mask);
  else
    dprintf(idx, _("Already ignoring %s\n"), mask);
  nfree(buf);
  return;
}

static void cmd_mns_noteign(struct userrec *u, int idx, char *par)
{
  struct userrec *u2;
  char *handle, *mask, *buf, *p;
  if (!par[0]) {
    dprintf(idx, "%s: -noteign [handle] <ignoremask>\n", _("Usage"));
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# -noteign %s", dcc[idx].nick, par);
  p = buf = nmalloc(strlen(par)+1);
  strcpy(p, par);
  handle = newsplit(&p);
  mask = newsplit(&p);
  if (mask[0]) {
    u2 = get_user_by_handle(userlist, handle);
    if (u != u2) {
      struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
      get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
      if (!(glob_master(fr) || glob_owner(fr))) {
	dprintf(idx, _("You are not allowed to change note ignores for %s\n"), handle);
	nfree(buf);
        return;
      }
    }
    if (!u2) {
      dprintf(idx, _("User %s does not exist.\n"), handle);
      nfree(buf);
      return;
    }
  } else {
    u2 = u;
    mask = handle;
  }

  if (del_note_ignore(u2, mask))
    dprintf(idx, _("No longer ignoring notes from %s\n"), mask);
  else
    dprintf(idx, _("Note ignore %s not found in list.\n"), mask);
  nfree(buf);
  return;
}

static void cmd_noteigns(struct userrec *u, int idx, char *par)
{
  struct userrec *u2;
  char **ignores;
  int ignoresn, i;

  if (par[0]) {
    u2 = get_user_by_handle(userlist, par);
    if (u != u2) {
      struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
      get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
      if (!(glob_master(fr) || glob_owner(fr))) {
	dprintf(idx, _("You are not allowed to change note ignores for %s\n"), par);
        return;
      }
    }
    if (!u2) {
      dprintf(idx, _("User %s does not exist.\n"), par);
      return;
    }
  } else
    u2 = u;

  ignoresn = get_note_ignores(u2, &ignores);
  if (!ignoresn) {
    dprintf(idx, "%s", _("No note ignores present.\n"));
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# noteigns %s", dcc[idx].nick, par);
  dprintf(idx, _("Note ignores for %s:\n"), u2->handle);
  for (i = 0; i < ignoresn; i++)
    dprintf(idx, " %s", ignores[i]);
  dprintf(idx, "\n");
  nfree(ignores[0]);		/* Free the string buffer	*/
  nfree(ignores);		/* Free the ptr array		*/
}

static void cmd_fwd(struct userrec *u, int idx, char *par)
{
  char *handle;
  struct userrec *u1;

  if (!par[0]) {
    dprintf(idx, "%s: fwd <handle> [user@bot]\n", _("Usage"));
    return;
  }
  handle = newsplit(&par);
  u1 = get_user_by_handle(userlist, handle);
  if (!u1) {
    dprintf(idx, "%s\n", _("No such user."));
    return;
  }
  if ((u1->flags & USER_OWNER) && egg_strcasecmp(handle, dcc[idx].nick)) {
    dprintf(idx, "%s\n", _("Cant change notes forwarding of the bot owner.\n"));
    return;
  }
  if (!par[0]) {
    putlog(LOG_CMDS, "*", "#%s# fwd %s", dcc[idx].nick, handle);
    dprintf(idx, _("Wiped notes forwarding for %s\n"), handle);
    set_user(&USERENTRY_FWD, u1, NULL);
    return;
  }
  /* Thanks to vertex & dw */
  if (strchr(par, '@') == NULL) {
    dprintf(idx, "%s\n", _("You must supply a botname to forward to."));
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# fwd %s %s", dcc[idx].nick, handle, par);
  dprintf(idx, _("Changed notes forwarding for %s to: %s\n"), handle, par);
  set_user(&USERENTRY_FWD, u1, par);
}

static void cmd_notes(struct userrec *u, int idx, char *par)
{
  char *fcn;

  if (!par[0]) {
    dprintf(idx, "%s: notes index\n", _("Usage"));
    dprintf(idx, "       notes read <# or ALL>\n");
    dprintf(idx, "       notes erase <# or ALL>\n");
    dprintf(idx, "       %s\n", _("# may be numbers and/or intervals separated by ;"));
    dprintf(idx, "       ex: notes erase 2-4;8;16-\n");
    return;
  }
  fcn = newsplit(&par);
  if (!egg_strcasecmp(fcn, "index"))
    notes_read(dcc[idx].nick, "", "+", idx);
  else if (!egg_strcasecmp(fcn, "read")) {
    if (!egg_strcasecmp(par, "all"))
      notes_read(dcc[idx].nick, "", "-", idx);
    else
      notes_read(dcc[idx].nick, "", par, idx);
  } else if (!egg_strcasecmp(fcn, "erase")) {
    if (!egg_strcasecmp(par, "all"))
      notes_del(dcc[idx].nick, "", "-", idx);
    else
      notes_del(dcc[idx].nick, "", par, idx);
  } else {
    dprintf(idx, "%s\n", _("Function must be one of INDEX, READ, or ERASE."));
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# notes %s %s", dcc[idx].nick, fcn, par);
}

static void cmd_note(struct userrec *u, int idx, char *par)
{
  char handle[512], *p;
  int echo;

  p = newsplit(&par);
  if (!par[0]) {
    dprintf(idx, "%s: note <to-whom> <message>\n", _("Usage"));
    return;
  }
  while ((*par == ' ') || (*par == '<') || (*par == '>'))
    par++;			/* These are now illegal *starting* notes
				 * characters */
  echo = (dcc[idx].status & STAT_ECHO);
  splitc(handle, p, ',');
  while (handle[0]) {
    rmspace(handle);
    add_note(handle, dcc[idx].nick, par, idx, echo);
    splitc(handle, p, ',');
  }
  rmspace(p);
  add_note(p, dcc[idx].nick, par, idx, echo);
}

static cmd_t notes_cmds[] =
{
  {"fwd",	"m",	(Function) cmd_fwd,		NULL},
  {"notes",	"",	(Function) cmd_notes,		NULL},
  {"+noteign",	"",	(Function) cmd_pls_noteign,	NULL},
  {"-noteign",	"",	(Function) cmd_mns_noteign,	NULL},
  {"noteigns",	"",	(Function) cmd_noteigns,	NULL},
  {"note",	"",	(Function) cmd_note,		NULL},
  {NULL,	NULL,	NULL,				NULL}
};

