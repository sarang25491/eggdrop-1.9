/*
 * woobie.c --
 *
 *	nonsensical command to exemplify module programming
 *
 * Originally written by ButchBub	  15 July     1997
 * Comments by Fabian Knittel		  29 December 1999
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

#ifndef lint
static const char rcsid[] = "$Id: woobie.c,v 1.6 2002/05/17 07:29:25 stdarg Exp $";
#endif

#define MODULE_NAME "woobie"
#define MAKING_WOOBIE
#include "lib/eggdrop/module.h"
#include <stdlib.h>

#define start woobie_LTX_start

/* Pointer to the eggdrop core function table. Gets initialized in
 * woobie_start().
 */
static eggdrop_t *egg = NULL;

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
  rem_builtins("dcc", mydcc);
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

char *woobie_start(eggdrop_t *eggdrop)
{
  /* Assign the core function table. After this point you use all normal
   * functions defined in src/mod/modules.h
   */
  egg = eggdrop;

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

  /* Add command table to the "dcc" table, responsible for dcc commands.
   * Currently we only add one command, `woobie'.
   */
  add_builtins("dcc", mydcc);
  return NULL;
}
