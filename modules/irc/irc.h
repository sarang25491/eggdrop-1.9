/*
 * irc.h -- part of irc.mod
 *
 * $Id: irc.h,v 1.5 2002/05/05 15:21:30 wingman Exp $
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

#ifndef _EGG_MOD_IRC_IRC_H
#define _EGG_MOD_IRC_IRC_H

#define NORMAL 0
#define QUICK  1

#define check_tcl_join(a,b,c,d) check_tcl_joinspltrejn(a,b,c,d,BT_join)
#define check_tcl_splt(a,b,c,d) check_tcl_joinspltrejn(a,b,c,d,BT_split)
#define check_tcl_rejn(a,b,c,d) check_tcl_joinspltrejn(a,b,c,d,BT_rejoin)
#define check_tcl_sign(a,b,c,d,e) check_tcl_signtopcnick(a,b,c,d,e,BT_quit)
#define check_tcl_topc(a,b,c,d,e) check_tcl_signtopcnick(a,b,c,d,e,BT_topic)
#define check_tcl_nick(a,b,c,d,e) check_tcl_signtopcnick(a,b,c,d,e,BT_nick)
#define check_tcl_mode(a,b,c,d,e,f) check_tcl_kickmode(a,b,c,d,e,f,BT_mode)
#define check_tcl_kick(a,b,c,d,e,f) check_tcl_kickmode(a,b,c,d,e,f,BT_kick)

#define REVENGE_KICK 1		/* Kicked victim	*/
#define REVENGE_DEOP 2		/* Took op		*/

#define fixcolon(x)             do {                                    \
	if ((x)[0] == ':')                                              \
		(x)++;                                                  \
	else                                                            \
		(x) = newsplit(&(x));                                   \
} while (0)

#ifdef MAKING_IRC
static void check_tcl_need(char *, char *);
static void check_tcl_kickmode(char *, char *, struct userrec *, char *,
			       char *, char *, bind_table_t *);
static void check_tcl_joinspltrejn(char *, char *, struct userrec *, char *,
			       bind_table_t *);
static void check_tcl_part(char *, char *, struct userrec *, char *, char *);
static void check_tcl_signtopcnick(char *, char *, struct userrec *u, char *,
				   char *, bind_table_t *);
static void check_tcl_pubm(char *, char *, char *, char *);
static int check_tcl_pub(char *, char *, char *, char *);
static int me_op(struct chanset_t *);
static int any_ops(struct chanset_t *);
static int hand_on_chan(struct chanset_t *, struct userrec *);
static char *getchanmode(struct chanset_t *);
static void flush_mode(struct chanset_t *, int);

/* reset(bans|exempts|invites) are now just macros that call resetmasks
 * in order to reduce the code duplication. <cybah>
 */
#define resetbans(chan)	    resetmasks((chan), (chan)->channel.ban,	\
				       (chan)->bans, global_bans, 'b')
#define resetexempts(chan)  resetmasks((chan), (chan)->channel.exempt,	\
				       (chan)->exempts, global_exempts, 'e')
#define resetinvites(chan)  resetmasks((chan), (chan)->channel.invite,	\
				       (chan)->invites, global_invites, 'I')

static void reset_chan_info(struct chanset_t *);
static void recheck_channel(struct chanset_t *, int);
static void set_key(struct chanset_t *, char *);
static void maybe_revenge(struct chanset_t *, char *, char *, int);
static int detect_chan_flood(char *, char *, char *, struct chanset_t *, int,
			     char *);
static void newmask(masklist *, char *, char *);
static char *quickban(struct chanset_t *, char *);
static void got_op(struct chanset_t *chan, char *nick, char *from, char *who,
 		   struct userrec *opu, struct flag_record *opper);
static int killmember(struct chanset_t *chan, char *nick);
static void check_lonely_channel(struct chanset_t *chan);
static int gotmode(char *, char *, char *);

#define newban(chan, mask, who)         newmask((chan)->channel.ban, mask, who)
#define newexempt(chan, mask, who)      newmask((chan)->channel.exempt, mask, \
						who)
#define newinvite(chan, mask, who)      newmask((chan)->channel.invite, mask, \
						who)

#else

/* 4-7 */
#define recheck_channel ((void(*)(struct chanset_t *,int))irc_funcs[4])
#define me_op ((int(*)(struct chanset_t *))irc_funcs[5])
#define recheck_channel_modes ((void(*)(struct chanset_t *))irc_funcs[6])
#define do_channel_part ((void(*)(struct chanset_t *))irc_funcs[7])

/* 8-11 */
#define check_this_ban ((void(*)(struct chanset_t *,char *,int))irc_funcs[8])
#define check_this_user ((void(*)(char *hand))irc_funcs[8])

#endif				/* MAKING_IRC */

#endif				/* _EGG_MOD_IRC_IRC_H */
