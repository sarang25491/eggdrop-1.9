/*
 * tclhash.c -- handles:
 *   bind and unbind
 *   checking and triggering the various in-bot bindings
 *   listing current bindings
 *   adding/removing new binding tables
 *   (non-Tcl) procedure lookups for msg/dcc/file commands
 *   (Tcl) binding internal procedures to msg/dcc/file commands
 *
 * $Id: tclhash.c,v 1.42 2001/10/11 11:34:19 tothwolf Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001 Eggheads Development Team
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

#include "main.h"
#include "chan.h"
#include "users.h"

extern Tcl_Interp	*interp;
extern struct dcc_t	*dcc;
extern struct userrec	*userlist;
extern int		 dcc_total;
extern time_t		 now;

/* New bind table list */
static bind_table_t *bind_table_list_head = NULL;
static bind_table_t *BT_event;
static bind_table_t *BT_link;
static bind_table_t *BT_disc;
static bind_table_t *BT_away;
static bind_table_t *BT_time;
static bind_table_t *BT_dcc;

p_tcl_bind_list		bind_table_list;
p_tcl_bind_list		H_chat, H_act, H_bcst, H_chon, H_chof,
			H_load, H_unld, H_link, H_disc, H_chjn, H_chpt,
			H_bot, H_time, H_nkch, H_away, H_note, H_filt, H_event;

static int builtin_2char();
static int builtin_3char();
static int builtin_5int();
static int builtin_char();
static int builtin_chpt();
static int builtin_chjn();
static int builtin_idxchar();
static int builtin_charidx();
static int builtin_chat();
static int builtin_dcc();

/* Delete trigger/command.
 */
static inline void tcl_cmd_delete(tcl_cmd_t *tc)
{
  free(tc->func_name);
  free(tc);
}

/* Delete bind and its elements.
 */
static inline void tcl_bind_mask_delete(tcl_bind_mask_t *tm)
{
  tcl_cmd_t		*tc, *tc_next;

  for (tc = tm->first; tc; tc = tc_next) {
    tc_next = tc->next;
    tcl_cmd_delete(tc);
  }
  free(tm->mask);
  free(tm);
}

/* Delete bind list and its elements.
 */
static inline void tcl_bind_list_delete(tcl_bind_list_t *tl)
{
  tcl_bind_mask_t	*tm, *tm_next;

  for (tm = tl->first; tm; tm = tm_next) {
    tm_next = tm->next;
    tcl_bind_mask_delete(tm);
  }
  free(tl);
}

inline void garbage_collect_tclhash(void)
{
  tcl_bind_list_t	*tl, *tl_next, *tl_prev;
  tcl_bind_mask_t	*tm, *tm_next, *tm_prev;
  tcl_cmd_t		*tc, *tc_next, *tc_prev;

  for (tl = bind_table_list, tl_prev = NULL; tl; tl = tl_next) {
    tl_next = tl->next;

    if (tl->flags & HT_DELETED) {
      if (tl_prev)
	tl_prev->next = tl->next;
      else
	bind_table_list = tl->next;
      tcl_bind_list_delete(tl);
    } else {
      for (tm = tl->first, tm_prev = NULL; tm; tm = tm_next) {
	tm_next = tm->next;

	if (!(tm->flags & TBM_DELETED)) {
	  for (tc = tm->first, tc_prev = NULL; tc; tc = tc_next) {
	    tc_next = tc->next;

	    if (tc->attributes & TC_DELETED) {
	      if (tc_prev)
		tc_prev->next = tc->next;
	      else
		tm->first = tc->next;
	      tcl_cmd_delete(tc);
	    } else
	      tc_prev = tc;
	  }
	}

	/* Delete the bind when it's marked as deleted or
	   when it's empty. */
	if ((tm->flags & TBM_DELETED) || tm->first == NULL) {
	  if (tm_prev)
	    tm_prev->next = tm->next;
	  else
	    tl->first = tm_next;
	  tcl_bind_mask_delete(tm);
	} else
	  tm_prev = tm;
      }
      tl_prev = tl;
    }
  }
}


extern cmd_t C_dcc[];
static int tcl_bind();
static int tcl_bind2();
static int tcl_unbind2();

void init_bind2(void)
{
	bind_table_list_head = NULL;
	Tcl_CreateCommand(interp, "bind2", tcl_bind2, NULL, NULL);
	Tcl_CreateCommand(interp, "unbind2", tcl_unbind2, NULL, NULL);
	BT_event = add_bind_table2("event", 1, "s", MATCH_MASK, BIND_STACKABLE);
	BT_link = add_bind_table2("link", 2, "ss", MATCH_MASK, BIND_STACKABLE);
	BT_disc = add_bind_table2("disc", 1, "s", MATCH_MASK, BIND_STACKABLE);
	BT_away = add_bind_table2("away", 3, "sis", MATCH_MASK, BIND_STACKABLE);
	BT_time = add_bind_table2("time", 5, "iiiii", MATCH_MASK, BIND_STACKABLE);
	BT_dcc = add_bind_table2("dcc", 3, "Uis", MATCH_MASK, BIND_USE_ATTR);
	add_builtins2(BT_dcc, C_dcc);
}

void init_bind(void)
{
  bind_table_list = NULL;
  Context;
  Tcl_CreateCommand(interp, "bind", tcl_bind, (ClientData) 0, NULL);
  Tcl_CreateCommand(interp, "unbind", tcl_bind, (ClientData) 1, NULL);
  H_unld = add_bind_table("unld", HT_STACKABLE, builtin_char);
  H_time = add_bind_table("time", HT_STACKABLE, builtin_5int);
  H_note = add_bind_table("note", 0, builtin_3char);
  H_nkch = add_bind_table("nkch", HT_STACKABLE, builtin_2char);
  H_load = add_bind_table("load", HT_STACKABLE, builtin_char);
  H_link = add_bind_table("link", HT_STACKABLE, builtin_2char);
  H_filt = add_bind_table("filt", HT_STACKABLE, builtin_idxchar);
  H_disc = add_bind_table("disc", HT_STACKABLE, builtin_char);
  H_chpt = add_bind_table("chpt", HT_STACKABLE, builtin_chpt);
  H_chon = add_bind_table("chon", HT_STACKABLE, builtin_charidx);
  H_chof = add_bind_table("chof", HT_STACKABLE, builtin_charidx);
  H_chjn = add_bind_table("chjn", HT_STACKABLE, builtin_chjn);
  H_chat = add_bind_table("chat", HT_STACKABLE, builtin_chat);
  H_bot = add_bind_table("bot", 0, builtin_3char);
  H_bcst = add_bind_table("bcst", HT_STACKABLE, builtin_chat);
  H_away = add_bind_table("away", HT_STACKABLE, builtin_chat);
  H_act = add_bind_table("act", HT_STACKABLE, builtin_chat);
  H_event = add_bind_table("evnt", HT_STACKABLE, builtin_char);
  Context;
}

void kill_bind2(void)
{
	while (bind_table_list_head) del_bind_table2(bind_table_list_head);
}

void kill_bind(void)
{
  tcl_bind_list_t	*tl, *tl_next;

  rem_builtins2(BT_dcc, C_dcc);

  for (tl = bind_table_list; tl; tl = tl_next) {
    tl_next = tl->next;

    if (!(tl->flags |= HT_DELETED))
      putlog(LOG_DEBUG, "*", "De-Allocated bind table %s", tl->name);
    tcl_bind_list_delete(tl);
  }
  bind_table_list = NULL;
}

bind_table_t *add_bind_table2(const char *name, int nargs, const char *syntax, int match_type, int flags)
{
	bind_table_t *table;

	for (table = bind_table_list_head; table; table = table->next) {
		if (!strcmp(table->name, name)) return(table);
	}
	/* Nope, we have to create a new one. */
	table = (bind_table_t *)malloc(sizeof(*table));
	table->chains = NULL;
	malloc_strcpy(table->name, name);
	table->nargs = nargs;
	malloc_strcpy(table->syntax, syntax);
	table->match_type = match_type;
	table->flags = flags;
	table->next = bind_table_list_head;
	bind_table_list_head = table;
	return(table);
}

tcl_bind_list_t *add_bind_table(const char *nme, int flg, Function func)
{
  tcl_bind_list_t	*tl, *tl_prev;
  int			 v;

  /* Do not allow coders to use bind table names longer than
     4 characters. */
  Assert(strlen(nme) <= 4);

  for (tl = bind_table_list, tl_prev = NULL; tl; tl_prev = tl, tl = tl->next) {
    if (tl->flags & HT_DELETED)
      continue;
    v = egg_strcasecmp(tl->name, nme);
    if (!v)
      return tl;	/* Duplicate, just return old value.	*/
    if (v > 0)
      break;		/* New. Insert at start of list.	*/
  }

  tl = calloc(1, sizeof(tcl_bind_list_t));
  strcpy(tl->name, nme);
  tl->flags = flg;
  tl->func = func;

  if (tl_prev) {
    tl->next = tl_prev->next;
    tl_prev->next = tl;
  } else {
    tl->next = bind_table_list;
    bind_table_list = tl;
  }

  putlog(LOG_DEBUG, "*", "Allocated bind table %s (flags %d)", nme, flg);
  return tl;
}

void del_bind_table2(bind_table_t *table)
{
	bind_table_t *cur, *prev;
	bind_chain_t *chain, *next_chain;
	bind_entry_t *entry, *next_entry;

	for (prev = NULL, cur = bind_table_list_head; cur; prev = cur, cur = cur->next) {
		if (!strcmp(table->name, cur->name)) break;
	}

	/* If it's found, remove it from the list. */
	if (cur) {
		if (prev) prev->next = cur->next;
		else bind_table_list_head = cur->next;
	}

	/* Now delete it. */
	free(table->name);
	for (chain = table->chains; chain; chain = next_chain) {
		next_chain = chain->next;
		for (entry = chain->entries; entry; entry = next_entry) {
			next_entry = entry->next;
			free(entry->function_name);
			free(entry);
		}
		free(chain);
	}
}

void del_bind_table(tcl_bind_list_t *tl_which)
{
  tcl_bind_list_t	*tl;

  for (tl = bind_table_list; tl; tl = tl->next) {
    if (tl->flags & HT_DELETED)
      continue;
    if (tl == tl_which) {
      tl->flags |= HT_DELETED;
      putlog(LOG_DEBUG, "*", "De-Allocated bind table %s", tl->name);
      return;
    }
  }
  putlog(LOG_DEBUG, "*", "??? Tried to delete not listed bind table ???");
}

bind_table_t *find_bind_table2(const char *name)
{
	bind_table_t *table;

	for (table = bind_table_list_head; table; table = table->next) {
		if (!strcmp(table->name, name)) break;
	}
	return(table);
}

tcl_bind_list_t *find_bind_table(const char *nme)
{
  tcl_bind_list_t	*tl;
  int			 v;

  for (tl = bind_table_list; tl; tl = tl->next) {
    if (tl->flags & HT_DELETED)
      continue;
    v = egg_strcasecmp(tl->name, nme);
    if (!v)
      return tl;
    if (v > 0)
      return NULL;
  }
  return NULL;
}

int del_bind_entry(bind_table_t *table, const char *flags, const char *mask, const char *function_name)
{
	bind_chain_t *chain;
	bind_entry_t *entry, *prev;

	/* Find the correct mask entry. */
	for (chain = table->chains; chain; chain = chain->next) {
		if (!strcmp(chain->mask, mask)) break;
	}
	if (!chain) return(1);

	/* Now find the function name in this mask entry. */
	for (prev = NULL, entry = chain->entries; entry; prev = entry, entry = entry->next) {
		if (!strcmp(entry->function_name, function_name)) break;
	}
	if (!entry) return(1);

	/* Delete it. */
	if (prev) prev->next = entry->next;
	else if (entry->next) chain->entries = entry->next;
	free(entry->function_name);
	free(entry);

	return(0);
}

static void *get_bind_cdata(bind_table_t *table , const char *flags, const char *mask, const char *function_name)
{
	bind_chain_t *chain;
	bind_entry_t *entry;

	/* Find the correct mask entry. */
	for (chain = table->chains; chain; chain = chain->next) {
		if (!strcmp(chain->mask, mask)) break;
	}
	if (!chain) return(NULL);

	/* Now find the function name in this mask entry. */
	for (entry = chain->entries; entry; entry = entry->next) {
		if (!strcmp(entry->function_name, function_name)) break;
	}
	if (!entry) return(NULL);
	else return(entry->client_data);
}

static int unbind_bind_entry(tcl_bind_list_t *tl, const char *flags,
			     const char *cmd, const char *proc)
{
  tcl_bind_mask_t	*tm;

  /* Search for matching bind in bind list. */
  for (tm = tl->first; tm; tm = tm->next) {
    if (tm->flags & TBM_DELETED)
      continue;
    if (!strcmp(cmd, tm->mask))
      break;			/* Found it! fall out! */
  }

  if (tm) {
    tcl_cmd_t		*tc;

    /* Search for matching proc in bind. */
    for (tc = tm->first; tc; tc = tc->next) {
      if (tc->attributes & TC_DELETED)
	continue;
      if (!egg_strcasecmp(tc->func_name, proc)) {
	/* Erase proc regardless of flags. */
	tc->attributes |= TC_DELETED;
	return 1;		/* Match.	*/
      }
    }
  }
  return 0;			/* No match.	*/
}

int add_bind_entry(bind_table_t *table, const char *flags, const char *mask, const char *function_name, int bind_flags, Function callback, void *client_data)
{
	bind_chain_t *chain;
	bind_entry_t *entry;

	/* Find the chain (mask) first. */
	for (chain = table->chains; chain; chain = chain->next) {
		if (!strcmp(chain->mask, mask)) break;
	}

	/* Create if it doesn't exist. */
	if (!chain) {
		chain = (bind_chain_t *)malloc(sizeof(*chain));
		chain->entries = NULL;
		malloc_strcpy(chain->mask, mask);
		chain->next = table->chains;
		table->chains = chain;
	}

	/* If it's stackable */
	if (table->flags & BIND_STACKABLE) {
		/* Search for specific entry. */
		for (entry = chain->entries; entry; entry = entry->next) {
			if (!strcmp(entry->function_name, function_name)) break;
		}
	}
	else {
		/* Nope, just use first entry. */
		entry = chain->entries;
	}

	/* If we have an old entry, re-use it. */
	if (entry) free(entry->function_name);
	else {
		/* Otherwise, create a new entry. */
		entry = (bind_entry_t *)malloc(sizeof(*entry));
		entry->next = chain->entries;
		chain->entries = entry;
	}

	malloc_strcpy(entry->function_name, function_name);
	entry->callback = callback;
	entry->client_data = client_data;
	entry->hits = 0;
	entry->bind_flags = bind_flags;

	entry->user_flags.match = FR_GLOBAL | FR_CHAN;
	break_down_flags(flags, &(entry->user_flags), NULL);

	return(0);
}

/* Add command (remove old one if necessary)
 */
static int bind_bind_entry(tcl_bind_list_t *tl, const char *flags,
			   const char *cmd, const char *proc)
{
  tcl_cmd_t		*tc;
  tcl_bind_mask_t	*tm, *tm_last;

  /* Search for matching bind in bind list. */
  for (tm = tl->first, tm_last = NULL; tm; tm_last = tm, tm = tm->next) {
    if (tm->flags & TBM_DELETED)
      continue;
    if (!strcmp(cmd, tm->mask))
      break;			/* Found it! fall out! */
  }

  /* Create bind if it doesn't exist yet. */
  if (!tm) {
    tm = calloc(1, sizeof(tcl_bind_mask_t));
    malloc_strcpy(tm->mask, cmd);

#if (TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 1) || (TCL_MAJOR_VERSION >= 9)
    str_nutf8tounicode(tm->mask, strlen(tm->mask) + 1);
#endif
	
    /* Link into linked list of binds. */
    tm->next = tl->first;
    tl->first = tm;
  }

  /* Proc already defined? If so, replace. */
  for (tc = tm->first; tc; tc = tc->next) {
    if (tc->attributes & TC_DELETED)
      continue;
    if (!egg_strcasecmp(tc->func_name, proc)) {
      tc->flags.match = FR_GLOBAL | FR_CHAN;
      break_down_flags(flags, &(tc->flags), NULL);
      return 1;
    }
  }

  /* If this bind list is not stackable, remove the
     old entry from this bind. */
  if (!(tl->flags & HT_STACKABLE)) {
    for (tc = tm->first; tc; tc = tc->next) {
      if (tc->attributes & TC_DELETED)
	continue;
      /* NOTE: We assume there's only one not-yet-deleted
               entry. */
      tc->attributes |= TC_DELETED;
      break;
    }
  }

  tc = calloc(1, sizeof(tcl_cmd_t));
  tc->flags.match = FR_GLOBAL | FR_CHAN;
  break_down_flags(flags, &(tc->flags), NULL);
  malloc_strcpy(tc->func_name, proc);

  /* Link into linked list of the bind's command list. */
  tc->next = tm->first;
  tm->first = tc;

  return 1;
}

static int tcl_getbinds(tcl_bind_list_t *tl_kind, const char *name)
{
  tcl_bind_mask_t	*tm;

  for (tm = tl_kind->first; tm; tm = tm->next) {
    if (tm->flags & TBM_DELETED)
      continue;
    if (!egg_strcasecmp(tm->mask, name)) {
      tcl_cmd_t		*tc;

      for (tc = tm->first; tc; tc = tc->next) {
	if (tc->attributes & TC_DELETED)
	  continue;
	Tcl_AppendElement(interp, tc->func_name);
      }
      break;
    }
  }
  return TCL_OK;
}

/* Works with string, int, and user type. */
static int my_tcl_bind_callback(tcl_cmd_cdata *cdata, ...)
{
	Tcl_DString final;
	int *arg;
	char *syntax, *str, buf[32];
	int retval;

	arg = (int *)&cdata;
	arg++;
	Tcl_DStringInit(&final);
	Tcl_DStringAppend(&final, cdata->cmd, -1);
	for (syntax = cdata->syntax; *syntax; syntax++) {
		switch (*syntax) {
			case 's':
				str = (char *)(*arg);
				break;
			case 'i': {
				sprintf(buf, "%d", *arg);
				str = buf;
				break;
			}
			case 'U': {
				struct userrec *u;
				u = (struct userrec *)(*arg);
				if (u) str = u->handle;
				else str = "*";
				break;
			}
			default:
				str = "(unsupported argument type)";
		}
		if (!str) str = "(null)";
		Tcl_DStringAppendElement(&final, str);
		arg++;
	}
	Tcl_Eval(cdata->irp, Tcl_DStringValue(&final));
	Tcl_DStringGetResult(cdata->irp, &final);
	retval = atoi(Tcl_DStringValue(&final));
	Tcl_DStringFree(&final);
	return(retval);
}

static int tcl_bind2 STDVAR
{
	tcl_cmd_cdata *cdata;
	bind_table_t *table;

	BADARGS(5, 5, " type flags cmd/mask procname");

	table = find_bind_table2(argv[1]);
	if (!table) {
		Tcl_AppendResult(irp, "invalid table type", NULL);
		return(TCL_ERROR);
	}

	cdata = (tcl_cmd_cdata *)malloc(sizeof(*cdata));
	cdata->irp = irp;
	malloc_strcpy(cdata->syntax, table->syntax);
	malloc_strcpy(cdata->cmd, argv[4]);
	add_bind_entry(table, argv[2], argv[3], argv[4], BIND_WANTS_CD, (Function) my_tcl_bind_callback, cdata);
	Tcl_AppendResult(irp, "moooo", NULL);
	return(TCL_OK);
}

static int tcl_unbind2 STDVAR
{
	bind_table_t *table;
	tcl_cmd_cdata *cdata;

	BADARGS(5, 5, " type flags cmd/mask procname");
	table = find_bind_table2(argv[1]);
	if (!table) {
		Tcl_AppendResult(irp, "invalid table type", NULL);
		return(TCL_ERROR);
	}
	cdata = get_bind_cdata(table, argv[2], argv[3], argv[4]);
	if (cdata) {
		free(cdata->cmd);
		free(cdata->syntax);
		free(cdata);
		del_bind_entry(table, argv[2], argv[3], argv[4]);
	}
	Tcl_AppendResult(irp, "mooooo", NULL);
	return(TCL_OK);
}

static int tcl_bind STDVAR
{
  tcl_bind_list_t	*tl;
  bind_table_t *table;

  /* Note: `cd' defines what tcl_bind is supposed do: 0 stands for
           bind and 1 stands for unbind. */
  if ((long int) cd == 1)
    BADARGS(5, 5, " type flags cmd/mask procname");
  else
    BADARGS(4, 5, " type flags cmd/mask ?procname?");

  table = find_bind_table2(argv[1]);
  if (table) {
    if ((int) cd == 0) return tcl_bind2(cd, irp, argc, argv);
    else return tcl_unbind2(cd, irp, argc, argv);
  }
  tl = find_bind_table(argv[1]);
  if (!tl) {
    Tcl_AppendResult(irp, "bad table type", NULL);
    return TCL_OK;
  }

  if ((long int) cd == 1) {
    if (!unbind_bind_entry(tl, argv[2], argv[3], argv[4])) {
      /* Don't error if trying to re-unbind a builtin */
      if (argv[4][0] != '*' || argv[4][4] != ':' ||
	  strcmp(argv[3], &argv[4][5]) || strncmp(argv[1], &argv[4][1], 3)) {
	Tcl_AppendResult(irp, "no such binding", NULL);
	return TCL_ERROR;
      }
    }
  } else {
    if (argc == 4)
      return tcl_getbinds(tl, argv[3]);
    bind_bind_entry(tl, argv[2], argv[3], argv[4]);
  }
  Tcl_AppendResult(irp, argv[3], NULL);
  return TCL_OK;
}

int check_validity(char *nme, Function func)
{
  char			*p;
  tcl_bind_list_t	*tl;

  if (*nme != '*')
    return 0;
  p = strchr(nme + 1, ':');
  if (p == NULL)
    return 0;
  *p = 0;
  tl = find_bind_table(nme + 1);
  *p = ':';
  if (!tl)
    return 0;
  if (tl->func != func)
    return 0;
  return 1;
}

static int builtin_3char STDVAR
{
  Function F = (Function) cd;

  BADARGS(4, 4, " from to args");
  CHECKVALIDITY(builtin_3char);
  F(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

static int builtin_2char STDVAR
{
  Function F = (Function) cd;

  BADARGS(3, 3, " nick msg");
  CHECKVALIDITY(builtin_2char);
  F(argv[1], argv[2]);
  return TCL_OK;
}

static int builtin_5int STDVAR
{
  Function F = (Function) cd;

  BADARGS(6, 6, " min hrs dom mon year");
  CHECKVALIDITY(builtin_5int);
  F(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
  return TCL_OK;
}

static int builtin_char STDVAR
{
  Function F = (Function) cd;

  BADARGS(2, 2, " handle");
  CHECKVALIDITY(builtin_char);
  F(argv[1]);
  return TCL_OK;
}

static int builtin_chpt STDVAR
{
  Function F = (Function) cd;

  BADARGS(3, 3, " bot nick sock");
  CHECKVALIDITY(builtin_chpt);
  F(argv[1], argv[2], atoi(argv[3]));
  return TCL_OK;
}

static int builtin_chjn STDVAR
{
  Function F = (Function) cd;

  BADARGS(6, 6, " bot nick chan# flag&sock host");
  CHECKVALIDITY(builtin_chjn);
  F(argv[1], argv[2], atoi(argv[3]), argv[4][0],
    argv[4][0] ? atoi(argv[4] + 1) : 0, argv[5]);
  return TCL_OK;
}

static int builtin_idxchar STDVAR
{
  Function F = (Function) cd;
  int idx;
  char *r;

  BADARGS(3, 3, " idx args");
  CHECKVALIDITY(builtin_idxchar);
  idx = findidx(atoi(argv[1]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  r = (((char *(*)()) F) (idx, argv[2]));

  Tcl_ResetResult(irp);
  Tcl_AppendResult(irp, r, NULL);
  return TCL_OK;
}

int findanyidx(register int z)
{
  register int j;

  for (j = 0; j < dcc_total; j++)
    if (dcc[j].sock == z)
      return j;
  return -1;
}

static int builtin_charidx STDVAR
{
  Function F = (Function) cd;
  int idx;

  BADARGS(3, 3, " handle idx");
  CHECKVALIDITY(builtin_charidx);
  idx = findanyidx(atoi(argv[2]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, int_to_base10(F(argv[1], idx)), NULL);
  return TCL_OK;
}

static int builtin_chat STDVAR
{
  Function F = (Function) cd;
  int ch;

  BADARGS(4, 4, " handle idx text");
  CHECKVALIDITY(builtin_chat);
  ch = atoi(argv[2]);
  F(argv[1], ch, argv[3]);
  return TCL_OK;
}

static int builtin_dcc STDVAR
{
  int idx;
  Function F = (Function) cd;

  BADARGS(4, 4, " hand idx param");
  idx = findidx(atoi(argv[2]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (F == NULL) {
    Tcl_AppendResult(irp, "break", NULL);
    return TCL_OK;
  }
  /* Check if it's a password change, if so, don't show the password. We
   * don't need pretty formats here, as it's only for debugging purposes.
   */
  debug4("tcl: builtin dcc call: %s %s %s %s", argv[0], argv[1], argv[2],
	 (!strcmp(argv[0] + 5, "newpass") ||
	  !strcmp(argv[0] + 5, "chpass")) ? "[something]" : argv[3]);
  (F) (dcc[idx].user, idx, argv[3]);
  Tcl_ResetResult(irp);
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

/* trigger (execute) a proc */
static int trigger_bind(const char *proc, const char *param)
{
  int x;

  /* We now try to debug the Tcl_VarEval() call below by remembering both
   * the called proc name and it's parameters. This should render us a bit
   * less helpless when we see context dumps.
   */
  {
    const char *msg = "TCL proc: %s, param: %s";
    char *buf;

    Context;
    buf = malloc(strlen(msg) + (proc ? strlen(proc) : 6)
		  + (param ? strlen(param) : 6) + 1);
    sprintf(buf, msg, proc ? proc : "<null>", param ? param : "<null>");
    ContextNote(buf);
    free(buf);
  }
  x = Tcl_VarEval(interp, proc, param, NULL);
  Context;
  if (x == TCL_ERROR) {
    if (strlen(interp->result) > 400)
      interp->result[400] = 0;
    putlog(LOG_MISC, "*", "TCL error [%s]: %s", proc, interp->result);
    return BIND_EXECUTED;
  } else {
    if (!strcmp(interp->result, "break"))
      return BIND_EXEC_BRK;
    return (atoi(interp->result) > 0) ? BIND_EXEC_LOG : BIND_EXECUTED;
  }
}

int check_bind(bind_table_t *table, const char *match, struct flag_record *_flags, ...)
{
	int *al; /* Argument list */
	struct flag_record *flags;
	bind_chain_t *chain;
	bind_entry_t *entry;
	int len, cmp, r, hits;
	Function cb;

	/* Experimental way to not use va_list... */
	flags = _flags;
	al = (int *)&_flags;

	/* Save the length for strncmp */
	len = strlen(match);

	/* Keep track of how many binds execute (or would) */
	hits = 0;

	/* Default return value is 0 */
	r = 0;

	/* For each chain in the table... */
	for (chain = table->chains; chain; chain = chain->next) {
		/* Test to see if it matches. */
		if (table->match_type & MATCH_PARTIAL) {
			if (table->match_type & MATCH_CASE) cmp = strncmp(match, chain->mask, len);
			else cmp = egg_strncasecmp(match, chain->mask, len);
		}
		else if (table->match_type & MATCH_MASK) {
			cmp = !wild_match_per((unsigned char *)chain->mask, (unsigned char *)match);
		}
		else {
			if (table->match_type & MATCH_CASE) cmp = strcmp(match, chain->mask);
			else cmp = egg_strcasecmp(match, chain->mask);
		}
		if (cmp) continue; /* Doesn't match. */

		/* Ok, now call the entries for this chain. */
		/* If it's not stackable, There Can Be Only One. */
		for (entry = chain->entries; entry; entry = entry->next) {
			/* Check flags. */
			if (table->match_type & BIND_USE_ATTR) {
				if (table->match_type & BIND_STRICT_ATTR) cmp = flagrec_eq(&entry->user_flags, flags);
				else cmp = flagrec_ok(&entry->user_flags, flags);
				if (!cmp) continue;
			}

			/* This is a hit */
			hits++;

			/* Does the callback want client data? */
			cb = entry->callback;
			if (entry->bind_flags & BIND_WANTS_CD) {
				al[0] = (int) entry->client_data;
				table->nargs++;
			}
			else al++;

			/* Default return value is 0. */
			r = 0;

			switch (table->nargs) {
				case 0: r = cb(); break;
				case 1: r = cb(al[0]); break;
				case 2: r = cb(al[0], al[1]); break;
				case 3: r = cb(al[0], al[1], al[2]); break;
				case 4: r = cb(al[0], al[1], al[2], al[3]); break;
				case 5: r = cb(al[0], al[1], al[2], al[3], al[4]); break;
				case 6: r = cb(al[0], al[1], al[2], al[3], al[4], al[5]); break;
				case 7: r = cb(al[0], al[1], al[2], al[3], al[4], al[5], al[6]);
				case 8: r = cb(al[0], al[1], al[2], al[3], al[4], al[5], al[6], al[7]);
			}

			if (entry->bind_flags & BIND_WANTS_CD) table->nargs--;
			else al--;

			if ((table->flags & BIND_BREAKABLE) && (r & BIND_RET_BREAK)) return(r);
		}
	}
	return(r);
}

int check_tcl_bind(tcl_bind_list_t *tl, const char *match,
		   struct flag_record *atr, const char *param, int match_type)
{
  tcl_bind_mask_t	*tm, *tm_last = NULL, *tm_p = NULL;
  int			 cnt = 0;
  char			*proc = NULL, *fullmatch = NULL;
  tcl_cmd_t		*tc, *htc = NULL;
  int			 finish = 0, atrok, x, ok;

  for (tm = tl->first; tm && !finish; tm_last = tm, tm = tm->next) {
    if (tm->flags & TBM_DELETED)
      continue;
    /* Find out whether this bind matches the mask or provides
       the the requested atcributes, depending on the specified
       requirements. */
    switch (match_type & 0x03) {
    case MATCH_PARTIAL:
      ok = !egg_strncasecmp(match, tm->mask, strlen(match));
      break;
    case MATCH_EXACT:
      ok = !egg_strcasecmp(match, tm->mask);
      break;
    case MATCH_CASE:
      ok = !strcmp(match, tm->mask);
      break;
    case MATCH_MASK:
      ok = wild_match_per((unsigned char *) tm->mask, (unsigned char *) match);
      break;
    default:
      ok = 0;
    }
    if (!ok)
      continue;	/* This bind does not match. */

    if (match_type & BIND_STACKABLE) {
      /* Could be multiple commands/triggers. */
      for (tc = tm->first; tc; tc = tc->next) {
	if (match_type & BIND_USE_ATTR) {
	  /* Check whether the provided flags suffice for
	     this command/trigger. */
	  if (match_type & BIND_HAS_BUILTINS)
	    atrok = flagrec_ok(&tc->flags, atr);
	  else
	    atrok = flagrec_eq(&tc->flags, atr);
	} else
	  atrok = 1;

	if (atrok) {
	  cnt++;
	  tc->hits++;
	  tm_p = tm_last;
	  Tcl_SetVar(interp, "lastbind", (char *) tm->mask, TCL_GLOBAL_ONLY);
	  x = trigger_bind(tc->func_name, param);
	  if (match_type & BIND_ALTER_ARGS) {
	    if (interp->result == NULL || !interp->result[0])
	      return x;
	    /* This is such an amazingly ugly hack: */
	    Tcl_SetVar(interp, "_a", (char *) interp->result, 0);
	    /* Note: If someone knows what the above tries to
	       achieve, please tell me! (Fabian, 2000-10-14) */
	  } else if ((match_type & BIND_WANTRET) && x == BIND_EXEC_LOG)
	    return x;
	}
      }

      /* Apart from MATCH_MASK, currently no match type allows us to match
         against more than one bind. So if this isn't MATCH_MASK then exit
	 the loop now. */
      /* This will suffice until we have stackable partials. */
      if ((match_type & 3) != MATCH_MASK)
	finish = 1;
    } else {
      /* Search for valid entry. */
      for (tc = tm->first; tc; tc = tc->next)
	if (!(tc->attributes & TC_DELETED))
	  break;
      if (tc) {
        /* Check whether the provided flags suffice for this
           command/trigger. */
        if (match_type & BIND_USE_ATTR) {
	  if (match_type & BIND_HAS_BUILTINS)
	    atrok = flagrec_ok(&tc->flags, atr);
	  else
	    atrok = flagrec_eq(&tc->flags, atr);
	} else
	  atrok = 1;

	if (atrok) {
	  cnt++;
	  /* Remember information about this bind and its only
	     command/trigger. */
	  proc = tc->func_name;
	  fullmatch = tm->mask;
	  htc = tc;
	  tm_p = tm_last;

	  /* Either this is a non-partial match, which means we
	     only want to execute _one_ bind ... */
	  if ((match_type & 3) != MATCH_PARTIAL ||
	      /* ... or this is happens to be an exact match. */
	      !egg_strcasecmp(match, tm->mask))
	    cnt = finish = 1;
	}
      }
    }
  }

  if (!cnt)
    return BIND_NOMATCH;
  if ((match_type & 0x03) == MATCH_MASK ||
      (match_type & 0x03) == MATCH_CASE)
    return BIND_EXECUTED;

  /* Now that we have found at least one bind, we can update the
     preferred entries information. */
  if (htc)
    htc->hits++;
  if (tm_p) {
    /* Move mask to front of bind's mask list. */
    tm = tm_p->next;
    tm_p->next = tm->next;		/* Unlink mask from list.	*/
    tm->next = tl->first;		/* Readd mask to front of list.	*/
    tl->first = tm;
  }

  if (cnt > 1)
    return BIND_AMBIGUOUS;
  Tcl_SetVar(interp, "lastbind", (char *) fullmatch, TCL_GLOBAL_ONLY);
  return trigger_bind(proc, param);
}

/* Check for tcl-bound dcc command, return 1 if found
 * dcc: proc-name <handle> <sock> <args...>
 */
void check_tcl_dcc(const char *cmd, int idx, const char *args)
{
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  int x;

  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);

  x = check_bind(BT_dcc, cmd, &fr, dcc[idx].user, idx, args);
  if (x & BIND_RET_LOG) {
    putlog(LOG_CMDS, "*", "#%s# %s %s", dcc[idx].nick, cmd, args);
  }
}

void check_tcl_bot(const char *nick, const char *code, const char *param)
{
  Tcl_SetVar(interp, "_bot1", (char *) nick, 0);
  Tcl_SetVar(interp, "_bot2", (char *) code, 0);
  Tcl_SetVar(interp, "_bot3", (char *) param, 0);
  check_tcl_bind(H_bot, code, 0, " $_bot1 $_bot2 $_bot3", MATCH_EXACT);
}

void check_tcl_chonof(char *hand, int sock, tcl_bind_list_t *tl)
{
  struct flag_record	 fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  char			 s[11];
  struct userrec	*u;

  u = get_user_by_handle(userlist, hand);
  touch_laston(u, "partyline", now);
  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_chonof1", (char *) hand, 0);
  egg_snprintf(s, sizeof s, "%d", sock);
  Tcl_SetVar(interp, "_chonof2", (char *) s, 0);
  check_tcl_bind(tl, hand, &fr, " $_chonof1 $_chonof2", MATCH_MASK |
		 BIND_USE_ATTR | BIND_STACKABLE | BIND_WANTRET);
}

void check_tcl_chatactbcst(const char *from, int chan, const char *text,
			   tcl_bind_list_t *tl)
{
  char s[11];

  egg_snprintf(s, sizeof s, "%d", chan);
  Tcl_SetVar(interp, "_cab1", (char *) from, 0);
  Tcl_SetVar(interp, "_cab2", (char *) s, 0);
  Tcl_SetVar(interp, "_cab3", (char *) text, 0);
  check_tcl_bind(tl, text, 0, " $_cab1 $_cab2 $_cab3",
		 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_nkch(const char *ohand, const char *nhand)
{
  Tcl_SetVar(interp, "_nkch1", (char *) ohand, 0);
  Tcl_SetVar(interp, "_nkch2", (char *) nhand, 0);
  check_tcl_bind(H_nkch, ohand, 0, " $_nkch1 $_nkch2",
		 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_link(const char *bot, const char *via)
{
  check_bind(BT_link, bot, NULL, bot, via);

  Tcl_SetVar(interp, "_link1", (char *) bot, 0);
  Tcl_SetVar(interp, "_link2", (char *) via, 0);
  check_tcl_bind(H_link, bot, 0, " $_link1 $_link2",
		 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_disc(const char *bot)
{
  check_bind(BT_disc, bot, NULL, bot);

  Tcl_SetVar(interp, "_disc1", (char *) bot, 0);
  check_tcl_bind(H_disc, bot, 0, " $_disc1", MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_loadunld(const char *mod, tcl_bind_list_t *tl)
{
  Tcl_SetVar(interp, "_lu1", (char *) mod, 0);
  check_tcl_bind(tl, mod, 0, " $_lu1", MATCH_MASK | BIND_STACKABLE);
}

const char *check_tcl_filt(int idx, const char *text)
{
  char			s[11];
  int			x;
  struct flag_record	fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  egg_snprintf(s, sizeof s, "%ld", dcc[idx].sock);
  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);
  Tcl_SetVar(interp, "_filt1", (char *) s, 0);
  Tcl_SetVar(interp, "_filt2", (char *) text, 0);
  x = check_tcl_bind(H_filt, text, &fr, " $_filt1 $_filt2",
		     MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE |
		     BIND_WANTRET | BIND_ALTER_ARGS);
  if (x == BIND_EXECUTED || x == BIND_EXEC_LOG) {
    if (interp->result == NULL || !interp->result[0])
      return "";
    else
      return interp->result;
  } else
    return text;
}

int check_tcl_note(const char *from, const char *to, const char *text)
{
  int	x;

  Tcl_SetVar(interp, "_note1", (char *) from, 0);
  Tcl_SetVar(interp, "_note2", (char *) to, 0);
  Tcl_SetVar(interp, "_note3", (char *) text, 0);
  x = check_tcl_bind(H_note, to, 0, " $_note1 $_note2 $_note3", MATCH_EXACT);
  return (x == BIND_MATCHED || x == BIND_EXECUTED || x == BIND_EXEC_LOG);
}

void check_tcl_listen(const char *cmd, int idx)
{
  char	s[11];
  int	x;

  egg_snprintf(s, sizeof s, "%d", idx);
  Tcl_SetVar(interp, "_n", (char *) s, 0);
  x = Tcl_VarEval(interp, cmd, " $_n", NULL);
  if (x == TCL_ERROR)
    putlog(LOG_MISC, "*", "error on listen: %s", interp->result);
}

void check_tcl_chjn(const char *bot, const char *nick, int chan,
		    const char type, int sock, const char *host)
{
  struct flag_record	fr = {FR_GLOBAL, 0, 0, 0, 0, 0};
  char			s[11], t[2], u[11];

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
  egg_snprintf(s, sizeof s, "%d", chan);
  egg_snprintf(u, sizeof u, "%d", sock);
  Tcl_SetVar(interp, "_chjn1", (char *) bot, 0);
  Tcl_SetVar(interp, "_chjn2", (char *) nick, 0);
  Tcl_SetVar(interp, "_chjn3", (char *) s, 0);
  Tcl_SetVar(interp, "_chjn4", (char *) t, 0);
  Tcl_SetVar(interp, "_chjn5", (char *) u, 0);
  Tcl_SetVar(interp, "_chjn6", (char *) host, 0);
  check_tcl_bind(H_chjn, s, &fr,
		 " $_chjn1 $_chjn2 $_chjn3 $_chjn4 $_chjn5 $_chjn6",
		 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_chpt(const char *bot, const char *hand, int sock, int chan)
{
  char	u[11], v[11];

  egg_snprintf(u, sizeof u, "%d", sock);
  egg_snprintf(v, sizeof v, "%d", chan);
  Tcl_SetVar(interp, "_chpt1", (char *) bot, 0);
  Tcl_SetVar(interp, "_chpt2", (char *) hand, 0);
  Tcl_SetVar(interp, "_chpt3", (char *) u, 0);
  Tcl_SetVar(interp, "_chpt4", (char *) v, 0);
  check_tcl_bind(H_chpt, v, 0, " $_chpt1 $_chpt2 $_chpt3 $_chpt4",
		 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_away(const char *bot, int idx, const char *msg)
{
  char	u[11];

  check_bind(BT_away, bot, NULL, bot, idx, msg);

  egg_snprintf(u, sizeof u, "%d", idx);
  Tcl_SetVar(interp, "_away1", (char *) bot, 0);
  Tcl_SetVar(interp, "_away2", (char *) u, 0);
  Tcl_SetVar(interp, "_away3", msg ? (char *) msg : "", 0);
  check_tcl_bind(H_away, bot, 0, " $_away1 $_away2 $_away3",
		 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_time(struct tm *tm)
{
  char y[18];

  egg_snprintf(y, sizeof y, "%02d", tm->tm_min);
  Tcl_SetVar(interp, "_time1", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d", tm->tm_hour);
  Tcl_SetVar(interp, "_time2", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d", tm->tm_mday);
  Tcl_SetVar(interp, "_time3", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d", tm->tm_mon);
  Tcl_SetVar(interp, "_time4", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%04d", tm->tm_year + 1900);
  Tcl_SetVar(interp, "_time5", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d %02d %02d %02d %04d", tm->tm_min, tm->tm_hour,
	       tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);

  check_bind(BT_time, y, NULL, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);

  check_tcl_bind(H_time, y, 0,
		 " $_time1 $_time2 $_time3 $_time4 $_time5",
		 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_event(const char *event)
{
  check_bind(BT_event, event, NULL, event);

  Tcl_SetVar(interp, "_event1", (char *) event, 0);
  check_tcl_bind(H_event, event, 0, " $_event1", MATCH_EXACT | BIND_STACKABLE);
}

void tell_binds(int idx, char *par)
{
  tcl_bind_list_t	*tl, *tl_kind;
  tcl_bind_mask_t	*tm;
  int			 fnd = 0, showall = 0, patmatc = 0;
  tcl_cmd_t		*tc;
  char			*name, *proc, *s, flg[100];

  if (par[0])
    name = newsplit(&par);
  else
    name = NULL;
  if (par[0])
    s = newsplit(&par);
  else
    s = NULL;

  if (name)
    tl_kind = find_bind_table(name);
  else
    tl_kind = NULL;

  if ((name && name[0] && !egg_strcasecmp(name, "all")) || (s && s[0] && !egg_strcasecmp(s, "all")))
    showall = 1;
  if (tl_kind == NULL && name && name[0] && egg_strcasecmp(name, "all"))
    patmatc = 1;

  dprintf(idx, _("Command bindings:\n"));
  dprintf(idx, "  TYPE FLGS     COMMAND              HITS BINDING (TCL)\n");

  for (tl = tl_kind ? tl_kind : bind_table_list; tl;
       tl = tl_kind ? 0 : tl->next) {
    if (tl->flags & HT_DELETED)
      continue;
    for (tm = tl->first; tm; tm = tm->next) {
      if (tm->flags & TBM_DELETED)
	continue;
      for (tc = tm->first; tc; tc = tc->next) {
	if (tc->attributes & TC_DELETED)
	  continue;
	proc = tc->func_name;
	build_flags(flg, &(tc->flags), NULL);
	if (showall || proc[0] != '*') {
	  int	ok = 0;

          if (patmatc == 1) {
            if (wild_match(name, tl->name) ||
                wild_match(name, tm->mask) ||
                wild_match(name, tc->func_name))
	      ok = 1;
          } else
	    ok = 1;

	  if (ok) {
            dprintf(idx, "  %-4s %-8s %-20s %4d %s\n", tl->name, flg, tm->mask,
		    tc->hits, tc->func_name);
            fnd = 1;
          }
        }
      }
    }
  }
  if (!fnd) {
    if (patmatc)
      dprintf(idx, "No command bindings found that match %s\n", name);
    else if (tl_kind)
      dprintf(idx, "No command bindings for type: %s.\n", name);
    else
      dprintf(idx, "No command bindings exist.\n");
  }
}

void add_builtins2(bind_table_t *table, cmd_t *cmds)
{
	char name[50];

	for (; cmds->name; cmds++) {
		snprintf(name, 50, "*%s:%s", table->name, cmds->funcname ? cmds->funcname : cmds->name);
		add_bind_entry(table, cmds->flags, cmds->name, name, 0, cmds->func, NULL);
	}
}

/* Bring the default msg/dcc/fil commands into the Tcl interpreter */
void add_builtins(tcl_bind_list_t *tl, cmd_t *cc)
{
  int	k, i;
  char	p[1024], *l;

  for (i = 0; cc[i].name; i++) {
    egg_snprintf(p, sizeof p, "*%s:%s", tl->name,
		   cc[i].funcname ? cc[i].funcname : cc[i].name);
    l = (char *) malloc(Tcl_ScanElement(p, &k));
    Tcl_ConvertElement(p, l, k | TCL_DONT_USE_BRACES);
    Tcl_CreateCommand(interp, p, tl->func, (ClientData) cc[i].func, NULL);
    bind_bind_entry(tl, cc[i].flags, cc[i].name, l);
    free(l);
  }
}

void rem_builtins2(bind_table_t *table, cmd_t *cmds)
{
	char name[50];

	for (; cmds->name; cmds++) {
		sprintf(name, "*%s:%s", table->name, cmds->funcname ? cmds->funcname : cmds->name);
		del_bind_entry(table, cmds->flags, cmds->name, name);
	}
}

/* Remove the default msg/dcc/fil commands from the Tcl interpreter */
void rem_builtins(tcl_bind_list_t *table, cmd_t *cc)
{
  int	k, i;
  char	p[1024], *l;

  for (i = 0; cc[i].name; i++) {
    egg_snprintf(p, sizeof p, "*%s:%s", table->name,
		   cc[i].funcname ? cc[i].funcname : cc[i].name);
    l = (char *) malloc(Tcl_ScanElement(p, &k));
    Tcl_ConvertElement(p, l, k | TCL_DONT_USE_BRACES);
    Tcl_DeleteCommand(interp, p);
    unbind_bind_entry(table, cc[i].flags, cc[i].name, l);
    free(l);
  }
}
