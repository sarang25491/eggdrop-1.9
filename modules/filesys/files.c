/*
 * files.c --
 *
 *	handles all file system commands
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

/* FIXME: #include mess
#ifndef lint
static const char rcsid[] = "$Id: files.c,v 1.13 2003/05/12 23:32:40 wcc Exp $";
#endif
*/

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

static void show_queued_files(int idx)
{
  int i, cnt = 0, len;
  char spaces[] = "                                 ";
  fileq_t *q;

  for (q = fileq; q; q = q->next) {
    if (!strcasecmp(q->nick, dcc[idx].nick)) {
      if (!cnt) {
	spaces[HANDLEN - 9] = 0;
	dprintf(idx, "  Send to  %s  Filename\n", spaces);
	dprintf(idx, "  ---------%s  --------------------\n", spaces);
	spaces[HANDLEN - 9] = ' ';
      }
      cnt++;
      spaces[len = HANDLEN - strlen(q->to)] = 0;
      if (q->dir[0] == '*')
	dprintf(idx, "  %s%s  %s/%s\n", q->to, spaces, &q->dir[1],
		q->file);
      else
	dprintf(idx, "  %s%s  /%s%s%s\n", q->to, spaces, q->dir,
		q->dir[0] ? "/" : "", q->file);
      spaces[len] = ' ';
    }
  }
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type == &DCC_GET_PENDING || dcc[i].type == &DCC_GET) &&
	(!strcasecmp(dcc[i].nick, dcc[idx].nick) ||
	 !strcasecmp(dcc[i].u.xfer->from, dcc[idx].nick))) {
      char *nfn;

      if (!cnt) {
	spaces[HANDLEN - 9] = 0;
	dprintf(idx, "  Send to  %s  Filename\n", spaces);
	dprintf(idx, "  ---------%s  --------------------\n", spaces);
	spaces[HANDLEN - 9] = ' ';
      }
      nfn = strrchr(dcc[i].u.xfer->origname, '/');
      if (nfn == NULL)
	nfn = dcc[i].u.xfer->origname;
      else
	nfn++;
      cnt++;
      spaces[len = HANDLEN - strlen(dcc[i].nick)] = 0;
      if (dcc[i].type == &DCC_GET_PENDING)
	dprintf(idx, "  %s%s  %s  [WAITING]\n", dcc[i].nick, spaces,
		nfn);
      else
	dprintf(idx, "  %s%s  %s  (%.1f%% done)\n", dcc[i].nick, spaces,
		nfn, (100.0 * ((float) dcc[i].status /
			       (float) dcc[i].u.xfer->length)));
      spaces[len] = ' ';
    }
  }
  if (!cnt)
    dprintf(idx, "No files queued up.\n");
  else
    dprintf(idx, "Total: %d\n", cnt);
}

/* Are there too many people in the file system?
 */
static int too_many_filers()
{
  int i, n = 0;

  if (dcc_users == 0)
    return 0;
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_FILES)
      n++;
  return (n >= dcc_users);
}

/* Someone uploaded a file -- add it
 */
static void add_file(char *dir, char *file, char *nick)
{
  FILE *f;

  /* Gave me a full pathname.
   * Only continue if the destination is within the visible file system.
   */
  if (!strncmp(dccdir, dir, strlen(dccdir)) &&
      (f = filedb_open(&dir[strlen(dccdir)], 2))) {
    filedb_add(f, file, nick);
    filedb_close(f);
  }
}

static int welcome_to_files(int idx)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  FILE *f;
  char *p = get_user(&USERENTRY_DCCDIR, dcc[idx].user);

  dprintf(idx, "\n");
  if (fr.global & USER_JANITOR)
    fr.global |= USER_MASTER;

  /* Show motd if the user went straight here without going thru the
   * party line.
   */
  if (!(dcc[idx].status & STAT_CHAT))
    show_motd(idx);
  sub_lang(idx, _("Welcome to the %B file server\n"));
  sub_lang(idx, _("All file transfers will be sent to '%N' by default.\n(You can specify an alternate nick with the 'get' command.)\nType 'help' for help.\n"));
  if (p)
    strcpy(dcc[idx].u.file->dir, p);
  else
    dcc[idx].u.file->dir[0] = 0;
  /* Does this dir even exist any more? */
  f = filedb_open(dcc[idx].u.file->dir, 0);
  if (f == NULL) {
    dcc[idx].u.file->dir[0] = 0;
    f = filedb_open(dcc[idx].u.file->dir, 0);
    if (f == NULL) {
      dprintf(idx, _("\nThe file system seems to be broken right now.\n"));
      dprintf(idx, _("(The files-path is set to an invalid directory.)\n"));
      dprintf(idx, "\n\n");
      dccdir[0] = 0;
      chanout_but(-1, dcc[idx].u.file->chat->channel,
		  "*** %s rejoined the party line.\n",
		  dcc[idx].nick);
      botnet_send_join_idx(idx, dcc[idx].u.file->chat->channel);
      return 0;			/* failed */
    }
  }
  filedb_close(f);
  dprintf(idx, "%s: /%s\n\n", _("Current directory"), dcc[idx].u.file->dir);
  return 1;
}

static void cmd_optimize(int idx, char *par)
{
  struct userrec *u = get_user_by_handle(userlist, dcc[idx].nick);
  FILE *fdb = NULL;
  char *p   = NULL;

  putlog(LOG_FILES, "*", "files: #%s# optimize", dcc[idx].nick);
  p = get_user(&USERENTRY_DCCDIR, u);
  /* Does this dir even exist any more? */
  if (p) {
    fdb = filedb_open(p, 1);
    if (!fdb) {
      set_user(&USERENTRY_DCCDIR, u, NULL);
      p = NULL;
    }
  }
  if (!p)
    fdb = filedb_open("", 1);
  if (!fdb) {
    dprintf(idx, _("Illegal directory.\n"));
    return;
  }
  filedb_close(fdb);
  dprintf(idx, "Current directory is now optimized.\n");
}

/* Given current directory, and the desired changes, fill 'real' with
 * the new current directory.  check directory permissions along the
 * way.  return 1 if the change can happen, 0 if not. 'real' will be
 * assigned newly allocated memory, so don't forget to free it...
 */
static int resolve_dir(char *current, char *change, char **real, int idx)
{
  char *elem = NULL, *s = NULL, *new = NULL, *work = NULL, *p = NULL;
  FILE *fdb  = NULL;
  DIR  *dir  = NULL;
  filedb_entry *fdbe = NULL;
  struct flag_record user = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0},
		     req  = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  *real = NULL;
  realloc_strcpy(*real, current);
  if (!change[0])
    return 1;				/* No change? */
  new = malloc(strlen(change) + 2);	/* Add 2, because we add '/' below */
  strcpy(new, change);
  if (new[0] == '/') {
    /* EVERYONE has access here */
    (*real)[0] = 0;
    strcpy(new, &new[1]);
  }
  /* Cycle thru the elements */
  strcat(new, "/");
  p = strchr(new, '/');
  while (p) {
    *p = 0;
    p++;
    realloc_strcpy(elem, new);
    strcpy(new, p);
    if (!elem[0] || !strcmp(elem, ".")) {
      p = strchr(new, '/');
      continue;
     }
     else if (!strcmp(elem, "..")) {
      /* Always allowed */
      p = strrchr(*real, '/');
      if (p == NULL) {
	/* Can't go back from here? */
	if (!(*real)[0]) {
	  free_null(elem);
	  free_null(new);
	  realloc_strcpy(*real, current);
	  return 0;
	}
	(*real)[0] = 0;
      } else
	*p = 0;
    } else {
      /* Allowed access here? */
      fdb = filedb_open(*real, 0);
      if (!fdb) {
	/* Non-existent starting point! */
	free_null(elem);
	free_null(new);
	realloc_strcpy(*real, current);
	return 0;
      }
      filedb_readtop(fdb, NULL);
      fdbe = filedb_matchfile(fdb, ftell(fdb), elem);
      filedb_close(fdb);
      if (!fdbe) {
	/* Non-existent */
	free_null(elem);
	free_null(new);
        free_null(s);
	realloc_strcpy(*real, current);
	return 0;
      }
      if (!(fdbe->stat & FILE_DIR)) {
	/* Not a dir */
	free_fdbe(&fdbe);
	free_null(elem);
	free_null(new);
        free_null(s);
	realloc_strcpy(*real, current);
	return 0;
      }
      if (idx >= 0)
	get_user_flagrec(dcc[idx].user, &user, fdbe->chan);
      else
	user.global = USER_OWNER | USER_BOT | USER_MASTER | USER_OP |
		      USER_FRIEND;

      if (fdbe->flags_req) {
        break_down_flags(fdbe->flags_req, &req, NULL);
        if (!flagrec_ok(&req, &user)) {
	  free_fdbe(&fdbe);
	  free_null(elem);
	  free_null(new);
          free_null(s);
	  realloc_strcpy(*real, current);
	  return 0;
	}
      }
      free_fdbe(&fdbe);
      realloc_strcpy(s, *real);
      if (s[0] && s[strlen(s) - 1] != '/') {
        s = nrealloc(s, strlen(s) + 2);
        strcat(s, "/");
      }
      work = malloc(strlen(s) + strlen(elem) + 1);
      sprintf(work, "%s%s", s, elem);
      realloc_strcpy(*real, work);
      s = realloc(s, strlen(dccdir) + strlen(*real) + 1);
      sprintf(s, "%s%s", dccdir, *real);
    }
    p = strchr(new, '/');
  }
  free_null(new);
  if (elem)
    free_null(elem);
  if (work)
    free_null(work);
  /* Sanity check: does this dir exist? */
  s = realloc(s, strlen(dccdir) + strlen(*real) + 1);
  sprintf(s, "%s%s", dccdir, *real);
  dir = opendir(s);
  free_null(s);
  if (!dir)
    return 0;
  closedir(dir);
  return 1;
}

static void incr_file_gots(char *ppath)
{
  char *p, *path = NULL, *destdir = NULL, *fn = NULL;
  filedb_entry *fdbe;
  FILE *fdb;

  /* Absolute dir?  probably a tcl script sending it, and it might not
   * be in the file system at all, so just leave it alone.
   */
  if ((ppath[0] == '*') || (ppath[0] == '/'))
    return;
  realloc_strcpy(path, ppath);
  p = strrchr(path, '/');
  if (p != NULL) {
    *p = 0;
    realloc_strcpy(destdir, path);
    realloc_strcpy(fn, p + 1);
    *p = '/';
  } else {
    realloc_strcpy(destdir, "");
    realloc_strcpy(fn, path);
  }
  fdb = filedb_open(destdir, 0);
  if (!fdb) {
    free_null(path);
    free_null(destdir);
    free_null(fn);
    return;			/* Not my concern, then */
  }
  free_null(path);
  free_null(destdir);
  filedb_readtop(fdb, NULL);
  fdbe = filedb_matchfile(fdb, ftell(fdb), fn);
  free_null(fn);
  if (fdbe) {
    fdbe->gots++;
    filedb_updatefile(fdb, fdbe->pos, fdbe, UPDATE_HEADER);
    free_fdbe(&fdbe);
  }
  filedb_close(fdb);
}

/*** COMMANDS ***/

static void cmd_pwd(int idx, char *par)
{
  putlog(LOG_FILES, "*", "files: #%s# pwd", dcc[idx].nick);
  dprintf(idx, "%s: /%s\n", _("Current directory"), dcc[idx].u.file->dir);
}

static void cmd_pending(int idx, char *par)
{
  show_queued_files(idx);
  putlog(LOG_FILES, "*", "files: #%s# pending", dcc[idx].nick);
}

static void cmd_cancel(int idx, char *par)
{
  if (!par[0]) {
    dprintf(idx, "%s: cancel <file-mask>\n", _("Usage"));
    return;
  }
  fileq_cancel(idx, par);
  putlog(LOG_FILES, "*", "files: #%s# cancel %s", dcc[idx].nick, par);
}

static void cmd_chdir(int idx, char *msg)
{
  char *s = NULL;

  if (!msg[0]) {
    dprintf(idx, "%s: cd <new-dir>\n", _("Usage"));
    return;
  }
  if (!resolve_dir(dcc[idx].u.file->dir, msg, &s, idx)) {
    dprintf(idx, _("No such directory.\n"));
    free_null(s);
    return;
  }
  strlcpy(dcc[idx].u.file->dir, s, sizeof dcc[idx].u.file->dir);
  free_null(s);
  set_user(&USERENTRY_DCCDIR, dcc[idx].user, dcc[idx].u.file->dir);
  putlog(LOG_FILES, "*", "files: #%s# cd /%s", dcc[idx].nick,
	 dcc[idx].u.file->dir);
  dprintf(idx, "%s: /%s\n", _("New current directory"), dcc[idx].u.file->dir);
}

static void files_ls(int idx, char *par, int showall)
{
  char *p, *s = NULL, *destdir = NULL, *mask = NULL;
  FILE *fdb;

  if (par[0]) {
    putlog(LOG_FILES, "*", "files: #%s# ls %s", dcc[idx].nick, par);
    p = strrchr(par, '/');
    if (p != NULL) {
      *p = 0;
      realloc_strcpy(s, par);
      realloc_strcpy(mask, p + 1);
      if (!resolve_dir(dcc[idx].u.file->dir, s, &destdir, idx)) {
	dprintf(idx, _("Illegal directory.\n"));
	free_null(s);
	free_null(mask);
	free_null(destdir);
	return;
      }
      free_null(s);
    } else {
      realloc_strcpy(destdir, dcc[idx].u.file->dir);
      realloc_strcpy(mask, par);
    }
    /* Might be 'ls dir'? */
    if (resolve_dir(destdir, mask, &s, idx)) {
      /* Aha! it was! */
      realloc_strcpy(destdir, s);
      realloc_strcpy(mask, "*");
    }
    free_null(s);
    fdb = filedb_open(destdir, 0);
    if (!fdb) {
      dprintf(idx, _("Illegal directory.\n"));
      free_null(destdir);
      free_null(mask);
      return;
    }
    filedb_ls(fdb, idx, mask, showall);
    filedb_close(fdb);
    free_null(destdir);
    free_null(mask);
  } else {
    putlog(LOG_FILES, "*", "files: #%s# ls", dcc[idx].nick);
    fdb = filedb_open(dcc[idx].u.file->dir, 0);
    if (fdb) {
      filedb_ls(fdb, idx, "*", showall);
      filedb_close(fdb);
    } else
      dprintf(idx, _("Illegal directory.\n"));
  }
}

static void cmd_ls(int idx, char *par)
{
  files_ls(idx, par, 0);
}

static void cmd_lsa(int idx, char *par)
{
  files_ls(idx, par, 1);
}

/* FIXME: botnet dependencies */
static void cmd_reget_get(int idx, char *par, int resend)
{
  int ok = 0, i;
  char *p, *what, *destdir = NULL, *s = NULL;
  filedb_entry *fdbe;
  FILE *fdb;
  long where = 0;
  int nicklen = NICKLEN;

  /* Get the nick length if necessary. */
  if (NICKLEN > 9) {
    module_entry *me = module_find("server", 1, 1);

    if (me && me->funcs)
      nicklen = (int) me->funcs[SERVER_NICKLEN];
  }
  if (!par[0]) {
    dprintf(idx, "%s: %sget <file(s)> [nickname]\n", _("Usage"),
	    resend ? "re" : "");
    return;
  }
  what = newsplit(&par);
  if (strlen(par) > nicklen) {
    dprintf(idx, _("Be reasonable.\n"));
    return;
  }
  p = strrchr(what, '/');
  if (p != NULL) {
    *p = 0;
    realloc_strcpy(s, what);
    strcpy(what, p + 1);
    if (!resolve_dir(dcc[idx].u.file->dir, s, &destdir, idx)) {
      free_null(destdir);
      free_null(s);
      dprintf(idx, _("Illegal directory.\n"));
      return;
    }
    free_null(s);
  } else
    realloc_strcpy(destdir, dcc[idx].u.file->dir);
  fdb = filedb_open(destdir, 0);
  if (!fdb)
    return;
  filedb_readtop(fdb, NULL);
  fdbe = filedb_matchfile(fdb, ftell(fdb), what);
  if (!fdbe) {
    filedb_close(fdb);
    free_fdbe(&fdbe);
    free_null(destdir);
    dprintf(idx, _("No matching files.\n"));
    return;
  }
  while (fdbe) {
    where = ftell(fdb);
    if (!(fdbe->stat & (FILE_HIDDEN | FILE_DIR))) {
      ok = 1;
      do_dcc_send(idx, destdir, fdbe->filename, par, resend);
    }
    free_fdbe(&fdbe);
    fdbe = filedb_matchfile(fdb, where, what);
  }
  filedb_close(fdb);
  free_null(destdir);
  if (!ok)
    dprintf(idx, _("No matching files.\n"));
  else
    putlog(LOG_FILES, "*", "files: #%s# %sget %s %s", dcc[idx].nick,
	   resend ? "re" : "", what, par);
}

static void cmd_reget(int idx, char *par)
{
  cmd_reget_get(idx, par, 1);
}

static void cmd_get(int idx, char *par)
{
  cmd_reget_get(idx, par, 0);
}

static void cmd_file_help(int idx, char *par)
{
  char *s;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.file->chat->con_chan);
  if (par[0]) {
    putlog(LOG_FILES, "*", "files: #%s# help %s", dcc[idx].nick, par);
    s = malloc(strlen(par) + 9);
    sprintf(s, "filesys/%s", par);
    s[256] = 0;
    tellhelp(idx, s, &fr, 0);
    free_null(s);
  } else {
    putlog(LOG_FILES, "*", "files: #%s# help", dcc[idx].nick);
    tellhelp(idx, "filesys/help", &fr, 0);
  }
}

static void cmd_hide(int idx, char *par)
{
  FILE *fdb;
  filedb_entry *fdbe;
  long where = 0;
  int ok = 0;

  if (!par[0]) {
    dprintf(idx, "%s: hide <file(s)>\n", _("Usage"));
    return;
  }
  fdb = filedb_open(dcc[idx].u.file->dir, 0);
  if (!fdb)
    return;
  filedb_readtop(fdb, NULL);
  fdbe = filedb_matchfile(fdb, ftell(fdb), par);
  if (!fdbe) {
    filedb_close(fdb);
    dprintf(idx, _("No matching files.\n"));
    return;
  }
  while (fdbe) {
    where = ftell(fdb);
    if (!(fdbe->stat & FILE_HIDDEN)) {
      fdbe->stat |= FILE_HIDDEN;
      ok++;
      dprintf(idx, "%s: %s\n", _("Hid"), fdbe->filename);
      filedb_updatefile(fdb, fdbe->pos, fdbe, UPDATE_HEADER);
    }
    free_fdbe(&fdbe);
    fdbe = filedb_matchfile(fdb, where, par);
  }
  filedb_close(fdb);
  if (!ok)
    dprintf(idx, _("No matching files.\n"));
  else {
    putlog(LOG_FILES, "*", "files: #%s# hide %s", dcc[idx].nick, par);
    if (ok > 1)
      dprintf(idx, "%s %d file%s.\n", _("Hid"), ok, ok == 1 ? "" : "s");
  }
}

static void cmd_unhide(int idx, char *par)
{
  FILE *fdb;
  filedb_entry *fdbe;
  long where;
  int ok = 0;

  if (!par[0]) {
    dprintf(idx, "%s: unhide <file(s)>\n", _("Usage"));
    return;
  }
  fdb = filedb_open(dcc[idx].u.file->dir, 0);
  if (!fdb)
    return;
  filedb_readtop(fdb, NULL);
  fdbe = filedb_matchfile(fdb, ftell(fdb), par);
  if (!fdbe) {
    filedb_close(fdb);
    dprintf(idx, _("No matching files.\n"));
    return;
  }
  while (fdbe) {
    where = ftell(fdb);
    if (fdbe->stat & FILE_HIDDEN) {
      fdbe->stat &= ~FILE_HIDDEN;
      ok++;
      dprintf(idx, "%s: %s\n", _("Unhid"), fdbe->filename);
      filedb_updatefile(fdb, fdbe->pos, fdbe, UPDATE_HEADER);
    }
    free_fdbe(&fdbe);
    fdbe = filedb_matchfile(fdb, where, par);
  }
  filedb_close(fdb);
  if (!ok)
    dprintf(idx, _("No matching files.\n"));
  else {
    putlog(LOG_FILES, "*", "files: #%s# unhide %s", dcc[idx].nick, par);
    if (ok > 1)
      dprintf(idx, "%s %d file%s.\n", _("Unhid"), ok, ok == 1 ? "" : "s");
  }
}

static void cmd_desc(int idx, char *par)
{
  char *fn, *desc, *p, *q;
  int ok = 0, lin;
  FILE *fdb;
  filedb_entry *fdbe;
  long where;

  fn = newsplit(&par);
  if (!fn[0]) {
    dprintf(idx, "%s: desc <filename> <new description>\n", _("Usage"));
    return;
  }
  /* Fix up desc */
  desc = malloc(strlen(par) + 2);
  strcpy(desc, par);
  strcat(desc, "|");
  /* Replace | with linefeeds, limit 5 lines */
  lin = 0;
  q = desc;
  while ((*q <= 32) && (*q))
    strcpy(q, &q[1]);		/* Zapf leading spaces */
  p = strchr(q, '|');
  while (p != NULL) {
    /* Check length */
    *p = 0;
    if (strlen(q) > 60) {
      /* Cut off at last space or truncate */
      *p = '|';
      p = q + 60;
      while ((*p != ' ') && (p != q))
	p--;
      if (p == q)
	*(q + 60) = '|';	/* No space, so truncate it */
      else
	*p = '|';
      p = strchr(q, '|');	/* Go back, find my place, and continue */
    }
    *p = '\n';
    q = p + 1;
    lin++;
    while ((*q <= 32) && (*q))
      strcpy(q, &q[1]);
    if (lin == 5) {
      *p = 0;
      p = NULL;
    } else
      p = strchr(q, '|');
  }
  if (desc[strlen(desc) - 1] == '\n')
    desc[strlen(desc) - 1] = 0;
  fdb = filedb_open(dcc[idx].u.file->dir, 0);
  if (!fdb) {
    free_null(desc);
    return;
  }
  filedb_readtop(fdb, NULL);
  fdbe = filedb_matchfile(fdb, ftell(fdb), fn);
  if (!fdbe) {
    filedb_close(fdb);
    dprintf(idx, _("No matching files.\n"));
    free_null(desc);
    return;
  }
  while (fdbe) {
    where = ftell(fdb);
    if (!(fdbe->stat & FILE_HIDDEN)) {
      ok = 1;
      if ((!(dcc[idx].user->flags & USER_JANITOR)) &&
	  (strcasecmp(fdbe->uploader, dcc[idx].nick)))
	dprintf(idx, _("You didnt upload %s\n"), fdbe->filename);
      else {
	if (desc[0]) {
	  /* If the current description is the same as the new
	   * one, don't change anything.
	   */
	  if (fdbe->desc && !strcmp(fdbe->desc, desc)) {
	    free_fdbe(&fdbe);
	    fdbe = filedb_matchfile(fdb, where, fn);
	    continue;
	  }
	  realloc_strcpy(fdbe->desc, desc);
	} else if (fdbe->desc) {
	  free_null(fdbe->desc);
	}
	filedb_updatefile(fdb, fdbe->pos, fdbe, UPDATE_ALL);
	if (par[0])
	  dprintf(idx, "%s: %s\n", _("Changed"), fdbe->filename);
	else
	  dprintf(idx, "%s: %s\n", _("Blanked"), fdbe->filename);
      }
    }
    free_fdbe(&fdbe);
    fdbe = filedb_matchfile(fdb, where, fn);
  }
  filedb_close(fdb);
  if (!ok)
    dprintf(idx, _("No matching files.\n"));
  else
    putlog(LOG_FILES, "*", "files: #%s# desc %s", dcc[idx].nick, fn);
  free_null(desc);
}

static void cmd_rm(int idx, char *par)
{
  FILE *fdb;
  filedb_entry *fdbe;
  long where;
  int ok = 0;
  char *s;

  if (!par[0]) {
    dprintf(idx, "%s: rm <file(s)>\n", _("Usage"));
    return;
  }
  fdb = filedb_open(dcc[idx].u.file->dir, 0);
  if (!fdb)
    return;
  filedb_readtop(fdb, NULL);
  fdbe = filedb_matchfile(fdb, ftell(fdb), par);
  if (!fdbe) {
    filedb_close(fdb);
    dprintf(idx, _("No matching files.\n"));
    return;
  }
  while (fdbe) {
    where = ftell(fdb);
    if (!(fdbe->stat & (FILE_HIDDEN | FILE_DIR))) {
      s = malloc(strlen(dccdir) + strlen(dcc[idx].u.file->dir)
		  + strlen(fdbe->filename) + 2);
      sprintf(s, "%s%s/%s", dccdir, dcc[idx].u.file->dir, fdbe->filename);
      ok++;
      filedb_delfile(fdb, fdbe->pos);
      unlink(s);
      dprintf(idx, "%s: %s\n", _("Erased"), fdbe->filename);
      free_null(s);
    }
    free_fdbe(&fdbe);
    fdbe = filedb_matchfile(fdb, where, par);
  }
  filedb_close(fdb);
  if (!ok)
    dprintf(idx, _("No matching files.\n"));
  else {
    putlog(LOG_FILES, "*", "files: #%s# rm %s", dcc[idx].nick, par);
    if (ok > 1)
      dprintf(idx, "%s %d file%s.\n", _("Erased"), ok, ok == 1 ? "" : "s");
  }
}

static void cmd_mkdir(int idx, char *par)
{
  char *name, *flags, *chan, *s;
  FILE *fdb;
  filedb_entry *fdbe;
  int ret;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (!par[0]) {
    dprintf(idx, "%s: mkdir <dir> [required-flags] [channel]\n", _("Usage"));
    return;
  }
  name = newsplit(&par);
  ret = strlen(name);
  if (ret > 60)
    name[(ret = 60)] = 0;
  if (name[ret] == '/')
    name[ret] = 0;
  if (strchr(name, '/'))
    dprintf(idx, "You can only create directories in the current directory\n");
  else {
    flags = newsplit(&par);
    chan = newsplit(&par);
    if (!chan[0] && flags[0] && (strchr(CHANMETA, flags[0]) != NULL)) {
      /* Need some extra checking here to makesure we dont mix up
       * the flags with a +channel. <cybah>
       */
      if(!findchan(flags) && flags[0] != '+') {
	dprintf(idx, "Invalid channel!\n");
	return;
      } else if(findchan(flags)) {
	/* Flags is a channel. */
	chan = flags;
	flags = par;
      }	/* (else) Couldn't find the channel and flags[0] is a '+', these
	 * are flags. */
    }
    if (chan[0] && !findchan(chan)) {
      dprintf(idx, "Invalid channel!\n");
      return;
    }
    fdb = filedb_open(dcc[idx].u.file->dir, 0);
    if (!fdb)
      return;
    filedb_readtop(fdb, NULL);
    fdbe = filedb_matchfile(fdb, ftell(fdb), name);
    if (!fdbe) {
      s = malloc(strlen(dccdir) + strlen(dcc[idx].u.file->dir)
		  + strlen(name) + 2);
      sprintf(s, "%s%s/%s", dccdir, dcc[idx].u.file->dir, name);
      if (mkdir(s, 0755) != 0) {
	dprintf(idx, _("Failed.\n"));
	filedb_close(fdb);
	free_null(s);
	return;
      }
      free_null(s);
      fdbe = malloc_fdbe();
      fdbe->stat = FILE_DIR;
      realloc_strcpy(fdbe->filename, name);
      fdbe->uploaded = now;
      dprintf(idx, "%s /%s%s%s\n", _("Created directory"), dcc[idx].u.file->dir,
	      dcc[idx].u.file->dir[0] ? "/" : "", name);
    } else if (!(fdbe->stat & FILE_DIR)) {
      dprintf(idx, _("No such directory.\n"));
      free_fdbe(&fdbe);
      filedb_close(fdb);
      return;
    }
    if (flags[0]) {
      char buffer[100];

      break_down_flags(flags, &fr, NULL);
      build_flags(buffer, &fr, NULL);
      realloc_strcpy(fdbe->flags_req, buffer);
      dprintf(idx, _("Changed %s/ to require +%s to access\n"), name, buffer);
    } else if (!chan[0]) {
      free_null(fdbe->flags_req);
      dprintf(idx, _("Changes %s/ to require no flags to access\n"), name);
    }
    if (chan[0]) {
      realloc_strcpy(fdbe->chan, chan);
      dprintf(idx, "Access set to channel: %s\n", chan);
    } else if (!flags[0]) {
      free_null(fdbe->chan);
      dprintf(idx, "Access set to all channels.\n");
    }
    if (!fdbe->pos)
      fdbe->pos = POS_NEW;
    filedb_updatefile(fdb, fdbe->pos, fdbe, UPDATE_ALL);
    filedb_close(fdb);
    free_fdbe(&fdbe);
    putlog(LOG_FILES, "*", "files: #%s# mkdir %s %s", dcc[idx].nick, name, par);
  }
}

static void cmd_rmdir(int idx, char *par)
{
  FILE *fdb;
  filedb_entry *fdbe;
  char *s, *name = NULL;

  realloc_strcpy(name, par);
  if (name[strlen(name) - 1] == '/')
    name[strlen(name) - 1] = 0;
  if (strchr(name, '/'))
    dprintf(idx, "You can only create directories in the current directory\n");
  else {
    fdb = filedb_open(dcc[idx].u.file->dir, 0);
    if (!fdb) {
      free_null(name);
      return;
    }
    filedb_readtop(fdb, NULL);
    fdbe = filedb_matchfile(fdb, ftell(fdb), name);
    if (!fdbe) {
      dprintf(idx, _("No such directory.\n"));
      filedb_close(fdb);
      free_null(name);
      return;
    }
    if (!(fdbe->stat & FILE_DIR)) {
      dprintf(idx, _("No such directory.\n"));
      filedb_close(fdb);
      free_null(name);
      free_fdbe(&fdbe);
      return;
    }
    /* Erase '.filedb' and '.files' if they exist */
    s = malloc(strlen(dccdir) + strlen(dcc[idx].u.file->dir)
		+ strlen(name) + 10);
    sprintf(s, "%s%s/%s/.filedb", dccdir, dcc[idx].u.file->dir, name);
    unlink(s);
    sprintf(s, "%s%s/%s/.files", dccdir, dcc[idx].u.file->dir, name);
    unlink(s);
    sprintf(s, "%s%s/%s", dccdir, dcc[idx].u.file->dir, name);
    if (rmdir(s) == 0) {
      dprintf(idx, "%s /%s%s%s\n", _("Removed directory"), dcc[idx].u.file->dir,
	      dcc[idx].u.file->dir[0] ? "/" : "", name);
      filedb_delfile(fdb, fdbe->pos);
      filedb_close(fdb);
      free_fdbe(&fdbe);
      free_null(s);
      free_null(name);
      putlog(LOG_FILES, "*", "files: #%s# rmdir %s", dcc[idx].nick, name);
      return;
    }
    dprintf(idx, _("Failed.\n"));
    filedb_close(fdb);
    free_fdbe(&fdbe);
    free_null(s);
    free_null(name);
  }
}

static void cmd_mv_cp(int idx, char *par, int copy)
{
  char *p, *fn, *oldpath = NULL, *s = NULL, *s1, *newfn = NULL;
  char *newpath = NULL;
  int   ok = 0, only_first = 0, skip_this = 0;
  FILE *fdb_old = NULL, *fdb_new = NULL;
  filedb_entry *fdbe_old = NULL, *fdbe_new = NULL;
  long where = 0;

  fn = newsplit(&par);
  if (!par[0]) {
    dprintf(idx, "%s: %s <oldfilepath> <newfilepath>\n",
	    _("Usage"), copy ? "cp" : "mv");
    return;
  }
  p = strrchr(fn, '/');
  if (p != NULL) {
    *p = 0;
    realloc_strcpy(s, fn);
    strcpy(fn, p + 1);
    if (!resolve_dir(dcc[idx].u.file->dir, s, &oldpath, idx)) {
      dprintf(idx, _("Illegal source directory.\n"));
      free_null(s);
      free_null(oldpath);
      return;
    }
    free_null(s);
  } else
    realloc_strcpy(oldpath, dcc[idx].u.file->dir);
  realloc_strcpy(s, par);
  if (!resolve_dir(dcc[idx].u.file->dir, s, &newpath, idx)) {
    free_null(newpath);
    /* Destination is not just a directory */
    p = strrchr(s, '/');
    if (p == NULL) {
      realloc_strcpy(newfn, s);
      s[0] = 0;
    } else {
      *p = 0;
      realloc_strcpy(newfn, p + 1);
    }
    if (!resolve_dir(dcc[idx].u.file->dir, s, &newpath, idx)) {
      dprintf(idx, _("Illegal destination directory.\n"));
      free_null(newfn);
      free_null(s);
      free_null(oldpath);
      free_null(newpath);
      return;
    }
  } else
    realloc_strcpy(newfn, "");
  free_null(s);
  /* Stupidness checks */
  if ((!strcmp(oldpath, newpath)) &&
      ((!newfn[0]) || (!strcmp(newfn, fn)))) {
    dprintf(idx, _("You cant %s files on top of themselves.\n"), copy ? _("copy") : _("move"));
    free_null(oldpath);
    free_null(newpath);
    free_null(newfn);
    return;
  }
  /* Be aware of 'cp * this.file' possibility: ONLY COPY FIRST ONE */
  if ((strchr(fn, '?') || strchr(fn, '*')) && newfn[0])
    only_first = 1;
  else
    only_first = 0;

  fdb_old = filedb_open(oldpath, 0);
  if (!strcmp(oldpath, newpath))
    fdb_new = fdb_old;
  else
    fdb_new = filedb_open(newpath, 0);
  if (!fdb_old || !fdb_new) {
    free_null(oldpath);
    free_null(newpath);
    free_null(newfn);
    return;
  }

  filedb_readtop(fdb_old, NULL);
  fdbe_old = filedb_matchfile(fdb_old, ftell(fdb_old), fn);
  if (!fdbe_old) {
    if (fdb_new != fdb_old)
      filedb_close(fdb_new);
    filedb_close(fdb_old);
    free_null(oldpath);
    free_null(newpath);
    free_null(newfn);
    return;
  }
  while (fdbe_old) {
    where = ftell(fdb_old);
    skip_this = 0;
    if (!(fdbe_old->stat & (FILE_HIDDEN | FILE_DIR))) {
      s = malloc(strlen(dccdir) + strlen(oldpath)
		  + strlen(fdbe_old->filename) + 2);
      s1 = malloc(strlen(dccdir) + strlen(newpath)
		   + strlen(newfn[0] ? newfn : fdbe_old->filename) + 2);
      sprintf(s, "%s%s%s%s", dccdir, oldpath,
	      oldpath[0] ? "/" : "", fdbe_old->filename);
      sprintf(s1, "%s%s%s%s", dccdir, newpath,
	      newpath[0] ? "/" : "", newfn[0] ? newfn : fdbe_old->filename);
      if (!strcmp(s, s1)) {
	dprintf(idx, "%s /%s%s%s %s\n", _("onto itself?  Nuh uh."),
	        copy ? _("copy") : _("move"), newpath,
	        newpath[0] ? "/" : "", newfn[0] ? newfn : fdbe_old->filename);
	skip_this = 1;
      }
      /* Check for existence of file with same name in new dir */
      filedb_readtop(fdb_new, NULL);
      fdbe_new = filedb_matchfile(fdb_new, ftell(fdb_new),
				  newfn[0] ? newfn : fdbe_old->filename);
      if (fdbe_new) {
	/* It's ok if the entry in the new dir is a normal file (we'll
	 * just scrap the old entry and overwrite the file) -- but if
	 * it's a directory, this file has to be skipped.
	 */
	if (fdbe_new->stat & FILE_DIR) {
	  /* Skip */
	  skip_this = 1;
	} else {
	  filedb_delfile(fdb_new, fdbe_new->pos);
	}
	free_fdbe(&fdbe_new);
      }
      if (!skip_this) {
        if ((copy ? copyfile(s, s1) : movefile(s, s1)) == 0) {
	  /* Raw file moved okay: create new entry for it */
	  ok++;
	  fdbe_new = malloc_fdbe();
	  fdbe_new->stat = fdbe_old->stat;
	  /* We don't have to worry about any entries to be
	   * NULL, because realloc_strcpy takes care of that.
	   */
	  realloc_strcpy(fdbe_new->flags_req, fdbe_old->flags_req);
	  realloc_strcpy(fdbe_new->chan, fdbe_old->chan);
	  realloc_strcpy(fdbe_new->filename, fdbe_old->filename);
	  realloc_strcpy(fdbe_new->desc, fdbe_old->desc);
	  if (newfn[0])
	    realloc_strcpy(fdbe_new->filename, newfn);
	  realloc_strcpy(fdbe_new->uploader, fdbe_old->uploader);
	  fdbe_new->uploaded = fdbe_old->uploaded;
	  fdbe_new->size = fdbe_old->size;
	  fdbe_new->gots = fdbe_old->gots;
	  filedb_addfile(fdb_new, fdbe_new);
	  if (!copy)
	    filedb_delfile(fdb_old, fdbe_old->pos);
	  free_fdbe(&fdbe_new);
	}
      }
      free_null(s);
      free_null(s1);
    }
    free_fdbe(&fdbe_old);
    fdbe_old = filedb_matchfile(fdb_old, where, fn);
    if (ok && only_first)
      free_fdbe(&fdbe_old);
  }
  if (fdb_old != fdb_new)
    filedb_close(fdb_new);
  filedb_close(fdb_old);
  if (!ok)
    dprintf(idx, _("No matching files.\n"));
  else {
    putlog(LOG_FILES, "*", "files: #%s# %s %s%s%s %s", dcc[idx].nick,
	   copy ? "cp" : "mv", oldpath, oldpath[0] ? "/" : "", fn, par);
    if (ok > 1)
      dprintf(idx, "%s %d file%s.\n",
	      copy ? _("Copied") : _("Moved"), ok, ok == 1 ? "" : "s");
  }
  free_null(oldpath);
  free_null(newpath);
  free_null(newfn);
}

static void cmd_mv(int idx, char *par)
{
  cmd_mv_cp(idx, par, 0);
}

static void cmd_cp(int idx, char *par)
{
  cmd_mv_cp(idx, par, 1);
}

static int cmd_stats(int idx, char *par)
{
  putlog(LOG_FILES, "*", "#%s# stats", dcc[idx].nick);
  my_tell_file_stats(idx, dcc[idx].nick);
  return 0;
}

static int cmd_filestats(int idx, char *par)
{
  char *nick;
  struct userrec *u;

  if (!par[0]) {
    dprintf(idx, "Usage: filestats <user>\n");
    return 0;
  }
  nick = newsplit(&par);
  putlog(LOG_FILES, "*", "#%s# filestats %s", dcc[idx].nick, nick);
  if (nick[0] == 0)
    my_tell_file_stats(idx, dcc[idx].nick);
  else if (!(u = get_user_by_handle(userlist, nick)))
    dprintf(idx, "No such user.\n");
  else if (!strcmp(par, "clear") && dcc[idx].user &&
	   (dcc[idx].user->flags & USER_JANITOR)) {
    set_user(&USERENTRY_FSTAT, u, NULL);
    dprintf(idx, "Cleared filestats for %s.\n", nick);
  } else
    my_tell_file_stats(idx, nick);
  return 0;
}

/* This function relays the dcc call to cmd_note() in the notes module,
 * if loaded.
 */
static void filesys_note(int idx, char *par)
{
  struct userrec *u = get_user_by_handle(userlist, dcc[idx].nick);
  module_entry *me = module_find("notes", 2, 1);

  if (me && me->funcs) {
    Function f = me->funcs[NOTES_CMD_NOTE];

    (f) (u, idx, par);
  } else {
    dprintf(idx, "Sending of notes is not supported.\n");
  }
}

static cmd_t myfiles[] =
{
  {"cancel",	"",	(Function) cmd_cancel,		NULL},
  {"cd",	"",	(Function) cmd_chdir,		NULL},
  {"chdir",	"",	(Function) cmd_chdir,		NULL},
  {"cp",	"j",	(Function) cmd_cp,		NULL},
  {"desc",	"",	(Function) cmd_desc,		NULL},
  {"filestats",	"j",	(Function) cmd_filestats,	NULL},
  {"get",	"",	(Function) cmd_get,		NULL},
  {"reget",	"",	(Function) cmd_reget,		NULL},
  {"help",	"",	(Function) cmd_file_help,	NULL},
  {"hide",	"j",	(Function) cmd_hide,		NULL},
  {"ls",	"",	(Function) cmd_ls,		NULL},
  {"lsa",	"j",	(Function) cmd_lsa,		NULL},
  {"mkdir",	"j",	(Function) cmd_mkdir,		NULL},
  {"mv",	"j",	(Function) cmd_mv,		NULL},
  {"note",	"",	(Function) filesys_note,	NULL},
  {"pending",	"",	(Function) cmd_pending,		NULL},
  {"pwd",	"",	(Function) cmd_pwd,		NULL},
  {"quit",	"",	(Function) CMD_LEAVE,		NULL},
  {"rm",	"j",	(Function) cmd_rm,		NULL},
  {"rmdir",	"j",	(Function) cmd_rmdir,		NULL},
  {"optimize",	"j",	(Function) cmd_optimize,	NULL},
  {"stats",	"",	(Function) cmd_stats,		NULL},
  {"unhide",	"j",	(Function) cmd_unhide,		NULL},
  {NULL,	NULL,	NULL,				NULL}
};


/*
 *    Tcl stub functions
 */
/* FIXME: botnet dependencies */
static int files_reget(int idx, char *fn, char *nick, int resend)
{
  int i = 0;
  char *p = NULL, *what = NULL, *destdir, *s = NULL;
  filedb_entry *fdbe = NULL;
  FILE *fdb = NULL;

  p = strrchr(fn, '/');
  if (p != NULL) {
    *p = 0;
    realloc_strcpy(s, fn);
    realloc_strcpy(what, p + 1);
    if (!resolve_dir(dcc[idx].u.file->dir, s, &destdir, idx)) {
      free_null(s);
      free_null(what);
      free_null(destdir);
      return 0;
    }
    free_null(s);
  } else {
    realloc_strcpy(destdir, dcc[idx].u.file->dir);
    realloc_strcpy(what, fn);
  }
  fdb = filedb_open(destdir, 0);
  if (!fdb) {
    free_null(what);
    free_null(destdir);
    return 0;
  }
  filedb_readtop(fdb, NULL);
  fdbe = filedb_matchfile(fdb, ftell(fdb), what);
  if (!fdbe) {
    filedb_close(fdb);
    free_null(what);
    free_null(destdir);
    return 0;
  }
  if (fdbe->stat & (FILE_HIDDEN | FILE_DIR)) {
    filedb_close(fdb);
    free_null(what);
    free_null(destdir);
    free_fdbe(&fdbe);
    return 0;
  }
  filedb_close(fdb);
  do_dcc_send(idx, destdir, fdbe->filename, nick, resend);
  free_null(what);
  free_null(destdir);
  free_fdbe(&fdbe);
  /* Don't increase got count till later */
  return 1;
}

static void files_setpwd(int idx, char *where)
{
  char *s;

  if (!resolve_dir(dcc[idx].u.file->dir, where, &s, idx))
    return;
  strcpy(dcc[idx].u.file->dir, s);
  set_user(&USERENTRY_DCCDIR, get_user_by_handle(userlist, dcc[idx].nick),
	   dcc[idx].u.file->dir);
  free_null(s);
}
