/*
 * flags.h --
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
/*
 * $Id: flags.h,v 1.19 2003/12/11 00:49:11 wcc Exp $
 */

#ifndef _EGG_FLAGS_H
#define _EGG_FLAGS_H

struct flag_record {
  int match;
  int global;
  int udef_global;
  int bot;
  int chan;
  int udef_chan;
};

#define FR_GLOBAL 0x00000001
#define FR_BOT    0x00000002
#define FR_CHAN   0x00000004
#define FR_OR     0x40000000
#define FR_AND    0x20000000
#define FR_ANYWH  0x10000000
#define FR_ALL    0x0fffffff

/*
 * userflags:
 *   abc?efgh?j??mnop???t?vwx??
 * + user defined A-Z
 *   unused letters: dillqrsuyz
 *
 * botflags:
 *   0123456789ab?????hi??l?????r????????
 *   unused letters: cdefgjkmnopqstuvwxyz
 *
 * chanflags:
 *   a???efg?????mno?????uv????
 * + user defined A-Z
 *   unused letters: bcdhijklpqrstwxyz
 */
#define USER_VALID    0x78F273   /* all USER_ flags in use */
#define CHAN_VALID    0x603071   /* all flags that can be chan specific */
#define BOT_VALID     0x68F273   /* all BOT_ flags in use */


#define USER_AUTOOP    0x00000001
#define USER_BOT       0x00000002
#define USER_C         0x00000004
#define USER_D         0x00000008
#define USER_EXEMPT    0x00000010
#define USER_FRIEND    0x00000020
#define USER_GVOICE    0x00000040
#define USER_H         0x00000080
#define USER_I         0x00000100
#define USER_JANITOR   0x00000200
#define USER_K         0x00000400
#define USER_L         0x00000800
#define USER_MASTER    0x00001000
#define USER_OWNER     0x00002000
#define USER_OP        0x00004000
#define USER_PARTY     0x00008000
#define USER_Q         0x00010000
#define USER_R         0x00020000
#define USER_S         0x00040000
#define USER_BOTMAST   0x00080000
#define USER_U         0x00100000
#define USER_VOICE     0x00200000
#define USER_WASOPTEST 0x00400000
#define USER_XFER      0x00800000
#define USER_Y         0x01000000
#define USER_Z         0x02000000
#define USER_DEFAULT   0x40000000

/* Flags specifically for bots
 */
#define BOT_ALT       0x00000001	/* a  auto-link here if all +h's
					      fail			 */
#define BOT_BOT       0x00000002	/* b  sanity bot flag		 */
#define BOT_C         0x00000004	/* c  unused			 */
#define BOT_D         0x00000008	/* d  unused			 */
#define BOT_E         0x00000010	/* e  unused			 */
#define BOT_F         0x00000020	/* f  unused			 */
#define BOT_G         0x00000040	/* g  unused			 */
#define BOT_HUB       0x00000080	/* h  auto-link to ONE of these
					      bots			 */
#define BOT_ISOLATE   0x00000100	/* i  isolate party line from
					      botnet			 */
#define BOT_J         0x00000200	/* j  unused			 */
#define BOT_K         0x00000400	/* k  unused			 */
#define BOT_LEAF      0x00000800	/* l  may not link other bots	 */
#define BOT_M         0x00001000	/* m  unused			 */
#define BOT_N         0x00002000	/* n  unused			 */
#define BOT_O         0x00004000	/* o  unused			 */
#define BOT_P         0x00008000	/* p  unused			 */
#define BOT_Q         0x00010000	/* q  unused			 */
#define BOT_REJECT    0x00020000	/* r  automatically reject
					      anywhere			 */
#define BOT_S         0x00040000	/* s  unused		 	 */
#define BOT_T         0x00080000	/* t  unused			 */
#define BOT_U         0x00100000	/* u  unused			 */
#define BOT_V         0x00200000	/* v  unused			 */
#define BOT_W         0x00400000	/* w  unused			 */
#define BOT_X         0x00800000	/* x  unused			 */
#define BOT_Y         0x01000000	/* y  unused			 */
#define BOT_Z         0x02000000	/* z  unused			 */
#define BOT_FLAG0     0x00200000	/* 0  user-defined flag #0	 */
#define BOT_FLAG1     0x00400000	/* 1  user-defined flag #1	 */
#define BOT_FLAG2     0x00800000	/* 2  user-defined flag #2	 */
#define BOT_FLAG3     0x01000000	/* 3  user-defined flag #3	 */
#define BOT_FLAG4     0x02000000	/* 4  user-defined flag #4	 */
#define BOT_FLAG5     0x04000000	/* 5  user-defined flag #5	 */
#define BOT_FLAG6     0x08000000	/* 6  user-defined flag #6	 */
#define BOT_FLAG7     0x10000000	/* 7  user-defined flag #7	 */
#define BOT_FLAG8     0x20000000	/* 8  user-defined flag #8	 */
#define BOT_FLAG9     0x40000000	/* 9  user-defined flag #9	 */


/* Flag checking macros
 */
#define chan_op(x)		((x).chan & USER_OP)
#define glob_op(x)		((x).global & USER_OP)
#define glob_master(x)		((x).global & USER_MASTER)
#define glob_bot(x)		((x).global & USER_BOT)
#define glob_owner(x)		((x).global & USER_OWNER)
#define chan_master(x)		((x).chan & USER_MASTER)
#define chan_owner(x)		((x).chan & USER_OWNER)
#define chan_autoop(x)		((x).chan & USER_AUTOOP)
#define glob_autoop(x)		((x).global & USER_AUTOOP)
#define chan_gvoice(x)		((x).chan & USER_GVOICE)
#define glob_gvoice(x)		((x).global & USER_GVOICE)
#define chan_voice(x)		((x).chan & USER_VOICE)
#define glob_voice(x)		((x).global & USER_VOICE)
#define chan_wasoptest(x)	((x).chan & USER_WASOPTEST)
#define glob_wasoptest(x)	((x).global & USER_WASOPTEST)
#define chan_friend(x)		((x).chan & USER_FRIEND)
#define glob_friend(x)		((x).global & USER_FRIEND)
#define glob_botmast(x)		((x).global & USER_BOTMAST)
#define glob_party(x)		((x).global & USER_PARTY)
#define glob_xfer(x)		((x).global & USER_XFER)
#define chan_exempt(x)		((x).chan & USER_EXEMPT)
#define glob_exempt(x)		((x).global & USER_EXEMPT)


#ifndef MAKING_MODS

void get_user_flagrec(struct userrec *, struct flag_record *, const char *);
void set_user_flagrec(struct userrec *, struct flag_record *, const char *);
void break_down_flags(const char *, struct flag_record *, struct flag_record *);
int build_flags(char *, struct flag_record *, struct flag_record *);
int flagrec_eq(struct flag_record *, struct flag_record *);
int flagrec_ok(struct flag_record *, struct flag_record *);
int sanity_check(int);
int chan_sanity_check(int, int);
char geticon(struct userrec *);

#endif				/* !MAKING_MODS */

#endif				/* !_EGG_FLAGS_H */
