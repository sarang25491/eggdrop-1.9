/*
 * module.h --
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
 * $Id: module.h,v 1.27 2002/06/18 06:12:31 guppy Exp $
 */

#ifndef _EGG_MOD_MODULE_H
#define _EGG_MOD_MODULE_H

/* FIXME: remove this ugliness ASAP! */
#define MAKING_MODS

#include "src/main.h"		/* NOTE: when removing this, include config.h */

/* FIXME: filter out files which are not needed (it was easier after removing
   proto.h and cleaning up main.h to just include all .h files here w/o
   validating if they aren't needed */
#include "src/bg.h"
#include "src/botcmd.h"
#include "src/botmsg.h"
#include "src/botnet.h"
#include "src/chan.h"
#include "src/chanprog.h"
#include "src/cmds.h"
#include "src/cmdt.h"
#include "src/core_binds.h"
#include "src/dcc.h"
#include "src/dccutil.h"
#include "src/debug.h"
#include "src/dns.h"
#include "src/egg.h"
#include "src/egg_timer.h"
#include "src/flags.h"
#include "src/irccmp.h"
#include "src/logfile.h"
#include "src/match.h"
#include "src/md5.h"
#include "src/misc.h"
#include "src/modules.h"
#include "src/net.h"
#include "src/tandem.h"
#include "src/tclegg.h"
#include "src/tcl.h"
#include "src/tclhash.h"
#include "src/traffic.h"
#include "src/userent.h"
#include "src/userrec.h"
#include "src/users.h"
#include "src/tandem.h"
#include "src/logfile.h"
#include "src/dns.h"

/*
 * This file contains all the orrible stuff required to do the lookup
 * table for symbols, rather than getting the OS to do it, since most
 * OS's require all symbols resolved, this can cause a problem with
 * some modules.
 *
 * This is intimately related to the table in `modules.c'. Don't change
 * the files unless you have flamable underwear.
 *
 * Do not read this file whilst unless heavily sedated, I will not be
 * held responsible for mental break-downs caused by this file <G>
 */

/* #undef feof */
#undef dprintf

#if defined (__CYGWIN__) 
#  define EXPORT_SCOPE	__declspec(dllexport)
#else
#  define EXPORT_SCOPE
#endif

/* Redefine for module-relevance */

/* 0 - 3 */
/* 0: nmalloc -- UNUSED (Tothwolf) */
/* 1: nfree -- UNUSED (Tothwolf) */
#define module_rename ((int (*)(char *, char *))egg->global[3])
/* 4 - 7 */
#define module_register ((int (*)(char *, Function *, int, int))egg->global[4])
#define module_find ((module_entry * (*)(char *,int,int))egg->global[5])
#define module_depend ((Function *(*)(char *,char *,int,int))egg->global[6])
#define module_undepend ((int(*)(char *))egg->global[7])
/* 8 - 11 */
/* #define add_bind_table ((p_tcl_bind_list(*)(const char *,int,Function))egg->global[8]) */
/* #define del_bind_table ((void (*) (p_tcl_bind_list))egg->global[9]) */
/* #define find_bind_table ((p_tcl_bind_list(*)(const char *))egg->global[10]) */
/* #define check_bind_bind ((int (*) (p_tcl_bind_list,const char *,struct flag_record *,const char *, int))egg->global[11]) */
/* 12 - 15 */
/* #define add_builtins ((int (*) (p_tcl_bind_list, cmd_t *))egg->global[12]) */
/* #define rem_builtins ((int (*) (p_tcl_bind_list, cmd_t *))egg->global[13]) */
#define add_tcl_commands ((void (*) (tcl_cmds *))egg->global[14])
#define rem_tcl_commands ((void (*) (tcl_cmds *))egg->global[15])
/* 16 - 19 */
#define add_tcl_ints ((void (*) (tcl_ints *))egg->global[16])
#define rem_tcl_ints ((void (*) (tcl_ints *))egg->global[17])
#define add_tcl_strings ((void (*) (tcl_strings *))egg->global[18])
#define rem_tcl_strings ((void (*) (tcl_strings *))egg->global[19])
/* 20 - 23 */
/* #define base64_to_int ((int (*) (char *))egg->global[20]) */
/* #define int_to_base64 ((char * (*) (int))egg->global[21]) */
/* #define int_to_base10 ((char * (*) (int))egg->global[22]) */
/* #define simple_sprintf ((int (*)())egg->global[23]) */
/* 24 - 27 */
#define botnet_send_zapf ((void (*)(int, char *, char *, char *))egg->global[24])
#define botnet_send_zapf_broad ((void (*)(int, char *, char *, char *))egg->global[25])
#define botnet_send_unlinked ((void (*)(int, char *, char *))egg->global[26])
#define botnet_send_bye ((void(*)(void))egg->global[27])
/* 28 - 31 */
#define botnet_send_chat ((void(*)(int,char*,char*))egg->global[28])
#define botnet_send_filereject ((void(*)(int,char*,char*,char*))egg->global[29])
#define botnet_send_filesend ((void(*)(int,char*,char*,char*))egg->global[30])
#define botnet_send_filereq ((void(*)(int,char*,char*,char*))egg->global[31])
/* 32 - 35 */
#define botnet_send_join_idx ((void(*)(int,int))egg->global[32])
#define botnet_send_part_idx ((void(*)(int,char *))egg->global[33])
#define updatebot ((void(*)(int,char*,char,int))egg->global[34])
#define nextbot ((int (*)(char *))egg->global[35])
/* 36 - 39 */
#define zapfbot ((void (*)(int))egg->global[36])
/* 37: n_free -- UNUSED (Tothwolf) */
#define u_pass_match ((int (*)(struct userrec *,char *))egg->global[38])
/* 39: user_malloc -- UNUSED (Tothwolf) */
/* 40 - 43 */
#define get_user ((void *(*)(struct user_entry_type *,struct userrec *))egg->global[40])
#define set_user ((int(*)(struct user_entry_type *,struct userrec *,void *))egg->global[41])
#define add_entry_type ((int (*) ( struct user_entry_type * ))egg->global[42])
#define del_entry_type ((int (*) ( struct user_entry_type * ))egg->global[43])
/* 44 - 47 */
#define get_user_flagrec ((void (*)(struct userrec *, struct flag_record *, const char *))egg->global[44])
#define set_user_flagrec ((void (*)(struct userrec *, struct flag_record *, const char *))egg->global[45])
#define get_user_by_host ((struct userrec * (*)(char *))egg->global[46])
#define get_user_by_handle ((struct userrec *(*)(struct userrec *,char *))egg->global[47])
/* 48 - 51 */
#define find_entry_type ((struct user_entry_type * (*) ( char * ))egg->global[48])
#define find_user_entry ((struct user_entry * (*)( struct user_entry_type *, struct userrec *))egg->global[49])
#define adduser ((struct userrec *(*)(struct userrec *,char*,char*,char*,int))egg->global[50])
#define deluser ((int (*)(char *))egg->global[51])
/* 52 - 55 */
#define addhost_by_handle ((void (*) (char *, char *))egg->global[52])
#define delhost_by_handle ((int(*)(char *,char *))egg->global[53])
#define readuserfile ((int (*)(char *,struct userrec **))egg->global[54])
#define write_userfile ((void(*)(int))egg->global[55])
/* 56 - 59 */
#define geticon ((char (*) (struct userrec *))egg->global[56])
#define clear_chanlist ((void (*)(void))egg->global[57])
#define reaffirm_owners ((void (*)(void))egg->global[58])
#define change_handle ((int(*)(struct userrec *,char*))egg->global[59])
/* 60 - 63 */
#define write_user ((int (*)(struct userrec *, FILE *,int))egg->global[60])
#define clear_userlist ((void (*)(struct userrec *))egg->global[61])
#define count_users ((int(*)(struct userrec *))egg->global[62])
#define sanity_check ((int(*)(int))egg->global[63])
/* 64 - 67 */
#define break_down_flags ((void (*)(const char *,struct flag_record *,struct flag_record *))egg->global[64])
#define build_flags ((void (*)(char *, struct flag_record *, struct flag_record *))egg->global[65])
#define flagrec_eq ((int(*)(struct flag_record*,struct flag_record *))egg->global[66])
#define flagrec_ok ((int(*)(struct flag_record*,struct flag_record *))egg->global[67])
/* 68 - 71 */
#define shareout (*(Function *)(egg->global[68]))
#define dprintf (egg->global[69])
#define chatout (egg->global[70])
#define chanout_but ((void(*)())egg->global[71])
/* 72 - 75 */
/* #define check_validity ((int (*) (char *,Function))egg->global[72]) */
#define list_delete ((int (*)( struct list_type **, struct list_type *))egg->global[73])
#define list_append ((int (*) ( struct list_type **, struct list_type *))egg->global[74])
#define list_contains ((int (*) (struct list_type *, struct list_type *))egg->global[75])
/* 76 - 79 */
#define answer ((int (*) (int,char *,char *,unsigned short *,int))egg->global[76])
#define getmyip ((IP (*) (void))egg->global[77])
#define neterror ((void (*) (char *))egg->global[78])
#define tputs ((void (*) (int, char *,unsigned int))egg->global[79])
/* 80 - 83 */
#define new_dcc ((int (*) (struct dcc_table *, int))egg->global[80])
#define lostdcc ((void (*) (int))egg->global[81])
#define getsock ((int (*) (int))egg->global[82])
#define killsock ((void (*) (int))egg->global[83])
/* 84 - 87 */
#define open_listen ((int (*) (int *,int))egg->global[84])
#define open_telnet_dcc ((int (*) (int,char *,char *))egg->global[85])
/* 86: get_data_ptr -- UNUSED (Tothwolf) */
#define open_telnet ((int (*) (char *, int))egg->global[87])
/* 88 - 91 */
#define check_bind_event ((void * (*) (const char *))egg->global[88])
/* #define memcpy ((void * (*) (void *, const void *, size_t))egg->global[89]) */
/* #define my_atoul ((IP(*)(char *))egg->global[90]) */
/* #define my_strcpy ((int (*)(char *, const char *))egg->global[91]) */
/* 92 - 95 */
#define dcc (*(struct dcc_t **)egg->global[92])
#define chanset (*(struct chanset_t **)(egg->global[93]))
#define userlist (*(struct userrec **)egg->global[94])
#define lastuser (*(struct userrec **)(egg->global[95]))
/* 96 - 99 */
#define global_bans (*(maskrec **)(egg->global[96]))
#define global_ign (*(struct igrec **)(egg->global[97]))
#define password_timeout (*(int *)(egg->global[98]))
#define share_greet (*(int *)egg->global[99])
/* 100 - 103 */
#define max_dcc (*(int *)egg->global[100])
/* #define require_p (*(int *)egg->global[101]) */
#define ignore_time (*(int *)(egg->global[102]))
/* #define use_console_r (*(int *)(egg->global[103])) */
/* 104 - 107 */
#define reserved_port_min (*(int *)(egg->global[104]))
#define reserved_port_max (*(int *)(egg->global[105]))
#define debug_output (*(int *)(egg->global[106]))
#define noshare (*(int *)(egg->global[107]))
/* 108 - 111 */
/* 108: gban_total -- UNUSED (Eule) */
#define make_userfile (*(int*)egg->global[109])
#define default_flags (*(int*)egg->global[110])
#define dcc_total (*(int*)egg->global[111])
/* 112 - 115 */
#define tempdir ((char *)(egg->global[112]))
#define natip ((char *)(egg->global[113]))
#define learn_users (*(int *)egg->global[114])
#define origbotname ((char *)(egg->global[115]))
/* 116 - 119 */
#define botuser ((char *)(egg->global[116]))
#define admin ((char *)(egg->global[117]))
#define userfile ((char *)egg->global[118])
#define ver ((char *)egg->global[119])
/* 120 - 123 */
#define notify_new ((char *)egg->global[120])
#define helpdir ((char *)egg->global[121])
#define Version ((char *)egg->global[122])
#define botnetnick ((char *)egg->global[123])
/* 124 - 127 */
#define DCC_CHAT_PASS (*(struct dcc_table *)(egg->global[124]))
#define DCC_BOT (*(struct dcc_table *)(egg->global[125]))
#define DCC_LOST (*(struct dcc_table *)(egg->global[126]))
#define DCC_CHAT (*(struct dcc_table *)(egg->global[127]))
/* 128 - 131 */
#define interp (*(Tcl_Interp **)(egg->global[128]))
#define now (*(time_t*)egg->global[129])
#define findanyidx ((int (*)(int))egg->global[130])
#define findchan ((struct chanset_t *(*)(char *))egg->global[131])
/* 132 - 135 */
#define cmd_die (egg->global[132])
#define days ((void (*)(time_t,time_t,char *))egg->global[133])
#define daysago ((void (*)(time_t,time_t,char *))egg->global[134])
#define daysdur ((void (*)(time_t,time_t,char *))egg->global[135])
/* 136 - 139 */
#define ismember ((memberlist * (*) (struct chanset_t *, char *))egg->global[136])
/* #define newsplit ((char *(*)(char **))egg->global[137]) */
/* 138: splitnick -- UNUSED (Tothwolf) */
/* #define splitc ((void (*)(char *,char *,char))egg->global[139]) */
/* 140 - 143 */
#define addignore ((void (*) (char *, char *, char *,time_t))egg->global[140])
#define match_ignore ((int (*)(char *))egg->global[141])
#define delignore ((int (*)(char *))egg->global[142])
#define fatal (egg->global[143])
/* 144 - 147 */
#define xtra_kill ((void (*)(struct user_entry *))egg->global[144])
#define xtra_unpack ((void (*)(struct userrec *, struct user_entry *))egg->global[145])
/* #define movefile ((int (*) (char *, char *))egg->global[146]) */
/* #define copyfile ((int (*) (char *, char *))egg->global[147]) */
/* 148 - 151 */
#define do_tcl ((void (*)(char *, char *))egg->global[148])
#define readtclprog ((int (*)(const char *))egg->global[149])
/* 150: get_language() -- UNUSED */
#define def_get ((void *(*)(struct userrec *, struct user_entry *))egg->global[151])
/* 152 - 155 */
#define makepass ((void (*) (char *))egg->global[152])
#define wild_match ((int (*)(const char *, const char *))egg->global[153])
#define maskhost ((void (*)(const char *, char *))egg->global[154])
#define show_motd ((void(*)(int))egg->global[155])
/* 156 - 159 */
#define tellhelp ((void(*)(int, char *, struct flag_record *, int))egg->global[156])
#define showhelp ((void(*)(char *, char *, struct flag_record *, int))egg->global[157])
#define add_help_reference ((void(*)(char *))egg->global[158])
#define rem_help_reference ((void(*)(char *))egg->global[159])
/* 160 - 163 */
#define touch_laston ((void (*)(struct userrec *,char *,time_t))egg->global[160])
#define add_mode ((void (*)(struct chanset_t *,char,char,char *))(*(Function**)(egg->global[161])))
/* #define rmspace ((void (*)(char *))egg->global[162]) */
#define in_chain ((int (*)(char *))egg->global[163])
/* 164 - 167 */
#define add_note ((int (*)(char *,char*,char*,int,int))egg->global[164])
/* 165: del_lang_section() -- UNUSED */
#define detect_dcc_flood ((int (*) (time_t *,struct chat_info *,int))egg->global[166])
#define flush_lines ((void(*)(int,struct chat_info*))egg->global[167])
/* 168 - 171 */
/* 168: expected_memory -- UNUSED (Tothwolf) */
/* 169: tell_mem_status -- UNUSED (Tothwolf) */
#define do_restart (*(int *)(egg->global[170]))
#define check_bind_filt ((const char *(*)(int, const char *))egg->global[171])
/* 172 - 175 */
#define add_hook(a,b) (((void (*) (int, Function))egg->global[172])(a,b))
#define del_hook(a,b) (((void (*) (int, Function))egg->global[173])(a,b))
/* 174: H_dcc -- UNUSED (stdarg) */
/* 175: H_filt -- UNUSED (oskar) */
/* 176 - 179 */
/* 176: H_chon -- UNUSED (oskar) */
/* 177: H_chof -- UNUSED (oskar) */
/*#define H_load (*(p_tcl_bind_list *)(egg->global[178])) */
/*#define H_unld (*(p_tcl_bind_list *)(egg->global[179])) */
/* 180 - 183 */
/* 180: H_chat -- UNUSED (oskar) */
/* 181: H_act -- UNUSED (oskar) */
/* 182: H_bcst -- UNUSED (oskar) */
/* 183: H_bot -- UNUSED (oskar) */
/* 184 - 187 */
/* 184: H_link -- UNUSED (oskar) */
/* 185: H_disc -- UNUSED (oskar) */
/* 186: H_away -- UNUSED (oskar) */
/* 187: H_nkch -- UNUSED (oskar) */
/* 188 - 191 */
#define USERENTRY_BOTADDR (*(struct user_entry_type *)(egg->global[188]))
#define USERENTRY_BOTFL (*(struct user_entry_type *)(egg->global[189]))
#define USERENTRY_HOSTS (*(struct user_entry_type *)(egg->global[190]))
#define USERENTRY_PASS (*(struct user_entry_type *)(egg->global[191]))
/* 192 - 195 */
#define USERENTRY_XTRA (*(struct user_entry_type *)(egg->global[192]))
#define user_del_chan ((void(*)(char *))(egg->global[193]))
#define USERENTRY_INFO (*(struct user_entry_type *)(egg->global[194]))
#define USERENTRY_COMMENT (*(struct user_entry_type *)(egg->global[195]))
/* 196 - 199 */
#define USERENTRY_LASTON (*(struct user_entry_type *)(egg->global[196]))
#define putlog (egg->global[197])
#define botnet_send_chan ((void(*)(int,char*,char*,int,char*))egg->global[198])
#define list_type_kill ((void(*)(struct list_type *))egg->global[199])
/* 200 - 203 */
#define logmodes ((int(*)(char *))egg->global[200])
#define masktype ((const char *(*)(int))egg->global[201])
/* #define stripmodes ((int(*)(char *))egg->global[202]) */
/* #define stripmasktype ((const char *(*)(int))egg->global[203]) */
/* 204 - 207 */
#define sub_lang ((void(*)(int,char *))egg->global[204])
#define online_since (*(int *)(egg->global[205]))
/* 206: cmd_loadlanguage() -- UNUSED */
#define check_dcc_attrs ((int (*)(struct userrec *,int))egg->global[207])
/* 208 - 211 */
#define check_dcc_chanattrs ((int (*)(struct userrec *,char *,int,int))egg->global[208])
#define add_tcl_coups ((void (*) (tcl_coups *))egg->global[209])
#define rem_tcl_coups ((void (*) (tcl_coups *))egg->global[210])
#define botname (*(char **)(egg->global[211]))
/* 212 - 215 */
/* 212: remove_gunk() -- UNUSED (drummer) */
#define check_bind_chjn ((void (*) (const char *,const char *,int,char,int,const char *))egg->global[213])
/* 214: sanitycheck_dcc() -- UNUSED (guppy) */
#define isowner ((int (*)(char *))egg->global[215])
/* 216 - 219 */
/* 216: min_dcc_port -- UNUSED (guppy) */
/* 217: max_dcc_port -- UNUSED (guppy) */
#define irccmp ((int(*)(char *, char *))(*(Function**)(egg->global[218])))
#define ircncmp ((int(*)(char *, char *, int *))(*(Function**)(egg->global[219])))
/* 220 - 223 */
#define global_exempts (*(maskrec **)(egg->global[220]))
#define global_invites (*(maskrec **)(egg->global[221]))
/* 222: ginvite_total -- UNUSED (Eule) */
/* 223: gexempt_total -- UNUSED (Eule) */
/* 224 - 227 */
/* 224: H_event -- UNUSED (stdarg) */
#define use_exempts (*(int *)(egg->global[225]))	/* drummer/Jason */
#define use_invites (*(int *)(egg->global[226]))	/* drummer/Jason */
#define force_expire (*(int *)(egg->global[227]))	/* Rufus */
/* 228 - 231 */
/* 228: add_lang_section() -- UNUSED */
/* 229: user_realloc -- UNUSED (Tothwolf) */
/* 230: nrealloc -- UNUSED (Tothwolf) */
#define xtra_set ((int(*)(struct userrec *,struct user_entry *, void *))egg->global[231])
/* 232 - 235 */
/* #define ContextNote(note) (egg->global[232](__FILE__, __LINE__, MODULE_NAME, note)) */
/* 233: Assert -- UNUSED (Tothwolf) */
#define allocsock ((int(*)(int sock,int options))egg->global[234])
#define call_hostbyip ((void(*)(char *, char *, int))egg->global[235])
/* 236 - 239 */
#define call_ipbyhost ((void(*)(char *, char *, int))egg->global[236])
#define iptostr ((char *(*)(IP))egg->global[237])
#define DCC_DNSWAIT (*(struct dcc_table *)(egg->global[238]))
/* 239: hostsanitycheck_dcc() -- UNUSED (guppy) */
/* 240 - 243 */
#define dcc_dnsipbyhost ((void (*)(char *))egg->global[240])
#define dcc_dnshostbyip ((void (*)(char *))egg->global[241])
#define changeover_dcc ((void (*)(int, struct dcc_table *, int))egg->global[242])
#define make_rand_str ((void (*) (char *, int))egg->global[243])
/* 244 - 247 */
#define protect_readonly (*(int *)(egg->global[244]))
#define findchan_by_dname ((struct chanset_t *(*)(char *))egg->global[245])
/* #define removedcc ((void (*) (int))egg->global[246]) */
#define userfile_perm (*(int *)egg->global[247])
/* 248 - 251 */
#define sock_has_data ((int(*)(int, int))egg->global[248])
#define bots_in_subtree ((int (*)(tand_t *))egg->global[249])
#define users_in_subtree ((int (*)(tand_t *))egg->global[250])
/* #define inet_aton ((int (*)(const char *cp, struct in_addr *addr))egg->global[251]) */
/* 252 - 255 */
/* #define snprintf (egg->global[252]) */
/* #define vsnprintf ((int (*)(char *, size_t, const char *, va_list))egg->global[253]) */
/* #define memset ((void *(*)(void *, int, size_t))egg->global[254]) */
/* #define strcasecmp ((int (*)(const char *, const char *))egg->global[255]) */
/* 256 - 259 */
#define fixfrom ((char *(*)(char *))egg->global[256])
/* #define is_file ((int (*)(const char *))egg->global[257]) */
/* #define must_be_owner (*(int *)(egg->global[258])) */
#define tandbot (*(tand_t **)(egg->global[259]))
/* 260 - 263 */
#define party (*(party_t **)(egg->global[260]))
#define open_address_listen ((int (*)(char *addr, int *port))egg->global[261])
/* #define str_escape ((char *(*)(const char *, const char, const char))egg->global[262]) */
/* #define strchr_unescape ((char *(*)(char *, const char, register const char))egg->global[263]) */
/* 264 - 267 */
/* #define str_unescape ((void (*)(char *, register const char))egg->global[264]) */
/* #define egg_strcatn ((int (*)(char *dst, const char *src, size_t max))egg->global[265]) */
#define clear_chanlist_member ((void (*)(const char *nick))egg->global[266])
/* 268 - 271 */
/* Please don't modify socklist directly, unless there's no other way.
 * Its structure might be changed, or it might be completely removed,
 * so you can't rely on it without a version-check.
 */
#define socklist (*(struct sock_list **)egg->global[268])
#define sockoptions ((int (*)(int, int, int))egg->global[269])
#define flush_inbuf ((int (*)(int))egg->global[270])
#ifdef IPV6
#  define getmyip6 ((struct in6_addr (*) (void))egg->global[271])
#endif
/* 272 - 275 */
#define getlocaladdr ((char* (*) (int))egg->global[272])
#define kill_bot ((void (*)(char *, char *))egg->global[273])
#define quit_msg ((char *)(egg->global[274]))
#define bind_table_add ((bind_table_t *(*)(const char *, int, char *, int, int))egg->global[275])
/* 276 - 279 */
#define bind_table_del ((void (*)(bind_table_t *))egg->global[276])
#define add_builtins ((void (*)(const char *, cmd_t *))egg->global[277])
#define rem_builtins ((void (*)(const char *, cmd_t *))egg->global[278])
#define bind_table_lookup ((bind_table_t *(*)(const char *))egg->global[279]) 
/* 280 - 283 */
#define check_bind ((int (*)(bind_table_t *, const char *, struct flag_record *, ...))egg->global[280])
#define egg_timeval_now (*(egg_timeval_t *)egg->global[281])

/* This is for blowfish module, couldnt be bothered making a whole new .h
 * file for it ;)
 */
#ifndef MAKING_ENCRYPTION

#  define encrypt_string(a, b)						\
	(((char *(*)(char *,char*))encryption_funcs[4])(a,b))
#  define decrypt_string(a, b)						\
	(((char *(*)(char *,char*))encryption_funcs[5])(a,b))
#endif

#endif				/* !_EGG_MOD_MODULE_H */
