/*
 * tclegg.h --
 *
 *	stuff used by tcl.c and tclhash.c
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
 * $Id: tclegg.h,v 1.19 2002/05/26 02:49:29 stdarg Exp $
 */

#ifndef _EGG_TCLEGG_H
#define _EGG_TCLEGG_H

#include "lush.h"		/* Include this here, since it's needed
				   in this file */
/* Used for stub functions:
 */

#define STDVAR		(cd, irp, argc, argv)				\
	ClientData cd;							\
	Tcl_Interp *irp;						\
	int argc;							\
	char *argv[];
#define BADARGS(nl, nh, example)	do {				\
	if ((argc < (nl)) || (argc > (nh))) {				\
		Tcl_AppendResult(irp, "wrong # args: should be \"",	\
				 argv[0], (example), "\"", NULL);	\
		return TCL_ERROR;					\
	}								\
} while (0)


typedef struct _tcl_strings {
  char *name;
  char *buf;
  int length;
  int flags;
} tcl_strings;

typedef struct _tcl_int {
  char *name;
  int *val;
  int readonly;
} tcl_ints;

typedef struct _tcl_coups {
  char *name;
  int *lptr;
  int *rptr;
} tcl_coups;

typedef struct _tcl_cmds {
  char *name;
  Function func;
} tcl_cmds;

void add_tcl_commands(tcl_cmds *);
void rem_tcl_commands(tcl_cmds *);
void add_tcl_strings(tcl_strings *);
void rem_tcl_strings(tcl_strings *);
void add_tcl_coups(tcl_coups *);
void rem_tcl_coups(tcl_coups *);
void add_tcl_ints(tcl_ints *);
void rem_tcl_ints(tcl_ints *);

#endif				/* !_EGG_TCLEGG_H */
