/*
 * woobie.c -- part of woobie.mod
 *   nonsensical command to exemplify module programming
 *
 * Originally written by ButchBub	  15 July     1997
 * Comments by Fabian Knittel		  29 December 1999
 *
 * $Id: woobie.c,v 1.3 2002/02/07 22:19:04 wcc Exp $
 */
/*
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

#define MODULE_NAME "woobie"
#define MAKING_WOOBIE
#include "lib/eggdrop/module.h"
#include <stdlib.h>

#define start woobie_LTX_start

#undef global
/* Pointer to the eggdrop core function table. Gets initialized in
 * woobie_start().
 */
static Function *global = NULL;

/* Bind table for dcc commands. We import it using find_bind_table(). */
static bind_table_t *BT_dcc;

static int cmd_woobie(struct userrec *u, int idx, char *par)
{
  /* Log the command as soon as you're sure all parameters are valid. */
  putlog(LOG_CMDS, "*", "#%s# woobie", dcc[idx].nick);

  dprintf(idx, "WOOBIE!\n");
  return 0;
}

/* A report on the module status.
 *
 * details is either 0 or 1:
 *    0 - `.status'
 *    1 - `.status all'  or  `.module woobie'
 */
static void woobie_report(int idx, int details)
{
}

/* Note: The tcl-name is automatically created if you set it to NULL. In
 *       the example below it would be just "*dcc:woobie". If you specify
 *       "woobie:woobie" it would be "*dcc:woobie:woobie" instead.
 *               ^----- command name   ^--- table name
 *        ^------------ module name
 */
static cmd_t mydcc[] =
{
  /* command	flags	function	tcl-name */
  {"woobie",	"",	cmd_woobie,	NULL},
  {NULL,	NULL,	NULL,		NULL}		/* Mark end. */
};

static char *woobie_close()
{
  if (BT_dcc) rem_builtins2(BT_dcc, mydcc);
  module_undepend(MODULE_NAME);
  return NULL;
}

/* Define the prototype here, to avoid warning messages in the
 * woobie_table.
 */
EXPORT_SCOPE char *woobie_start();

/* This function table is exported and may be used by other modules and
 * the core.
 *
 * The first four have to be defined (you may define them as NULL), as
 * they are checked by eggdrop core.
 */
static Function woobie_table[] =
{
  (Function) woobie_start,
  (Function) woobie_close,
  (Function) 0,
  (Function) woobie_report,
};

char *woobie_start(Function *global_funcs)
{
  /* Assign the core function table. After this point you use all normal
   * functions defined in src/mod/modules.h
   */
  global = global_funcs;

  /* Register the module. */
  module_register(MODULE_NAME, woobie_table, 2, 0);
  /*                                            ^--- minor module version
   *                                         ^------ major module version
   *                           ^-------------------- module function table
   *              ^--------------------------------- module name
   */

  if (!module_depend(MODULE_NAME, "eggdrop", 107, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires eggdrop1.7.0 or later";
  }

  /* Add command table to bind list BT_dcc, responsible for dcc commands.
   * Currently we only add one command, `woobie'.
   */
  BT_dcc = find_bind_table2("dcc");
  if (BT_dcc) add_builtins2(BT_dcc, mydcc);
  return NULL;
}
