/*
 * filesys.c --
 *
 *	main file of the filesys eggdrop module
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
static const char rcsid[] = "$Id: filesys.c,v 1.22 2003/03/04 22:02:27 wcc Exp $";
#endif

#include <fcntl.h>
#include <sys/stat.h>
#define MODULE_NAME "filesys"
#define MAKING_FILESYS
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <sys/file.h>
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
#include "lib/eggdrop/module.h"
#include "filedb3.h"
#include "filesys.h"
#include "src/tandem.h"
#include "files.h"
#include "dbcompat.h"
#include "filelist.h"

#define start filesys_LTX_start

static bind_table_t *BT_file;
static Function *transfer_funcs = NULL;

static eggdrop_t *egg = NULL;

/* Root dcc directory */
static char dccdir[121] = "";

/* Directory to put incoming dcc's into */
static char dccin[121] = "";

/* Let all uploads go to the user's current directory? */
static int upload_to_cd = 0;

/* Maximum allowable file size for dcc send (1M). 0 indicates
 * unlimited file size.
 */
static int dcc_maxsize = 1024;

/* Maximum number of users can be in the file area at once */
static int dcc_users = 0;

/* Where to put the filedb, if not in a hidden '.filedb' file in
 * each directory.
 */
static char filedb_path[121] = "";

static struct dcc_table DCC_FILES =
{
  "FILES",
  DCT_MASTER | DCT_VALIDIDX | DCT_SHOWWHO | DCT_SIMUL | DCT_CANBOOT | DCT_FILES,
  eof_dcc_files,
  dcc_files,
  NULL,
  NULL,
  disp_dcc_files,
  kill_dcc_files,
  out_dcc_files
};

static struct dcc_table DCC_GET =
{
  "GET",
  DCT_FILETRAN | DCT_VALIDIDX,
  eof_dcc_get,
  dcc_get,
  &wait_dcc_xfer,
  transfer_get_timeout,
  display_dcc_get,
  kill_dcc_xfer,
  out_dcc_xfer,
  outdone_dcc_xfer
};

static struct dcc_table DCC_FORK_SEND =
{
  "FORK_SEND",
  DCT_FILETRAN | DCT_FORKTYPE | DCT_FILESEND | DCT_VALIDIDX,
  eof_dcc_fork_send,
  dcc_fork_send,
  &wait_dcc_xfer,
  eof_dcc_fork_send,
  display_dcc_fork_send,
  kill_dcc_xfer,
  out_dcc_xfer
};

static struct dcc_table DCC_GET_PENDING =
{
  "GET_PENDING",
  DCT_FILETRAN | DCT_VALIDIDX,
  eof_dcc_get,
  dcc_get_pending,
  &wait_dcc_xfer,
  transfer_get_timeout,
  display_dcc_get_p,
  kill_dcc_xfer,
  out_dcc_xfer
};

static struct dcc_table DCC_SEND =
{
  "SEND",
  DCT_FILETRAN | DCT_FILESEND | DCT_VALIDIDX,
  eof_dcc_send,
  dcc_send,
  &wait_dcc_xfer,
  tout_dcc_send,
  display_dcc_send,
  kill_dcc_xfer,
  out_dcc_xfer
};

static struct user_entry_type USERENTRY_DCCDIR =
{
  NULL,				/* always NULL ;) */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  "DCCDIR"
};

static struct user_entry_type USERENTRY_FSTAT =
{
  NULL,
  fstat_gotshare,
  fstat_dupuser,
  fstat_unpack,
  fstat_pack,
  fstat_write_userfile,
  fstat_kill,
  NULL,
  fstat_set,
  fstat_tcl_get,
  fstat_tcl_set,
  fstat_display,
  "FSTAT"
};

#include "files.c"
#include "filedb3.c"
#include "tclfiles.c"
#include "dbcompat.c"
#include "filelist.c"

/* Check for tcl-bound file command, return 1 if found
 * fil: proc-name <handle> <dcc-handle> <args...>
 */
static int check_tcl_fil(char *cmd, int idx, char *text)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  int x;

  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.file->chat->con_chan);
  x = check_bind(BT_file, cmd, &fr, dcc[idx].nick, dcc[idx].sock, text);
  if (x & BIND_RET_LOG) {
    putlog(LOG_FILES, "*", "#%s# files: %s %s", dcc[idx].nick, cmd, text);
  }
  return 0;
}

static int at_limit(char *nick)
{
  int i, x = 0;

  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_GET || dcc[i].type == &DCC_GET_PENDING)
      if (!strcasecmp(dcc[i].nick, nick))
	x++;
  return (x >= dcc_limit);
}

static void wipe_tmp_filename(char *fn, int idx)
{
  int i, ok = 1;

  if (!copy_to_tmp)
    return;

  for (i = 0; i < dcc_total; i++) {
    if ((i != idx) &&
        (dcc[i].type == &DCC_GET || dcc[i].type == &DCC_GET_PENDING) &&
        (!strcmp(dcc[i].u.xfer->filename, fn))) {
      ok = 0;
      break;
    }
  }
  if (ok)
    unlink(fn);
}

static void queue_file(char *dir, char *file, char *from, char *to)
{
  fileq_t *q = fileq;

  fileq = (fileq_t *) malloc(sizeof(fileq_t));
  fileq->next = q;
  fileq->dir = strdup(dir);
  fileq->file = strdup(file);
  strcpy(fileq->nick, from);
  strcpy(fileq->to, to);
}


/* Starts a new DCC SEND or DCC RESEND connection to `nick', transferring
 * `filename' from `dir'.
 *
 * Use raw_dcc_resend() and raw_dcc_send() instead of this function.
 */
static int raw_dcc_resend_send(char *filename, char *nick, char *from,
			       char *dir, int resend, char *addr)
{
  int zz, port, i;
  char *nfn, *buf = NULL;
  long dccfilesize;
  FILE *f, *dccfile;

  putlog(LOG_DEBUG, "*", "|FILESYS| raw_dcc_resend_send(... addr=\"%s\")",
         addr);
  zz = -1;
  dccfile = fopen(filename,"r");
  fseek(dccfile, 0, SEEK_END);
  dccfilesize = ftell(dccfile);
  fclose(dccfile);
  /* File empty?! */
  if (dccfilesize == 0)
    return DCCSEND_FEMPTY;
  if (reserved_port_min > 0 && reserved_port_min < reserved_port_max) {
    for (port = reserved_port_min; port <= reserved_port_max; port++) {
      if (addr && addr[0])
        zz = open_address_listen(addr, &port);
      else {
        zz = open_listen(&port, AF_INET);
        addr = getlocaladdr(-1);
      }
      if (zz != -1)
        break;
    }
  } else {
    port = reserved_port_min;
    if (addr && addr[0])
      zz = open_address_listen(addr, &port);
    else {
      zz = open_listen(&port, AF_INET);
      addr = getlocaladdr(-1);
    }
  }

  if (zz == -1)
    return DCCSEND_NOSOCK;

  nfn = strrchr(dir, '/');
  if (nfn == NULL)
    nfn = dir;
  else
    nfn++;
  f = fopen(filename, "r");

  if (!f)
    return DCCSEND_BADFN;

  if ((i = new_dcc(&DCC_GET_PENDING, sizeof(struct xfer_info))) == -1)
     return DCCSEND_FULL;

  dcc[i].sock = zz;
  strcpy(dcc[i].addr, "222.173.240.13"); /* (IP) (-559026163);   WTF?? */
  dcc[i].port = port;
  strcpy(dcc[i].nick, nick);
  strcpy(dcc[i].host, "irc");
  dcc[i].u.xfer->filename = calloc(1, strlen(filename) + 1);
  strcpy(dcc[i].u.xfer->filename, filename);
  if (strchr(nfn, ' '))
    nfn = buf = replace_spaces(nfn);
  dcc[i].u.xfer->origname = calloc(1, strlen(nfn) + 1);
  strcpy(dcc[i].u.xfer->origname, nfn);
  strlcpy(dcc[i].u.xfer->from, from, NICKLEN);
  strlcpy(dcc[i].u.xfer->dir, dir, DIRLEN);
  dcc[i].u.xfer->length = dccfilesize;
  dcc[i].timeval = now;
  dcc[i].u.xfer->f = f;
  dcc[i].u.xfer->type = resend ? XFER_RESEND_PEND : XFER_SEND;
  if (nick[0] != '*') {
    dprintf(DP_HELP, "PRIVMSG %s :\001DCC %sSEND %s %s %d %lu\001\n", nick,
	    resend ? "RE" :  "", nfn, addr,
	    port, dccfilesize);
    putlog(LOG_FILES, "*", "Begin DCC %ssend %s to %s", resend ? "re" :  "",
	   nfn, nick);
  }
  if (buf)
    free(buf);
  return DCCSEND_OK;
}

static int raw_dcc_resend(char *filename, char *nick, char *from,
			    char *dir, char *addr)
{
  return raw_dcc_resend_send(filename, nick, from, dir, 1, addr);
}

static int raw_dcc_send(char *filename, char *nick, char *from,
			    char *dir, char *addr)
{
  return raw_dcc_resend_send(filename, nick, from, dir, 0, addr);
}

static int fstat_unpack(struct userrec *u, struct user_entry *e)
{
  char *par, *arg;
  struct filesys_stats *fs;

  fs = calloc(1, sizeof(struct filesys_stats));
  par = e->u.list->extra;
  arg = newsplit(&par);
  if (arg[0])
    fs->uploads = atoi(arg);
  arg = newsplit(&par);
  if (arg[0])
    fs->upload_ks = atoi(arg);
  arg = newsplit(&par);
  if (arg[0])
    fs->dnloads = atoi(arg);
  arg = newsplit(&par);
  if (arg[0])
    fs->dnload_ks = atoi(arg);

  list_type_kill(e->u.list);
  e->u.extra = fs;
  return 1;
}

static int fstat_pack(struct userrec *u, struct user_entry *e)
{
  struct filesys_stats *fs;
  struct list_type *l = malloc(sizeof(struct list_type));

  fs = e->u.extra;
  l->extra = malloc(41);
  snprintf(l->extra, 41, "%09u %09u %09u %09u",
          fs->uploads, fs->upload_ks, fs->dnloads, fs->dnload_ks);
  l->next = NULL;
  e->u.list = l;
  free(fs);
  return 1;
}

static int fstat_write_userfile(FILE *f, struct userrec *u,
				struct user_entry *e)
{
  struct filesys_stats *fs;

  fs = e->u.extra;
  if (fprintf(f, "--FSTAT %09u %09u %09u %09u\n",
	      fs->uploads, fs->upload_ks,
	      fs->dnloads, fs->dnload_ks) == EOF)
    return 0;
  return 1;
}

static int fstat_set(struct userrec *u, struct user_entry *e, void *buf)
{
  struct filesys_stats *fs = buf;

  if (e->u.extra != fs) {
    if (e->u.extra)
      free(e->u.extra);
    e->u.extra = fs;
  } else if (!fs)
    return 1;
  return 1;
}

static int fstat_tcl_get(Tcl_Interp *irp, struct userrec *u,
			 struct user_entry *e, int argc, char **argv)
{
  struct filesys_stats *fs;
  char d[50];

  BADARGS(3, 4, " handle FSTAT ?u/d?");
  fs = e->u.extra;
  if (argc == 3)
    snprintf(d, sizeof d, "%u %u %u %u", fs->uploads, fs->upload_ks,
                 fs->dnloads, fs->dnload_ks);
  else
    switch (argv[3][0]) {
    case 'u':
      snprintf(d, sizeof d, "%u %u", fs->uploads, fs->upload_ks);
      break;
    case 'd':
      snprintf(d, sizeof d, "%u %u", fs->dnloads, fs->dnload_ks);
      break;
    }

  Tcl_AppendResult(irp, d, NULL);
  return TCL_OK;
}

static int fstat_kill(struct user_entry *e)
{
  if (e->u.extra)
    free(e->u.extra);
  free(e);
  return 1;
}

static void fstat_display(int idx, struct user_entry *e)
{
  struct filesys_stats *fs;

  fs = e->u.extra;
  dprintf(idx, "  FILES: %u download%s (%luk), %u upload%s (%luk)\n",
	  fs->dnloads, (fs->dnloads == 1) ? "" : "s", fs->dnload_ks,
	  fs->uploads, (fs->uploads == 1) ? "" : "s", fs->upload_ks);
}

static int fstat_gotshare(struct userrec *u, struct user_entry *e,
			  char *par, int idx)
{
  return 0;
}

static int fstat_dupuser(struct userrec *u, struct userrec *o,
			 struct user_entry *e)
{
  struct filesys_stats *fs;

  if (e->u.extra) {
    fs = malloc(sizeof(struct filesys_stats));
    memcpy(fs, e->u.extra, sizeof(struct filesys_stats));
    return set_user(&USERENTRY_FSTAT, u, fs);
  }
  return 0;
}

static void stats_add_dnload(struct userrec *u, unsigned long bytes)
{
  struct user_entry *ue;
  struct filesys_stats *fs;

  if (u) {
    if (!(ue = find_user_entry(&USERENTRY_FSTAT, u)) ||
        !(fs = ue->u.extra))
      fs = calloc(1, sizeof(struct filesys_stats));
    fs->dnloads++;
    fs->dnload_ks += ((bytes + 512) / 1024);
    set_user(&USERENTRY_FSTAT, u, fs);
  }
}

static void stats_add_upload(struct userrec *u, unsigned long bytes)
{
  struct user_entry *ue;
  struct filesys_stats *fs;

  if (u) {
    if (!(ue = find_user_entry(&USERENTRY_FSTAT, u)) ||
        !(fs = ue->u.extra))
      fs = calloc(1, sizeof(struct filesys_stats));
    fs->uploads++;
    fs->upload_ks += ((bytes + 512) / 1024);
    set_user(&USERENTRY_FSTAT, u, fs);
  }
}

static int fstat_tcl_set(Tcl_Interp *irp, struct userrec *u,
			 struct user_entry *e, int argc, char **argv)
{
  struct filesys_stats *fs;
  int f = 0, k = 0;

  BADARGS(4, 6, " handle FSTAT u/d ?files ?ks??");
  if (argc > 4)
    f = atoi(argv[4]);
  if (argc > 5)
    k = atoi(argv[5]);
  switch (argv[3][0]) {
  case 'u':
  case 'd':
    if (!(fs = e->u.extra))
      fs = calloc(1, sizeof(struct filesys_stats));
    switch (argv[3][0]) {
    case 'u':
      fs->uploads = f;
      fs->upload_ks = k;
      break;
    case 'd':
      fs->dnloads = f;
      fs->dnload_ks = k;
      break;
    }
    set_user(&USERENTRY_FSTAT, u, fs);
    break;
  case 'r':
    set_user(&USERENTRY_FSTAT, u, NULL);
    break;
  }
  return TCL_OK;
}

static void dcc_files_pass(int idx, char *buf, int x)
{
  struct userrec *u = get_user_by_handle(userlist, dcc[idx].nick);

  if (!x)
    return;
  if (u_pass_match(u, buf)) {
    if (too_many_filers()) {
      dprintf(idx, _("Too many people are in the file system right now.\n"));
      dprintf(idx, _("Please try again later.\n"));
      putlog(LOG_MISC, "*", "File area full: DCC chat [%s]%s", dcc[idx].nick,
	     dcc[idx].host);
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return;
    }
    dcc[idx].type = &DCC_FILES;
    if (dcc[idx].status & STAT_TELNET)
      dprintf(idx, "\377\374\001\n");	/* turn echo back on */
    putlog(LOG_FILES, "*", "File system: [%s]%s/%d", dcc[idx].nick,
	   dcc[idx].host, dcc[idx].port);
    if (!welcome_to_files(idx)) {
      putlog(LOG_FILES, "*", _("File system broken."));
      killsock(dcc[idx].sock);
      lostdcc(idx);
    } else {
      struct userrec *u = get_user_by_handle(userlist, dcc[idx].nick);

      touch_laston(u, "filearea", now);
    }
    return;
  }
  dprintf(idx, _("Negative on that, Houston.\n"));
  putlog(LOG_MISC, "*", "Bad password: DCC chat [%s]%s", dcc[idx].nick,
	 dcc[idx].host);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

/* Hash function for file area commands.
 */
static int got_files_cmd(int idx, char *msg)
{
  char *code;

  strcpy(msg, check_bind_filt(idx, msg));
  if (!msg[0])
    return 1;
  if (msg[0] == '.')
    msg++;
  code = newsplit(&msg);
  return check_tcl_fil(code, idx, msg);
}

static void dcc_files(int idx, char *buf, int i)
{
  if (buf[0] &&
      detect_dcc_flood(&dcc[idx].timeval, dcc[idx].u.file->chat, idx))
    return;
  dcc[idx].timeval = now;
  strcpy(buf, check_bind_filt(idx, buf));
  if (!buf[0])
    return;
  touch_laston(dcc[idx].user, "filearea", now);
  if (buf[0] == ',') {
    for (i = 0; i < dcc_total; i++) {
      if ((dcc[i].type->flags & DCT_MASTER) && dcc[idx].user &&
	  (dcc[idx].user->flags & USER_MASTER) &&
	  ((dcc[i].type == &DCC_FILES) ||
	   (dcc[i].u.chat->channel >= 0)) &&
	  ((i != idx) || (dcc[idx].status & STAT_ECHO)))
	dprintf(i, "-%s- %s\n", dcc[idx].nick, &buf[1]);
    }
  } else if (got_files_cmd(idx, buf)) {
    dprintf(idx, "*** Ja mata!\n");
    flush_lines(idx, dcc[idx].u.file->chat);
    putlog(LOG_FILES, "*", "DCC user [%s]%s left file system", dcc[idx].nick,
	   dcc[idx].host);
    set_user(&USERENTRY_DCCDIR, dcc[idx].user, dcc[idx].u.file->dir);
    if (dcc[idx].status & STAT_CHAT) {
      struct chat_info *ci;

      dprintf(idx, _("Returning you to command mode...\n"));
      ci = dcc[idx].u.file->chat;
      free_null(dcc[idx].u.file);
      dcc[idx].u.chat = ci;
      dcc[idx].status &= (~STAT_CHAT);
      dcc[idx].type = &DCC_CHAT;
      if (dcc[idx].u.chat->channel >= 0) {
	chanout_but(-1, dcc[idx].u.chat->channel,
		    "*** %s has returned.\n", dcc[idx].nick);
	if (dcc[idx].u.chat->channel < 100000)
	  botnet_send_join_idx(idx, -1);
      }
    } else {
      dprintf(idx, _("Dropping connection now.\n"));
      putlog(LOG_FILES, "*", "Left files: [%s]%s/%d", dcc[idx].nick,
	     dcc[idx].host, dcc[idx].port);
      killsock(dcc[idx].sock);
      lostdcc(idx);
    }
  }
  if (dcc[idx].status & STAT_PAGE)
    flush_lines(idx, dcc[idx].u.file->chat);
}

static void my_tell_file_stats(int idx, char *hand)
{
  struct userrec *u;
  struct filesys_stats *fs;
  float fr = (-1.0), kr = (-1.0);

  u = get_user_by_handle(userlist, hand);
  if (u == NULL)
    return;
  if (!(fs = get_user(&USERENTRY_FSTAT, u))) {
    dprintf(idx, _("No file statistics for %s.\n"), hand);
  } else {
    dprintf(idx, "  uploads: %4u / %6luk\n", fs->uploads, fs->upload_ks);
    dprintf(idx, "downloads: %4u / %6luk\n", fs->dnloads, fs->dnload_ks);
    if (fs->uploads)
      fr = ((float) fs->dnloads / (float) fs->uploads);
    if (fs->upload_ks)
      kr = ((float) fs->dnload_ks / (float) fs->upload_ks);
    if (fr < 0.0)
      dprintf(idx, _("(infinite file leech)\n"));
    else
      dprintf(idx, "leech ratio (files): %6.2f\n", fr);
    if (kr < 0.0)
      dprintf(idx, _("(infinite size leech)\n"));
    else
      dprintf(idx, "leech ratio (size) : %6.2f\n", kr);
  }
}

static int cmd_files(struct userrec *u, int idx, char *par)
{
  int atr = u ? u->flags : 0;
  static struct chat_info *ci;

  if (dccdir[0] == 0)
    dprintf(idx, _("There is no file transfer area.\n"));
  else if (too_many_filers()) {
    dprintf(idx,
            P_("The maximum of %d person is in the file area right now.\n",
               "The maximum of %d people are in the file area right now.\n",
               dcc_users), dcc_users);
    dprintf(idx, _("Please try again later.\n"));
  } else {
    if (!(atr & (USER_MASTER | USER_XFER)))
      dprintf(idx, _("You don't have access to the file area.\n"));
    else {
      putlog(LOG_CMDS, "*", "#%s# files", dcc[idx].nick);
      dprintf(idx, _("Entering file system...\n"));
      if (dcc[idx].u.chat->channel >= 0) {

	chanout_but(-1, dcc[idx].u.chat->channel,
		    "*** %s has left: file system\n",
		    dcc[idx].nick);
	if (dcc[idx].u.chat->channel < 100000)
	  botnet_send_part_idx(idx, "file system");
      }
      ci = dcc[idx].u.chat;
      dcc[idx].u.file = calloc(1, sizeof(struct file_info));
      dcc[idx].u.file->chat = ci;
      dcc[idx].type = &DCC_FILES;
      dcc[idx].status |= STAT_CHAT;
      if (!welcome_to_files(idx)) {
	struct chat_info *ci = dcc[idx].u.file->chat;

	free_null(dcc[idx].u.file);
	dcc[idx].u.chat = ci;
	dcc[idx].type = &DCC_CHAT;
	putlog(LOG_FILES, "*", _("File system broken."));
	if (dcc[idx].u.chat->channel >= 0) {
	  chanout_but(-1, dcc[idx].u.chat->channel,
		      "*** %s has returned.\n",
		      dcc[idx].nick);
	  if (dcc[idx].u.chat->channel < 100000)
	    botnet_send_join_idx(idx, -1);
	}
      } else
	touch_laston(u, "filearea", now);
    }
  }
  return 0;
}

static int _dcc_send(int idx, char *filename, char *nick, char *dir,
		     int resend)
{
  int x;
  char *nfn, *buf = NULL;

  if (strlen(nick) > NICKMAX)
    nick[NICKMAX] = 0;
  if (resend)
    x = raw_dcc_resend(filename, nick, dcc[idx].nick, dir, 0);
  else
    x = raw_dcc_send(filename, nick, dcc[idx].nick, dir, 0);
  if (x == DCCSEND_FULL) {
    dprintf(idx, _("Sorry, too many DCC connections.  (try again later)\n"));
    putlog(LOG_FILES, "*", "DCC connections full: %sGET %s [%s]", filename,
	   resend ? "RE" : "", dcc[idx].nick);
    return 0;
  }
  if (x == DCCSEND_NOSOCK) {
    if (reserved_port_min) {
      dprintf(idx, _("All my DCC SEND ports are in use.  Try later.\n"));
      putlog(LOG_FILES, "*", "DCC port in use (can't open): %sGET %s [%s]",
	     resend ? "RE" : "", filename, dcc[idx].nick);
    } else {
      dprintf(idx, _("Unable to listen at a socket.\n"));
      putlog(LOG_FILES, "*", "DCC socket error: %sGET %s [%s]", filename,
	     resend ? "RE" : "", dcc[idx].nick);
    }
    return 0;
  }
  if (x == DCCSEND_BADFN) {
    dprintf(idx, _("File not found ?\n"));
    putlog(LOG_FILES, "*", "DCC file not found: %sGET %s [%s]", filename,
	   resend ? "RE" : "", dcc[idx].nick);
    return 0;
  }
  if (x == DCCSEND_FEMPTY) {
    dprintf(idx, _("The file is empty.  Aborted transfer.\n"));
    putlog(LOG_FILES, "*", "DCC file is empty: %s [%s]", filename,
	   dcc[idx].nick);
    return 0;
  }
  nfn = strrchr(dir, '/');
  if (nfn == NULL)
    nfn = dir;
  else
    nfn++;

  /* Eliminate any spaces in the filename. */
  if (strchr(nfn, ' ')) {
    char *p;

    realloc_strcpy(buf, nfn);
    p = nfn = buf;
    while ((p = strchr(p, ' ')) != NULL)
      *p = '_';
  }

  if (strcasecmp(nick, dcc[idx].nick))
    dprintf(DP_HELP, "NOTICE %s :Here is %s file from %s %s...\n", nick,
	    resend ? "the" : "a", dcc[idx].nick, resend ? "again " : "");
  dprintf(idx, "%sending: %s to %s\n", resend ? "Res" : "S", nfn, nick);
  free_null(buf);
  return 1;
}

static int do_dcc_send(int idx, char *dir, char *fn, char *nick, int resend)
{
  char *s = NULL, *s1 = NULL;
  int x;

  if (nick && strlen(nick) > NICKMAX)
    nick[NICKMAX] = 0;
  if (dccdir[0] == 0) {
    dprintf(idx, _("DCC file transfers not supported.\n"));
    putlog(LOG_FILES, "*", "Refused dcc %sget %s from [%s]", resend ? "re" : "",
	   fn, dcc[idx].nick);
    return 0;
  }
  if (strchr(fn, '/') != NULL) {
    dprintf(idx, _("Filename cannot have '/' in it...\n"));
    putlog(LOG_FILES, "*", "Refused dcc %sget %s from [%s]", resend ? "re" : "",
	   fn, dcc[idx].nick);
    return 0;
  }
  if (dir[0]) {
    s = malloc(strlen(dccdir) + strlen(dir) + strlen(fn) + 2);
    sprintf(s, "%s%s/%s", dccdir, dir, fn);
  } else {
    s = malloc(strlen(dccdir) + strlen(fn) + 1);
    sprintf(s, "%s%s", dccdir, fn);
  }
  if (!file_readable(s)) {
    dprintf(idx, "No such file.\n");
    putlog(LOG_FILES, "*", "Refused dcc %sget %s from [%s]", resend ? "re" : "",
	   fn, dcc[idx].nick);
    free_null(s);
    return 0;
  }
  if (!nick || !nick[0])
    nick = dcc[idx].nick;
  /* Already have too many transfers active for this user?  queue it */
  if (at_limit(nick)) {
    char xxx[1024];

    sprintf(xxx, "%d*%s%s", strlen(dccdir), dccdir, dir);
    queue_file(xxx, fn, dcc[idx].nick, nick);
    dprintf(idx, "Queued: %s to %s\n", fn, nick);
    free_null(s);
    return 1;
  }
  if (copy_to_tmp) {
    char *tempfn = mktempfile(fn);

    /* Copy this file to /tmp, add a random prefix to the filename. */
    s = realloc(s, strlen(dccdir) + strlen(dir) + strlen(fn) + 2);
    sprintf(s, "%s%s%s%s", dccdir, dir, dir[0] ? "/" : "", fn);
    s1 = realloc(s1, strlen(tempdir) + strlen(tempfn) + 1);
    sprintf(s1, "%s%s", tempdir, tempfn);
    free_null(tempfn);
    if (copyfile(s, s1) != 0) {
      dprintf(idx, _("Can't make temporary copy of file!\n"));
      putlog(LOG_FILES | LOG_MISC, "*",
	     "Refused dcc %sget %s: copy to %s FAILED!",
	     resend ? "re" : "", fn, tempdir);
      free_null(s);
      free_null(s1);
      return 0;
    }
  } else {
    s1 = realloc(s1, strlen(dccdir) + strlen(dir) + strlen(fn) + 2);
    sprintf(s1, "%s%s%s%s", dccdir, dir, dir[0] ? "/" : "", fn);
  }
  s = realloc(s, strlen(dir) + strlen(fn) + 2);
  sprintf(s, "%s%s%s", dir, dir[0] ? "/" : "", fn);
  x = _dcc_send(idx, s1, nick, s, resend);
  if (x != DCCSEND_OK)
    wipe_tmp_filename(s1, -1);
  free_null(s);
  free_null(s1);
  return x;
}

static void tout_dcc_files_pass(int i)
{
  dprintf(i, _("Timeout.\n"));
  putlog(LOG_MISC, "*", "Password timeout on dcc chat: [%s]%s", dcc[i].nick,
	 dcc[i].host);
  killsock(dcc[i].sock);
  lostdcc(i);
}

static void disp_dcc_files(int idx, char *buf)
{
  sprintf(buf, "file  flags: %c%c%c%c%c",
	  dcc[idx].status & STAT_CHAT ? 'C' : 'c',
	  dcc[idx].status & STAT_PARTY ? 'P' : 'p',
	  dcc[idx].status & STAT_TELNET ? 'T' : 't',
	  dcc[idx].status & STAT_ECHO ? 'E' : 'e',
	  dcc[idx].status & STAT_PAGE ? 'P' : 'p');
}

static void disp_dcc_files_pass(int idx, char *buf)
{
  sprintf(buf, "fpas  waited %lus", now - dcc[idx].timeval);
}

static void kill_dcc_files(int idx, void *x)
{
  register struct file_info *f = (struct file_info *) x;

  if (f->chat)
    DCC_CHAT.kill(idx, f->chat);
  free_null(x);
}

static void eof_dcc_files(int idx)
{
  dcc[idx].u.file->chat->con_flags = 0;
  putlog(LOG_MISC, "*", "Lost dcc connection to %s (%s/%d)", dcc[idx].nick,
	 dcc[idx].host, dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void out_dcc_files(int idx, char *buf, void *x)
{
  register struct file_info *p = (struct file_info *) x;

  if (p->chat)
    DCC_CHAT.output(idx, buf, p->chat);
  else
    tputs(dcc[idx].sock, buf, strlen(buf));
}

static cmd_t mydcc[] =
{
  {"files",		"-",	cmd_files,		NULL},
  {NULL,		NULL,	NULL,			NULL}
};

static tcl_strings mystrings[] =
{
  {"files_path",	dccdir,		120,	STR_DIR | STR_PROTECT},
  {"incoming_path",	dccin,		120,	STR_DIR | STR_PROTECT},
  {"filedb_path",	filedb_path,	120,	STR_DIR | STR_PROTECT},
  {NULL,		NULL,		0,	0}
};

static tcl_ints myints[] =
{
  {"max_filesize",	&dcc_maxsize},
  {"max_file_users",	&dcc_users},
  {"upload_to_pwd",	&upload_to_cd},
  {NULL,		NULL}
};

static struct dcc_table DCC_FILES_PASS =
{
  "FILES_PASS",
  0,
  eof_dcc_files,
  dcc_files_pass,
  NULL,
  tout_dcc_files_pass,
  disp_dcc_files_pass,
  kill_dcc_files,
  out_dcc_files
};


static void filesys_dcc_send_hostresolved(int);

/* Received a ctcp-dcc.
 */
static void filesys_dcc_send(char *nick, char *from, struct userrec *u,
			     char *text)
{
  char *param, *ip, *prt, *buf = NULL, *msg;
  int atr = u ? u->flags : 0, i;
  unsigned long ipnum;
  char ipbuf[32];

  buf = malloc(strlen(text) + 1);
  msg = buf;
  strcpy(buf, text);
  param = newsplit(&msg);
  if (!(atr & USER_XFER)) {
    putlog(LOG_FILES, "*",
	   "Refused DCC SEND %s (no access): %s!%s", param,
	   nick, from);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :No access\n", nick);
  } else if (!dccin[0] && !upload_to_cd) {
    dprintf(DP_HELP,
	    "NOTICE %s :DCC file transfers not supported.\n", nick);
    putlog(LOG_FILES, "*",
	   "Refused dcc send %s from %s!%s", param, nick, from);
  } else if (strchr(param, '/')) {
    dprintf(DP_HELP,
	    "NOTICE %s :Filename cannot have '/' in it...\n", nick);
    putlog(LOG_FILES, "*",
	   "Refused dcc send %s from %s!%s", param, nick, from);
  } else {
    ip = newsplit(&msg);
    prt = newsplit(&msg);
    if (atoi(prt) < 1024 || atoi(prt) > 65535) {
      /* Invalid port */
      dprintf(DP_HELP, "NOTICE %s :%s (invalid port)\n", nick,
	      _("Failed to connect"));
      putlog(LOG_FILES, "*", "Refused dcc send %s (%s): invalid port", param,
	     nick);
    } else if (atoi(msg) == 0) {
      dprintf(DP_HELP,
	      "NOTICE %s :Sorry, file size info must be included.\n",
	      nick);
      putlog(LOG_FILES, "*", "Refused dcc send %s (%s): no file size",
	     param, nick);
    } else if (dcc_maxsize && (atoi(msg) > (dcc_maxsize * 1024))) {
      dprintf(DP_HELP, "NOTICE %s :Sorry, file too large.\n", nick);
      putlog(LOG_FILES, "*", "Refused dcc send %s (%s): file too large", param,
	     nick);
    } else {
      i = new_dcc(&DCC_DNSWAIT, sizeof(struct dns_info));
      if (i < 0) {
	dprintf(DP_HELP, "NOTICE %s :Sorry, too many DCC connections.\n",
		nick);
	putlog(LOG_MISC, "*", "DCC connections full: SEND %s (%s!%s)",
	       param, nick, from);
	return;
      }
      putlog(LOG_DEBUG, "*", "|FILESYS| dcc send ip: (%s)", ip);
      ipnum = strtoul(ip, NULL, 10);
      ipnum = htonl(ipnum);
      inet_ntop(AF_INET, &ipnum, ipbuf, sizeof(ipbuf));
      ip = ipbuf;
      strlcpy(dcc[i].addr, ip, ADDRLEN);
      putlog(LOG_DEBUG, "*", "|FILESYS| addr: (%s)", dcc[i].addr);
      dcc[i].port = atoi(prt);
      dcc[i].sock = (-1);
      dcc[i].user = u;
      strcpy(dcc[i].nick, nick);
      strcpy(dcc[i].host, from);
      dcc[i].u.dns->cbuf = calloc(1, strlen(param) + 1);
      strcpy(dcc[i].u.dns->cbuf, param);
      dcc[i].u.dns->ibuf = atoi(msg);
      
      dcc[i].u.dns->host = calloc(1, strlen(dcc[i].addr) + 1);
      strcpy(dcc[i].u.dns->host, dcc[i].addr);

      dcc[i].u.dns->dns_type = RES_HOSTBYIP;
      dcc[i].u.dns->dns_success = filesys_dcc_send_hostresolved;
      dcc[i].u.dns->dns_failure = filesys_dcc_send_hostresolved;
      dcc[i].u.dns->type = &DCC_FORK_SEND;
      dcc_dnshostbyip(dcc[i].addr);
    }
  }
  free_null(buf);
}

/* Create a temporary filename with random elements. Shortens
 * the filename if the total string is longer than NAME_MAX.
 * The original buffer is not modified.   (Fabian)
 *
 * Please adjust MKTEMPFILE_TOT if you change any lengths
 *   7 - size of the random string
 *   2 - size of additional characters in "%u-%s-%s" format string
 *   8 - estimated size of getpid()'s output converted to %u */
#define MKTEMPFILE_TOT (7 + 2 + 8)
static char *mktempfile(char *filename)
{
  char rands[8], *tempname, *fn = filename;
  int l;

  make_rand_str(rands, 7);
  l = strlen(filename);
  if ((l + MKTEMPFILE_TOT) > NAME_MAX) {
    fn[NAME_MAX - MKTEMPFILE_TOT] = 0;
    l = NAME_MAX - MKTEMPFILE_TOT;
    fn = malloc(l + 1);
    strncpy(fn, filename, l);
    fn[l] = 0;
  }
  tempname = malloc(l + MKTEMPFILE_TOT + 1);
  sprintf(tempname, "%u-%s-%s", getpid(), rands, fn);
  if (fn != filename)
    free_null(fn);
  return tempname;
}

static void filesys_dcc_send_hostresolved(int i)
{
  char *s1, *param = NULL, prt[100], *tempf;
  int len = dcc[i].u.dns->ibuf, j;

  sprintf(prt, "%d", dcc[i].port);
  realloc_strcpy(param, dcc[i].u.dns->cbuf);

  changeover_dcc(i, &DCC_FORK_SEND, sizeof(struct xfer_info));
  if (param[0] == '.')
    param[0] = '_';
  /* Save the original filename */
  dcc[i].u.xfer->origname = calloc(1, strlen(param) + 1);
  strcpy(dcc[i].u.xfer->origname, param);
  tempf = mktempfile(param);
  dcc[i].u.xfer->filename = calloc(1, strlen(tempf) + 1);
  strcpy(dcc[i].u.xfer->filename, tempf);
  /* We don't need the temporary buffers anymore */
  free_null(tempf);
  free_null(param);

  if (upload_to_cd) {
    char *p = get_user(&USERENTRY_DCCDIR, dcc[i].user);

    if (p)
      sprintf(dcc[i].u.xfer->dir, "%s%s/", dccdir, p);
    else
      sprintf(dcc[i].u.xfer->dir, "%s", dccdir);
  } else
    strcpy(dcc[i].u.xfer->dir, dccin);
  dcc[i].u.xfer->length = len;
  s1 = malloc(strlen(dcc[i].u.xfer->dir) +
	       strlen(dcc[i].u.xfer->origname) + 1);
  sprintf(s1, "%s%s", dcc[i].u.xfer->dir, dcc[i].u.xfer->origname);
  if (file_readable(s1)) {
    dprintf(DP_HELP, "NOTICE %s :File `%s' already exists.\n",
	    dcc[i].nick, dcc[i].u.xfer->origname);
    lostdcc(i);
    free_null(s1);
  } else {
    free_null(s1);
    /* Check for dcc-sends in process with the same filename */
    for (j = 0; j < dcc_total; j++)
      if (j != i) {
        if ((dcc[j].type->flags & (DCT_FILETRAN | DCT_FILESEND))
	    == (DCT_FILETRAN | DCT_FILESEND)) {
	  if (!strcmp(dcc[i].u.xfer->origname, dcc[j].u.xfer->origname)) {
	    dprintf(DP_HELP, "NOTICE %s :File `%s' is already being sent.\n",
		    dcc[i].nick, dcc[i].u.xfer->origname);
	    lostdcc(i);
	    return;
	  }
	}
      }
    /* Put uploads in /tmp first */
    s1 = malloc(strlen(tempdir) + strlen(dcc[i].u.xfer->filename) + 1);
    sprintf(s1, "%s%s", tempdir, dcc[i].u.xfer->filename);
    dcc[i].u.xfer->f = fopen(s1, "w");
    free_null(s1);
    if (dcc[i].u.xfer->f == NULL) {
      dprintf(DP_HELP,
	      "NOTICE %s :Can't create file `%s' (temp dir error)\n",
	      dcc[i].nick, dcc[i].u.xfer->origname);
      lostdcc(i);
    } else {
      dcc[i].timeval = now;
      dcc[i].sock = getsock(SOCK_BINARY);
      if (dcc[i].sock < 0 || open_telnet_dcc(dcc[i].sock, dcc[i].addr, prt) < 0)
	dcc[i].type->eof(i);
    }
  }
}

/* This only handles CHAT requests, otherwise it's handled in transfer.
 */
static int filesys_DCC_CHAT(char *nick, char *from, char *handle,
			    char *object, char *keyword, char *text)
{
  char *param, *ip, *prt, buf[512], *msg = buf;
  int i, sock;
  struct userrec *u = get_user_by_handle(userlist, handle);
  struct flag_record fr = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  if (strcasecmp(object, botname))
    return 0;
  if (!strncasecmp(text, "SEND ", 5)) {
    filesys_dcc_send(nick, from, u, text + 5);
    return 1;
  }
  if (strncasecmp(text, "CHAT ", 5) || !u)
    return 0;
  strcpy(buf, text + 5);
  get_user_flagrec(u, &fr, 0);
  param = newsplit(&msg);
  if (dcc_total == max_dcc) {
    putlog(LOG_MISC, "*", _("DCC connections full: %s %s (%s!%s)"), "CHAT(file)", param, nick, from);
  } else if (glob_party(fr))
    return 0;			/* Allow ctcp.so to pick up the chat */
  else if (!glob_xfer(fr)) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("No access"));
    putlog(LOG_MISC, "*", "%s: %s!%s", _("Refused DCC chat (no access)"), nick, from);
  } else if (u_pass_match(u, "-")) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You must have a password set."));
    putlog(LOG_MISC, "*", "%s: %s!%s", _("Refused DCC chat (no password)"), nick, from);
  } else if (!dccdir[0]) {
    putlog(LOG_MISC, "*", "%s: %s!%s", _("Refused DCC chat (+x but no file area)"), nick, from);
  } else {
    ip = newsplit(&msg);
    prt = newsplit(&msg);
    sock = getsock(0);
    if (sock < 0 || open_telnet_dcc(sock, ip, prt) < 0) {
      neterror(buf);
      if (!quiet_reject)
        dprintf(DP_HELP, "NOTICE %s :%s (%s)\n", nick,
	        _("Failed to connect"), buf);
      putlog(LOG_MISC, "*", "%s: CHAT(file) (%s!%s)", _("DCC connection failed"),
	     nick, from);
      putlog(LOG_MISC, "*", "    (%s)", buf);
      killsock(sock);
    } else if (atoi(prt) < 1024 || atoi(prt) > 65535) {
      /* Invalid port */
      if (!quiet_reject)
        dprintf(DP_HELP, "NOTICE %s :%s", nick,
	        _("Failed to connect (invalid port)\n"));
      putlog(LOG_FILES, "*", "%s: %s!%s", _("Refused DCC chat (invalid port)"), nick, from);

    } else {
      unsigned int ip_int;

      i = new_dcc(&DCC_FILES_PASS, sizeof(struct file_info));
      sscanf(ip, "%u", &ip_int);
      strcpy(dcc[i].addr, iptostr(htonl(ip_int)));
      dcc[i].port = atoi(prt);
      dcc[i].sock = sock;
      strcpy(dcc[i].nick, u->handle);
      strcpy(dcc[i].host, from);
      dcc[i].status = STAT_ECHO;
      dcc[i].timeval = now;
      dcc[i].u.file->chat = calloc(1, sizeof(struct chat_info));
      strcpy(dcc[i].u.file->chat->con_chan, "*");
      dcc[i].user = u;
      putlog(LOG_MISC, "*", "DCC connection: CHAT(file) (%s!%s)", nick, from);
      dprintf(i, "%s\n", _("Enter your password."));
    }
  }
  return 1;
}

static cmd_t myctcp[] =
{
  {"DCC",	"",	filesys_DCC_CHAT,		"files:DCC"},
  {NULL,	NULL,	NULL,				NULL}
};

static void init_server_ctcps(char *module)
{
  add_builtins("ctcp", myctcp);
}

static cmd_t myload[] =
{
  {"server",	"",	(Function) init_server_ctcps,	"filesys:server"},
  {NULL,	NULL,	NULL,				NULL}
};

static void filesys_report(int idx, int details)
{
  if (details) {
    if (dccdir[0]) {
      dprintf(idx, _("    DCC file path: %s"), dccdir);
      if (upload_to_cd)
        dprintf(idx, _("\n        incoming: (go to the current dir)\n"));
      else if (dccin[0])
        dprintf(idx, _("\n        incoming: %s\n"), dccin);
      else
        dprintf(idx, _("    (no uploads)\n"));
      if (dcc_users)
        dprintf(idx, _("        max users is %d\n"), dcc_users);
      if ((upload_to_cd) || (dccin[0]))
        dprintf(idx, _("    DCC max file size: %dk\n"), dcc_maxsize);
    } else
      dprintf(idx, _("  (Filesystem module loaded, but no active dcc path.)\n"));
  }
}

static char *filesys_close()
{
  int i;

  putlog(LOG_MISC, "*", _("Unloading filesystem, killing all filesystem connections."));
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_FILES) {
      dprintf(i, _("-=- poof -=-\n"));
      dprintf(i,
	 _("You have been booted from the filesystem, module unloaded.\n"));
      killsock(dcc[i].sock);
      lostdcc(i);
    } else if (dcc[i].type == &DCC_FILES_PASS) {
      killsock(dcc[i].sock);
      lostdcc(i);
    }
  rem_tcl_commands(mytcls);
  rem_tcl_strings(mystrings);
  rem_tcl_ints(myints);
  rem_builtins("dcc", mydcc);
  rem_builtins("load", myload);
  rem_help_reference("filesys.help");
  rem_builtins("ctcp", myctcp);
  bind_table_del(BT_file);
  del_entry_type(&USERENTRY_DCCDIR);
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *start();

static Function filesys_table[] =
{
  /* 0 - 3 */
  (Function) start,
  (Function) filesys_close,
  (Function) 0,
  (Function) filesys_report,
  /* 4 - 7 */
  (Function) 0,
  (Function) add_file,
  (Function) incr_file_gots,
  (Function) is_valid,
};

char *start(eggdrop_t *eggdrop)
{
  egg = eggdrop;

  module_register(MODULE_NAME, filesys_table, 2, 0);
  if (!module_depend(MODULE_NAME, "eggdrop", 107, 0)) {
    module_undepend(MODULE_NAME);
    return _("This module requires eggdrop1.7.0 or later");
  }
  add_tcl_commands(mytcls);
  add_tcl_strings(mystrings);
  add_tcl_ints(myints);
  BT_file = bind_table_add("file", 3, "sis", MATCH_MASK, BIND_STACKABLE);
  add_builtins("dcc", mydcc);
  add_builtins("load", myload);
  add_builtins("file", myfiles);
  add_help_reference("filesys.help");
  init_server_ctcps(0);
  memcpy(&USERENTRY_DCCDIR, &USERENTRY_INFO, sizeof(struct user_entry_type) -
         sizeof(char *));
  add_entry_type(&USERENTRY_DCCDIR);
  DCC_FILES_PASS.timeout_val = &password_timeout;
  return NULL;
}

static int is_valid()
{
  return dccdir[0];
}
