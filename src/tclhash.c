/*
 * tclhash.c --
 *
 *	bind and unbind
 *	checking and triggering the various in-bot bindings
 *	listing current bindings
 *	adding/removing new binding tables
 *	(non-Tcl) procedure lookups for msg/dcc/file commands
 *	(Tcl) binding internal procedures to msg/dcc/file commands
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

#ifndef lint
static const char rcsid[] = "$Id: tclhash.c,v 1.70 2002/05/26 03:02:58 stdarg Exp $";
#endif

#include "main.h"
#include "chan.h"
#include "users.h"
#include "logfile.h"
#include "cmdt.h"		/* cmd_t			*/
#include "userrec.h"		/* touch_laston			*/
#include "match.h"		/* wild_match_per		*/
#include "tclhash.h"		/* prototypes			*/
#include "egg_timer.h"

extern Tcl_Interp	*interp;
extern struct dcc_t	*dcc;
extern struct userrec	*userlist;
extern int		 dcc_total;
extern time_t		 now;

/* Our bind tables. */
static bind_table_t *bind_table_list_head = NULL;
static bind_table_t *BT_link, *BT_disc, *BT_away, *BT_dcc;
static bind_table_t *BT_chat, *BT_act, *BT_bcst, *BT_note;
static bind_table_t *BT_bot, *BT_nkch, *BT_chon, *BT_chof;
static bind_table_t *BT_chpt, *BT_chjn, *BT_filt;

/* Variables to control garbage collection. */
static int check_bind_executing = 0;
static int already_scheduled = 0;
static void bind_table_really_del(bind_table_t *table);
static void bind_entry_really_del(bind_table_t *table, bind_entry_t *entry);

static char *global_filt_string = NULL; /* String for FILT binds to modify. */
static script_linked_var_t tclhash_script_vars[] = {
	{"", "filt_string", &global_filt_string, SCRIPT_STRING, NULL},
	{0}
};

/* Prototypes for the commands we create in this file. */
static int script_bind(char *table_name, char *flags, char *mask, script_callback_t *callback);
static int script_unbind(char *table_name, char *flags, char *mask, char *name);
static int script_rebind(char *table_name, char *flags, char *mask, char *command, char *newflags, char *newmask);
static int script_binds(script_var_t *retval, char *tablename);

static script_command_t tclhash_script_cmds[] = {
	{"", "binds", script_binds, NULL, 0, "s", "?bind-table?", 0, SCRIPT_PASS_RETVAL | SCRIPT_VAR_ARGS},
	{"", "bind", script_bind, NULL, 4, "sssc", "table flags mask command", SCRIPT_INTEGER, 0},
	{"", "unbind", script_unbind, NULL, 4, "ssss", "table flags mask command", SCRIPT_INTEGER, 0},
	{"", "rebind", script_rebind, NULL, 6, "ssssss", "table flags mask command newflags newmask", SCRIPT_INTEGER, 0},
	{0}
};

extern cmd_t C_dcc[];

void binds_init(void)
{
	bind_table_list_head = NULL;
	global_filt_string = strdup("");
	script_link_vars(tclhash_script_vars);
	script_create_commands(tclhash_script_cmds);
	BT_link = bind_table_add("link", 2, "ss", MATCH_MASK, BIND_STACKABLE);
	BT_nkch = bind_table_add("nkch", 2, "ss", MATCH_MASK, BIND_STACKABLE);
	BT_disc = bind_table_add("disc", 1, "s", MATCH_MASK, BIND_STACKABLE);
	BT_away = bind_table_add("away", 3, "sis", MATCH_MASK, BIND_STACKABLE);
	BT_dcc = bind_table_add("dcc", 3, "Uis", MATCH_PARTIAL, BIND_USE_ATTR);
	add_builtins("dcc", C_dcc);
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

static int internal_bind_cleanup()
{
	bind_table_t *table, *next_table;
	bind_entry_t *entry, *next_entry;

	for (table = bind_table_list_head; table; table = next_table) {
		next_table = table->next;
		if (table->flags & BIND_DELETED) {
			bind_table_really_del(table);
			continue;
		}
		for (entry = table->entries; entry; entry = next_entry) {
			next_entry = entry->next;
			if (entry->flags & BIND_DELETED) bind_entry_really_del(table, entry);
		}
	}
	already_scheduled = 0;
	return(0);
}

static void schedule_bind_cleanup()
{
	egg_timeval_t when;

	if (already_scheduled) return;
	already_scheduled = 1;

	when.sec = 0;
	when.usec = 0;
	timer_create(&when, internal_bind_cleanup);
}

void kill_binds(void)
{
	while (bind_table_list_head) bind_table_del(bind_table_list_head);
}

bind_table_t *bind_table_add(const char *name, int nargs, const char *syntax, int match_type, int flags)
{
	bind_table_t *table;

	for (table = bind_table_list_head; table; table = table->next) {
		if (!strcmp(table->name, name)) return(table);
	}
	/* Nope, we have to create a new one. */
	table = (bind_table_t *)calloc(1, sizeof(*table));
	table->name = strdup(name);
	table->nargs = nargs;
	table->syntax = strdup(syntax);
	table->match_type = match_type;
	table->flags = flags;
	table->next = bind_table_list_head;
	bind_table_list_head = table;
	return(table);
}

void bind_table_del(bind_table_t *table)
{
	bind_table_t *cur, *prev;

	for (prev = NULL, cur = bind_table_list_head; cur; prev = cur, cur = cur->next) {
		if (!strcmp(table->name, cur->name)) break;
	}

	/* If it's found, remove it from the list. */
	if (cur) {
		if (prev) prev->next = cur->next;
		else bind_table_list_head = cur->next;
	}

	/* Now delete it. */
	if (check_bind_executing) {
		table->flags |= BIND_DELETED;
		schedule_bind_cleanup();
	}
	else {
		bind_table_really_del(table);
	}
}

static void bind_table_really_del(bind_table_t *table)
{
	bind_entry_t *entry, *next;

	free(table->name);
	for (entry = table->entries; entry; entry = next) {
		next = entry->next;
		free(entry->function_name);
		free(entry->mask);
		free(entry);
	}
	free(table);
}

bind_table_t *bind_table_lookup(const char *name)
{
	bind_table_t *table;

	for (table = bind_table_list_head; table; table = table->next) {
		if (!(table->flags & BIND_DELETED) && !strcmp(table->name, name)) break;
	}
	return(table);
}

/* Look up a bind entry based on either function name or id. */
bind_entry_t *bind_entry_lookup(bind_table_t *table, int id, const char *mask, const char *function_name)
{
	bind_entry_t *entry;

	for (entry = table->entries; entry; entry = entry->next) {
		if (entry->flags & BIND_DELETED) continue;
		if (entry->id == id || (!strcmp(entry->mask, mask) && !strcmp(entry->function_name, function_name))) break;
	}
	return(entry);
}

int bind_entry_del(bind_table_t *table, int id, const char *mask, const char *function_name, void *cdata)
{
	bind_entry_t *entry;

	entry = bind_entry_lookup(table, id, mask, function_name);
	if (!entry) return(-1);

	if (cdata) *(void **)cdata = entry->client_data;

	/* Delete it. */
	if (check_bind_executing) {
		entry->flags |= BIND_DELETED;
		schedule_bind_cleanup();
	}
	else bind_entry_really_del(table, entry);
	return(0);
}

static void bind_entry_really_del(bind_table_t *table, bind_entry_t *entry)
{
	if (entry->next) entry->next->prev = entry->prev;
	if (entry->prev) entry->prev->next = entry->next;
	else table->entries = entry->next;
	free(entry->function_name);
	free(entry->mask);
	memset(entry, 0, sizeof(*entry));
	free(entry);
}

/* Modify a bind entry's flags and mask. */
int bind_entry_modify(bind_table_t *table, int id, const char *mask, const char *function_name, const char *newflags, const char *newmask)
{
	bind_entry_t *entry;

	entry = bind_entry_lookup(table, id, mask, function_name);
	if (!entry) return(-1);

	/* Modify it. */
	free(entry->mask);
	entry->mask = strdup(newmask);
	entry->user_flags.match = FR_GLOBAL | FR_CHAN;
	break_down_flags(newflags, &(entry->user_flags), NULL);

	return(0);
}

int bind_entry_add(bind_table_t *table, const char *flags, const char *mask, const char *function_name, int bind_flags, Function callback, void *client_data)
{
	bind_entry_t *entry, *old_entry;

	old_entry = bind_entry_lookup(table, -1, mask, function_name);

	if (old_entry) {
		if (table->flags & BIND_STACKABLE) {
			entry = (bind_entry_t *)calloc(1, sizeof(*entry));
			entry->prev = old_entry;
			entry->next = old_entry->next;
			old_entry->next = entry;
			if (entry->next) entry->next->prev = entry;
		}
		else {
			entry = old_entry;
			free(entry->function_name);
			free(entry->mask);
		}
	}
	else {
		for (old_entry = table->entries; old_entry && old_entry->next; old_entry = old_entry->next) {
			; /* empty loop */
		}
		entry = (bind_entry_t *)calloc(1, sizeof(*entry));
		if (old_entry) old_entry->next = entry;
		else table->entries = entry;
		entry->prev = old_entry;
	}

	entry->mask = strdup(mask);
	entry->function_name = strdup(function_name);
	entry->callback = callback;
	entry->client_data = client_data;
	entry->flags = bind_flags;

	entry->user_flags.match = FR_GLOBAL | FR_CHAN;
	break_down_flags(flags, &(entry->user_flags), NULL);

	return(0);
}

static int script_bind(char *table_name, char *flags, char *mask, script_callback_t *callback)
{
	bind_table_t *table;
	int retval;

	table = bind_table_lookup(table_name);
	if (!table) return(1);

	callback->syntax = strdup(table->syntax);
	retval = bind_entry_add(table, flags, mask, callback->name, BIND_WANTS_CD, callback->callback, callback);
	return(retval);
}

static int script_unbind(char *table_name, char *flags, char *mask, char *name)
{
	bind_table_t *table;
	script_callback_t *callback;
	int retval;

	table = bind_table_lookup(table_name);
	if (!table) return(1);

	retval = bind_entry_del(table, -1, mask, name, &callback);
	if (callback) callback->del(callback);
	return(retval);
}

static int script_rebind(char *table_name, char *flags, char *mask, char *command, char *newflags, char *newmask)
{
	bind_table_t *table;

	table = bind_table_lookup(table_name);
	if (!table) return(-1);
	return bind_entry_modify(table, -1, mask, command, newflags, newmask);
}

int findanyidx(register int z)
{
  register int j;

  for (j = 0; j < dcc_total; j++)
    if (dcc[j].type && dcc[j].sock == z)
      return j;
  return -1;
}

/* Returns a list of binds in a given table, or the list of bind tables. */
static int script_binds(script_var_t *retval, char *tablename)
{
	bind_table_t *table;
	bind_entry_t *entry;
	script_var_t *sublist, *func, *flags, *mask, *nhits;
	char flagbuf[128];

	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
	retval->len = 0;
	retval->value = NULL;

	/* No table name? Then return the list of tables. */
	if (!tablename) {
		for (table = bind_table_list_head; table; table = table->next) {
			if (table->flags & BIND_DELETED) return(0);
			script_list_append(retval, script_string(table->name, -1));
		}
		return(0);
	}

	table = bind_table_lookup(tablename);
	if (!table) return(0);

	for (entry = table->entries; entry; entry = entry->next) {
		if (entry->flags & BIND_DELETED) continue;

		mask = script_string(entry->mask, -1);
		build_flags(flagbuf, &entry->user_flags, NULL);
		flags = script_copy_string(flagbuf, -1);
		nhits = script_int(entry->nhits);
		func = script_string(entry->function_name, -1);
		sublist = script_list(4, flags, mask, nhits, func);
		script_list_append(retval, sublist);
	}
	return(0);
}

/* Execute a bind entry with the given argument list. */
static int bind_entry_exec(bind_table_t *table, bind_entry_t *entry, void **al)
{
	bind_entry_t *prev;

	/* Give this entry a hit. */
	entry->nhits++;

	/* Search for the last entry that isn't deleted. */
	for (prev = entry->prev; prev; prev = prev->prev) {
		if (!(prev->flags & BIND_DELETED) && (prev->nhits >= entry->nhits)) break;
	}

	/* See if this entry is more popular than the preceding one. */
	if (entry->prev != prev) {
		/* Remove entry. */
		if (entry->prev) entry->prev->next = entry->next;
		else table->entries = entry->next;
		if (entry->next) entry->next->prev = entry->prev;

		/* Re-add in correct position. */
		if (prev) {
			entry->next = prev->next;
			if (prev->next) prev->next->prev = entry;
			prev->next = entry;
		}
		else {
			entry->next = table->entries;
			table->entries = entry;
		}
		entry->prev = prev;
		if (entry->next) entry->next->prev = entry;
	}

	/* Does the callback want client data? */
	if (entry->flags & BIND_WANTS_CD) {
		*al = entry->client_data;
	}
	else al++;

	return entry->callback(al[0], al[1], al[2], al[3], al[4], al[5], al[6], al[7], al[8], al[9]);
}

int check_bind(bind_table_t *table, const char *match, struct flag_record *flags, ...)
{
	void *args[11];
	bind_entry_t *entry, *next;
	int i, cmp, retval;
	va_list ap;

	check_bind_executing++;

	va_start(ap, flags);
	for (i = 1; i <= table->nargs; i++) {
		args[i] = va_arg(ap, void *);
	}
	va_end(ap);

	/* Default return value is 0 */
	retval = 0;

	/* If it's a partial bind, we have to find the closest match. */
	if (table->match_type & MATCH_PARTIAL) {
		int matchlen, masklen, tie;
		bind_entry_t *winner;

		matchlen = strlen(match);
		tie = 0;
		winner = NULL;
		for (entry = table->entries; entry; entry = entry->next) {
			if (entry->flags & BIND_DELETED) continue;
			masklen = strlen(entry->mask);
			if (!strncasecmp(match, entry->mask, masklen < matchlen ? masklen : matchlen)) {
				winner = entry;
				if (masklen == matchlen) break;
				else if (tie) return(-1);
				else tie = 1;
			}
		}
		if (winner) retval = bind_entry_exec(table, winner, args);
		else retval = -1;
		check_bind_executing--;
		return(retval);
	}

	for (entry = table->entries; entry; entry = next) {
		next = entry->next;
		if (entry->flags & BIND_DELETED) continue;

		if (table->match_type & MATCH_MASK) {
			cmp = !wild_match_per((unsigned char *)entry->mask, (unsigned char *)match);
		}
		else {
			if (table->match_type & MATCH_CASE) cmp = strcmp(entry->mask, match);
			else cmp = strcasecmp(entry->mask, match);
		}
		if (cmp) continue; /* Doesn't match. */

		/* Check flags. */
		if (table->flags & BIND_USE_ATTR) {
			if (table->flags & BIND_STRICT_ATTR) cmp = flagrec_eq(&entry->user_flags, flags);
			else cmp = flagrec_ok(&entry->user_flags, flags);
			if (!cmp) continue;
		}

		retval = bind_entry_exec(table, entry, args);
		if ((table->flags & BIND_BREAKABLE) && (retval & BIND_RET_BREAK)) {
			check_bind_executing--;
			return(retval);
		}
	}
	check_bind_executing--;
	return(retval);
}

/* Check for tcl-bound dcc command, return 1 if found
 * dcc: proc-name <handle> <sock> <text>
 */
void check_tcl_dcc(const char *cmd, int idx, const char *text)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  int x;

  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);

  x = check_bind(BT_dcc, cmd, &fr, dcc[idx].user, idx, text);
  if (x & BIND_RET_LOG) {
    putlog(LOG_CMDS, "*", "#%s# %s %s", dcc[idx].nick, cmd, text);
  }
}

void check_tcl_bot(const char *nick, const char *code, const char *param)
{
  check_bind(BT_bot, code, NULL, nick, code, param);
}

void check_tcl_chon(char *hand, int idx)
{
  struct flag_record	 fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  struct userrec	*u;

  u = get_user_by_handle(userlist, hand);
  touch_laston(u, "partyline", now);
  get_user_flagrec(u, &fr, NULL);
  check_bind(BT_chon, hand, &fr, hand, idx);
}

void check_tcl_chof(char *hand, int idx)
{
  struct flag_record	 fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  struct userrec	*u;

  u = get_user_by_handle(userlist, hand);
  touch_laston(u, "partyline", now);
  get_user_flagrec(u, &fr, NULL);
  check_bind(BT_chof, hand, &fr, hand, idx);
}

int check_tcl_chat(const char *from, int chan, const char *text)
{
  return check_bind(BT_chat, text, NULL, from, chan, text);
}

void check_tcl_act(const char *from, int chan, const char *text)
{
  check_bind(BT_act, text, NULL, from, chan, text);
}

void check_tcl_bcst(const char *from, int chan, const char *text)
{
  check_bind(BT_bcst, text, NULL, from, chan, text);
}

void check_tcl_nkch(const char *ohand, const char *nhand)
{
  check_bind(BT_nkch, ohand, NULL, ohand, nhand);
}

void check_tcl_link(const char *bot, const char *via)
{
  check_bind(BT_link, bot, NULL, bot, via);
}

void check_tcl_disc(const char *bot)
{
  check_bind(BT_disc, bot, NULL, bot);
}

const char *check_tcl_filt(int idx, const char *text)
{
  int			x;
  struct flag_record	fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);
  global_filt_string = strdup(text);
  x = check_bind(BT_filt, text, &fr, idx, text);
  return(global_filt_string);
}

int check_tcl_note(const char *from, const char *to, const char *text)
{
  return check_bind(BT_note, to, NULL, from, to, text);
}

void check_tcl_listen(const char *cmd, int idx)
{
  char	s[11];
  int	x;

  snprintf(s, sizeof s, "%d", idx);
  Tcl_SetVar(interp, "_n", (char *) s, 0);
  x = Tcl_VarEval(interp, cmd, " $_n", NULL);
  if (x == TCL_ERROR)
    putlog(LOG_MISC, "*", "error on listen: %s", interp->result);
}

void check_tcl_chjn(const char *bot, const char *nick, int chan,
		    const char type, int sock, const char *host)
{
  struct flag_record	fr = {FR_GLOBAL, 0, 0, 0, 0, 0};
  char			s[11], t[2];

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

void check_tcl_chpt(const char *bot, const char *hand, int sock, int chan)
{
  char	v[11];

  snprintf(v, sizeof v, "%d", chan);
  check_bind(BT_chpt, v, NULL, bot, hand, sock, chan);
}

void check_tcl_away(const char *bot, int idx, const char *msg)
{
  check_bind(BT_away, bot, NULL, bot, idx, msg);
}

void add_builtins(const char *table_name, cmd_t *cmds)
{
	char name[50];
	bind_table_t *table;

	table = bind_table_lookup(table_name);
	if (!table) return;

	for (; cmds->name; cmds++) {
		snprintf(name, 50, "*%s:%s", table->name, cmds->funcname ? cmds->funcname : cmds->name);
		bind_entry_add(table, cmds->flags, cmds->name, name, 0, cmds->func, NULL);
	}
}

void rem_builtins(const char *table_name, cmd_t *cmds)
{
	char name[50];
	bind_table_t *table;

	table = bind_table_lookup(table_name);
	if (!table) return;

	for (; cmds->name; cmds++) {
		sprintf(name, "*%s:%s", table->name, cmds->funcname ? cmds->funcname : cmds->name);
		bind_entry_del(table, -1, cmds->name, name, NULL);
	}
}
