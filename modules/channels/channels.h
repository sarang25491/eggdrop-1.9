/*
 * channels.h --
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
/*
 * $Id: channels.h,v 1.9 2002/10/10 05:50:11 wcc Exp $
 */

#ifndef _EGG_MOD_CHANNELS_CHANNELS_H
#define _EGG_MOD_CHANNELS_CHANNELS_H

/* User defined chanmodes/settings */
#define UDEF_FLAG 1
#define UDEF_INT 2
#define UDEF_STR 3

#define MASKREASON_MAX	307	/* Max length of ban/invite/exempt/etc.
				   reasons.				*/
#define MASKREASON_LEN	(MASKREASON_MAX + 1)


#ifdef MAKING_CHANNELS

/* Structure for udef channel values. Udef setting have one such
 * structure for each channel where they have a defined value.
 */
struct udef_chans {
  struct udef_chans *next;	/* Ptr to next value.			*/
  char *chan;			/* Dname of channel name.		*/
  int value;			/* Actual value.			*/
};

/* Structure for user defined channel settings.
 */
struct udef_struct {
  struct udef_struct *next;	/* Ptr to next setting.			*/
  char *name;			/* Name of setting.			*/
  int defined;			/* Boolean that specifies whether this
				   flag was defined by, e.g. a Tcl
				   script yet.				*/
  int type;			/* Type of setting: UDEF_FLAG, UDEF_INT	*/
  struct udef_chans *values;	/* Ptr to linked list of udef channel
				   structures.				*/
};

static void del_chanrec(struct userrec *u, char *);
static struct chanuserrec *get_chanrec(struct userrec *u, char *chname);
static struct chanuserrec *add_chanrec(struct userrec *u, char *chname);
static void add_chanrec_by_handle(struct userrec *bu, char *hand, char *chname);
static void get_handle_chaninfo(char *handle, char *chname, char *s);
static void set_handle_chaninfo(struct userrec *bu, char *handle,
				char *chname, char *info);
static void set_handle_laston(char *chan, struct userrec *u, time_t n);
static int u_sticky_mask(maskrec *u, char *uhost);
static int u_setsticky_mask(int type, struct chanset_t *chan, char *uhost,
			    int sticky);

static int u_equals_mask(maskrec *u, char *uhost);
static int u_match_mask(struct maskrec *rec, char *mask);
static int u_delmask(char type, struct chanset_t *c, char *who, int doit);
static int u_addmask(char type, struct chanset_t *chan, char *who, char *from,
		     char *note, time_t expire_time, int flags);
static void tell_bans(int idx, int show_inact, char *match);
static int write_bans(FILE * f, int idx);
static void check_expired_bans(void);
static void tell_exempts (int idx, int show_inact, char * match);
static int write_exempts (FILE * f, int idx);
static void check_expired_exempts(void);
static void tell_invites (int idx, int show_inact, char * match);
static int write_invites (FILE * f, int idx);
static void check_expired_invites(void);
static void write_channels(void);
static void read_channels(int);
static void clear_channel(struct chanset_t *, int);
static void get_mode_protect(struct chanset_t *chan, char *s);
static void set_mode_protect(struct chanset_t *chan, char *set);
static int ismasked(masklist *m, char *user);
static int ismodeline(masklist *m, char *user);
static int tcl_channel_modify(Tcl_Interp * irp, struct chanset_t *chan,
			      int items, char **item);
static int tcl_channel_add(Tcl_Interp * irp, char *, char *);
static char *convert_element(char *src, char *dst);
static void free_udef(struct udef_struct *);
static void free_udef_chans(struct udef_chans *, int);
static int getudef(struct udef_chans *, char *);
static void initudef(int type, char *, int);
static void setudef(struct udef_struct *, char *, int);
static void remove_channel(struct chanset_t *);
static int ngetudef(char *, char *);
static int expired_mask(struct chanset_t *chan, char *who);
inline static int chanset_unlink(struct chanset_t *chan);

#else

/* 4 - 7 */
#define u_setsticky_mask ((int (*)(int, struct chanset_t *, char *, int))channels_funcs[4])
#define u_delmask ((int (*)(char, struct chanset_t *, char *, int))channels_funcs[5])
#define u_addmask ((int (*)(char, struct chanset_t *, char *, char *, char *, time_t, int))channels_funcs[6])
#define write_bans ((int (*)(FILE *, int))channels_funcs[7])
/* 8 - 11 */
#define get_chanrec ((struct chanuserrec *(*)(struct userrec *, char *))channels_funcs[8])
#define add_chanrec ((struct chanuserrec *(*)(struct userrec *, char *))channels_funcs[9])
#define del_chanrec ((void (*)(struct userrec *, char *))channels_funcs[10])
#define set_handle_chaninfo ((void (*)(struct userrec *, char *, char *, char *))channels_funcs[11])
/* 12 - 15 */
/* 12: channel_malloc -- UNUSED (Tothwolf) */
#define u_match_mask ((int (*)(maskrec *, char *))channels_funcs[13])
#define u_equals_mask ((int (*)(maskrec *, char *))channels_funcs[14])
#define clear_channel ((void (*)(struct chanset_t *, int))channels_funcs[15])
/* 16 - 19 */
#define set_handle_laston ((void (*)(char *,struct userrec *,time_t))channels_funcs[16])
/* *HOLE* channels_funcs[17] */
#define use_info (*(int *)(channels_funcs[18]))
#define get_handle_chaninfo ((void (*)(char *, char *, char *))channels_funcs[19])
/* 20 - 23 */
#define u_sticky_mask ((int (*)(maskrec *, char *))channels_funcs[20])
#define ismasked ((int (*)(masklist *, char *))channels_funcs[21])
#define add_chanrec_by_handle ((void (*)(struct userrec *, char *, char *))channels_funcs[22])
/* *HOLE* channels_funcs[23] used to be isexempted() <cybah> */
/* 24 - 27 */
/* *HOLE* channels_funcs[24] */
/* *HOLE* channels_funcs[25] used to be isinvited() by arthur2 <cybah> */
/* *HOLE* channels_funcs[26] */
/* *HOLE* channels_funcs[27] used to be u_match_exempt() by arthur2 <cybah> */
/* 28 - 31 */
/* *HOLE* channels_funcs[28] used to be u_setsticky_exempt() <cybah> */
/* *HOLE* channels_funcs[29] */
/* *HOLE* channels_funcs[30] */
/* *HOLE* channels_funcs[31] used to be u_equals_exempt() <cybah> */
/* 32 - 35 */
/* *HOLE* channels_funcs[32] used to be u_sticky_exempt() <cybah> */
/* *HOLE* channels_funcs[33] used to be u_match_invite() <cybah> */
/* *HOLE* channels_funcs[34] used to be killchanset().			*/
/* *HOLE* channels_funcs[35] */
/* 36 - 39 */
/* *HOLE* channels_funcs[36] */
#define tcl_channel_add ((int (*)(Tcl_Interp *, char *, char *))channels_funcs[37])
#define tcl_channel_modify ((int (*)(Tcl_Interp *, struct chanset_t *, int, char **))channels_funcs[38])
#define write_exempts ((int (*)(FILE *, int))channels_funcs[39])
/* 40 - 43 */
#define write_invites ((int (*)(FILE *, int))channels_funcs[40])
#define ismodeline ((int(*)(masklist *, char *))channels_funcs[41])
#define initudef ((void(*)(int, char *,int))channels_funcs[42])
#define ngetudef ((int(*)(char *, char *))channels_funcs[43])
/* 44 - 47 */
#define expired_mask ((int (*)(struct chanset_t *, char *))channels_funcs[44])
#define remove_channel ((void (*)(struct chanset_t *))channels_funcs[45])
#define global_ban_time (*(int *)(channels_funcs[46]))
#define global_exempt_time (*(int *)(channels_funcs[47]))
/* 48 - 51 */
#define global_invite_time (*(int *)(channels_funcs[48]))

#endif				/* MAKING_CHANNELS */

/* Macro's here because their functions were replaced by something more
 * generic. <cybah>
 */
#define isbanned(chan, user)    ismasked((chan)->channel.ban, user)
#define isexempted(chan, user)  ismasked((chan)->channel.exempt, user)
#define isinvited(chan, user)   ismasked((chan)->channel.invite, user)

#define ischanban(chan, user)    ismodeline((chan)->channel.ban, user)
#define ischanexempt(chan, user) ismodeline((chan)->channel.exempt, user)
#define ischaninvite(chan, user) ismodeline((chan)->channel.invite, user)

#define u_setsticky_ban(chan, host, sticky)     u_setsticky_mask('b', chan, host, sticky)
#define u_setsticky_exempt(chan, host, sticky)  u_setsticky_mask('e', chan, host, sticky)
#define u_setsticky_invite(chan, host, sticky)  u_setsticky_mask('I', chan, host, sticky)

#define is_perm_exempted(chan, user) (u_match_mask(chan->exempts, user) || \
				      (u_match_mask(global_exempts, user) && \
				       (chan->ircnet_status & CHAN_HONORGLOBALEXEMPTS)))

#endif				/* !_EGG_MOD_CHANNELS_CHANNELS_H */
