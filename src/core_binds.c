#include <eggdrop/eggdrop.h>
#include "main.h"
#include "users.h"
#include "logfile.h"
#include "cmdt.h"
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
	bind_check(BT_time, full, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
}

void check_bind_event(char *event)
{
	bind_check(BT_event, event, NULL, event);
}

void check_bind_dcc(const char *cmd, int idx, const char *text)
{
	int x;

	x = bind_check(BT_dcc, cmd, dcc[idx].user, idx, text);
	if (x & BIND_RET_LOG) {
		putlog(LOG_CMDS, "*", "#%s# %s %s", dcc[idx].nick, cmd, text);
	}
}

const char *check_bind_filt(int idx, const char *text)
{
	int x;

	global_filt_string = strdup(text);
	x = bind_check(BT_filt, text, idx, text);
	return(global_filt_string);
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
