/*
 * proto.h
 *   prototypes for every function used outside its own module
 *
 * (i guess i'm not very modular, cuz there are a LOT of these.)
 * with full prototyping, some have been moved to other .h files
 * because they use structures in those
 * (saves including those .h files EVERY time) - Beldin
 *
 * $Id: proto.h,v 1.68 2002/03/04 02:32:38 stdarg Exp $
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

#ifndef _EGG_PROTO_H
#define _EGG_PROTO_H

#include "lush.h"

#include "irccmp.h"
#include "match.h"

#ifdef HAVE_DPRINTF
#define dprintf dprintf_eggdrop
#endif

struct chanset_t;		/* keeps the compiler warnings down :) */
struct userrec;
struct maskrec;
struct igrec;
struct flag_record;
struct list_type;
struct tand_t_struct;

#ifndef MAKING_MODS
extern void (*encrypt_pass) (char *, char *);
extern char *(*encrypt_string) (char *, char *);
extern char *(*decrypt_string) (char *, char *);
extern int (*match_noterej) (struct userrec *, char *);
extern int (*storenote)(char *from, char *to, char *msg, int idx, char *who, int bufsize);
#endif

/* botcmd.c */
void bot_share(int, char *);

/* botnet.c */
void answer_local_whom(int, int);
char *lastbot(char *);
int nextbot(char *);
int in_chain(char *);
void tell_bots(int);
void tell_bottree(int, int);
int botlink(char *, int, char *);
int botunlink(int, char *, char *);
void dump_links(int);
void addbot(char *, char *, char *, char, int);
void updatebot(int, char *, char, int);
void rembot(char *);
struct tand_t_struct *findbot(char *);
void unvia(int, struct tand_t_struct *);
void check_botnet_pings();
int partysock(char *, char *);
int addparty(char *, char *, int, char, int, char *, int *);
void remparty(char *, int);
void partystat(char *, int, int, int);
int partynick(char *, int, char *);
int partyidle(char *, char *);
void partysetidle(char *, int, int);
void partyaway(char *, int, char *);
void zapfbot(int);
void tandem_relay(int, char *, int);
int getparty(char *, int);

/* botmsg.c */
int add_note(char *, char *, char *, int, int);
void tandout_but EGG_VARARGS(int, arg1);

/* chanprog.c */
void tell_verbose_status(int);
void tell_settings(int);
int logmodes(char *);
int isowner(char *);
char *masktype(int);
char *maskname(int);
void reaffirm_owners();
void rehash();
void reload();
void chanprog();
void check_timers();
void check_utimers();
void set_chanlist(const char *host, struct userrec *rec);
void clear_chanlist(void);
void clear_chanlist_member(const char *nick);

/* cmds.c */
int check_dcc_attrs(struct userrec *, int);
int check_dcc_chanattrs(struct userrec *, char *, int, int);
int stripmodes(char *);
char *stripmasktype(int);

/* dcc.c */
void failed_link(int);
void dupwait_notify(char *);

/* dccutil.c */
void dprintf EGG_VARARGS(int, arg1);
void chatout EGG_VARARGS(char *, arg1);
extern void (*shareout) ();
extern void (*sharein) (int, char *);
void chanout_but EGG_VARARGS(int, arg1);
void dcc_chatter(int);
void lostdcc(int);
void removedcc(int);
void makepass(char *);
void tell_dcc(int);
void not_away(int);
void set_away(int, char *);
void dcc_remove_lost(void);

void flush_lines(int, struct chat_info *);
struct dcc_t *find_idx(int);
int new_dcc(struct dcc_table *, int);
void del_dcc(int);
char *add_cr(char *);
void changeover_dcc(int, struct dcc_table *, int);

/* gotdcc.c */
void gotdcc(char *, char *, struct userrec *, char *);
void do_boot(int, char *, char *);
int detect_dcc_flood(time_t *, struct chat_info *, int);

/* main.c */
void fatal(const char *, int);
void patch(const char *);
void backup_userfile(void);

/* net.c */
unsigned long iptolong(IP);
IP getmyip();
struct in6_addr getmyip6();
void neterror(char *);
void setsock(int, int);
int allocsock(int, int);
int getsock(int);
void killsock(int);
int answer(int, char *, char *, unsigned short *, int);
inline int open_listen(int *, int);
int open_address_listen(char *, int *);
int open_telnet(char *, int);
int open_telnet_dcc(int, char *, char *);
int open_telnet_raw(int, char *, int);
void tputs(int, char *, unsigned int);
void dequeue_sockets();
int sockgets(char *, int *);
void tell_netdebug(int);
char *iptostr(IP);
char *getlocaladdr(int);
int sock_has_data(int, int);
int sockoptions(int sock, int operation, int sock_options);
int flush_inbuf(int idx);

/* tcl.c */
void do_tcl(char *, char *);
int readtclprog(char *fname);
int findidx(int);
int findanyidx(int);

/* userent.c */
void list_type_kill(struct list_type *);
int xtra_set();

/* userrec.c */
struct userrec *adduser(struct userrec *, char *, char *, char *, int);
void addhost_by_handle(char *, char *);
void clear_masks(struct maskrec *);
void clear_userlist(struct userrec *);
int u_pass_match(struct userrec *, char *);
int delhost_by_handle(char *, char *);
int ishost_for_handle(char *, char *);
int count_users(struct userrec *);
int deluser(char *);
void freeuser(struct userrec *);
int change_handle(struct userrec *, char *);
void correct_handle(char *);
int write_user(struct userrec *u, FILE * f, int shr);
void write_userfile(int);
struct userrec *check_dcclist_hand(char *);
void touch_laston(struct userrec *, char *, time_t);
void user_del_chan(char *);
char *fixfrom(char *);

/* users.c */
void addignore(char *, char *, char *, time_t);
int delignore(char *);
void tell_ignores(int, char *);
int match_ignore(char *);
void check_expired_ignores();
void autolink_cycle(char *);
void tell_file_stats(int, char *);
void tell_user_ident(int, char *, int);
void tell_users_match(int, char *, int, int, int, char *);
int readuserfile(char *, struct userrec **);

#endif				/* _EGG_PROTO_H */
