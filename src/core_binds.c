/*
 * core_binds.c --
 */
/*
 * Copyright (C) 2002 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA	02111-1307, USA.
 */

#ifndef lint
static const char rcsid[] = "$Id: core_binds.c,v 1.6 2002/10/07 22:36:36 stdarg Exp $";
#endif

#include "main.h"
#include "users.h"
#include "logfile.h"
#include "cmdt.h"
#include "tclhash.h"
#include "userrec.h"

static bind_table_t *BT_time, *BT_event;

static bind_table_t *BT_link, *BT_disc, *BT_away, *BT_dcc;
static bind_table_t *BT_chat, *BT_act, *BT_bcst, *BT_note;
static bind_table_t *BT_bot, *BT_nkch, *BT_chon, *BT_chof;
static bind_table_t *BT_chpt, *BT_chjn, *BT_filt;

/* The global_filt_string is what FILT binds modify. */
static char *global_filt_string = NULL;
static script_linked_var_t core_binds_script_vars[] = {
	{"", "filt_string", &global_filt_string, SCRIPT_STRING, NULL},
	{0}
};

extern struct dcc_t *dcc;
extern struct userrec *userlist;
extern time_t now;

void core_binds_init()
{
	global_filt_string = strdup("");
	script_link_vars(core_binds_script_vars);

	BT_time = bind_table_add("time", 5, "iiiii", MATCH_MASK, BIND_STACKABLE);
	BT_event = bind_table_add("event", 1, "s", MATCH_MASK, BIND_STACKABLE);

	/* Stuff that will probably be moved to partyline/botnet. */
	BT_link = bind_table_add("link", 2, "ss", MATCH_MASK, BIND_STACKABLE);
	BT_nkch = bind_table_add("nkch", 2, "ss", MATCH_MASK, BIND_STACKABLE);
	BT_disc = bind_table_add("disc", 1, "s", MATCH_MASK, BIND_STACKABLE);
	BT_away = bind_table_add("away", 3, "sis", MATCH_MASK, BIND_STACKABLE);
	BT_dcc = bind_table_add("dcc", 3, "Uis", MATCH_PARTIAL, BIND_USE_ATTR);
	//add_builtins("dcc", C_dcc);
	BT_chat = bind_table_add("chat", 3, "sis", MATCH_MASK, BIND_STACKABLE | BIND_BREAKABLE);
	BT_act = bind_table_add("act", 3, "sis", MATCH_MASK, BIND_STACKABLE);
	BT_bcst = bind_table_add("bcst", 3, "sis", MATCH_MASK, BIND_STACKABLE);
	BT_note = bind_table_add("note", 3 , "sss", MATCH_EXACT, 0);
	BT_bot = bind_table_add("bot", 3, "sss", MATCH_EXACT, 0);
	BT_chon = bind_table_add("chon", 2, "si", MATCH_MASK, BIND_USE_ATTR | BIND_STACKABLE);
	BT_chof = bind_table_add("chof", 2, "si", MATCH_MASK, BIND_USE_ATTR | BIND_STACKABLE);
	BT_chpt = bind_table_add("chpt", 4, "ssii", MATCH_MASK, BIND_STACKABLE);
	BT_chjn = bind_table_add("chjn", 6, "ssisis", MATCH_MASK, BIND_STACKABLE);
	BT_filt = bind_table_add("filt", 2, "is", MATCH_MASK, BIND_USE_ATTR | BIND_STACKABLE);
}

void check_bind_time(struct tm *tm)
{
	char full[32];
	snprintf(full, sizeof(full), "%02d %02d %02d %02d %04d", tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
	check_bind(BT_time, full, NULL, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
}

void check_bind_event(char *event)
{
	check_bind(BT_event, event, NULL, event);
}

void check_bind_away(const char *bot, int idx, const char *msg)
{
	check_bind(BT_away, bot, NULL, bot, idx, msg);
}

void check_bind_dcc(const char *cmd, int idx, const char *text)
{
	struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
	int x;

	get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);

	x = check_bind(BT_dcc, cmd, &fr, dcc[idx].user, idx, text);
	if (x & BIND_RET_LOG) {
		putlog(LOG_CMDS, "*", "#%s# %s %s", dcc[idx].nick, cmd, text);
	}
}

void check_bind_bot(const char *nick, const char *code, const char *param)
{
	check_bind(BT_bot, code, NULL, nick, code, param);
}

void check_bind_chon(char *hand, int idx)
{
	struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
	struct userrec *u;

	u = get_user_by_handle(userlist, hand);
	touch_laston(u, "partyline", now);
	get_user_flagrec(u, &fr, NULL);
	check_bind(BT_chon, hand, &fr, hand, idx);
}

void check_bind_chof(char *hand, int idx)
{
	struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
	struct userrec *u;

	u = get_user_by_handle(userlist, hand);
	touch_laston(u, "partyline", now);
	get_user_flagrec(u, &fr, NULL);
	check_bind(BT_chof, hand, &fr, hand, idx);
}

int check_bind_chat(const char *from, int chan, const char *text)
{
	return check_bind(BT_chat, text, NULL, from, chan, text);
}

void check_bind_act(const char *from, int chan, const char *text)
{
	check_bind(BT_act, text, NULL, from, chan, text);
}

void check_bind_bcst(const char *from, int chan, const char *text)
{
	check_bind(BT_bcst, text, NULL, from, chan, text);
}

void check_bind_nkch(const char *ohand, const char *nhand)
{
	check_bind(BT_nkch, ohand, NULL, ohand, nhand);
}

void check_bind_link(const char *bot, const char *via)
{
	check_bind(BT_link, bot, NULL, bot, via);
}

void check_bind_disc(const char *bot)
{
	check_bind(BT_disc, bot, NULL, bot);
}

const char *check_bind_filt(int idx, const char *text)
{
	int x;
	struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

	get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);
	global_filt_string = strdup(text);
	x = check_bind(BT_filt, text, &fr, idx, text);
	return(global_filt_string);
}

int check_bind_note(const char *from, const char *to, const char *text)
{
	return check_bind(BT_note, to, NULL, from, to, text);
}

void check_bind_listen(const char *cmd, int idx)
{
	putlog(LOG_MISC, "*", "check_bind_listen: unsupported right now");

/*
	char s[11];
	int x;

	snprintf(s, sizeof s, "%d", idx);
	Tcl_SetVar(interp, "_n", (char *) s, 0);
	x = Tcl_VarEval(interp, cmd, " $_n", NULL);
	if (x == TCL_ERROR)
		putlog(LOG_MISC, "*", "error on listen: %s", interp->result);
*/

}

void check_bind_chjn(const char *bot, const char *nick, int chan,
				const char type, int sock, const char *host)
{
	struct flag_record fr = {FR_GLOBAL, 0, 0, 0, 0, 0};
	char s[11], t[2];

	t[0] = type;
	t[1] = 0;
	switch (type) {
	case '*':
		fr.global = USER_OWNER;
		break;
	case '+':
		fr.global = USER_MASTER;
		break;
	case '@':
		fr.global = USER_OP;
		break;
	case '%':
		fr.global = USER_BOTMAST;
	}
	snprintf(s, sizeof s, "%d", chan);
	check_bind(BT_chjn, s, &fr, bot, nick, chan, t, sock, host);
}

void check_bind_chpt(const char *bot, const char *hand, int sock, int chan)
{
	char v[11];
	
	snprintf(v, sizeof v, "%d", chan);
	check_bind(BT_chpt, v, NULL, bot, hand, sock, chan);
}	 
