/*
 * tcl.h
 *
 * $Id: tcl.h,v 1.1 2002/05/05 15:21:30 wingman Exp $
 */
/*
 * Copyright (C) 2000, 2001, 2002 Eggheads Development Team
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

#ifndef _EGG_TCL_H
#define _EGG_TCL_H

/* For pre Tcl7.5p1 versions */
#ifndef HAVE_TCL_FREE
#  define Tcl_Free(x) free(x)
#endif

/* For pre7.6 Tcl versions */
#ifndef TCL_PATCH_LEVEL
#  define TCL_PATCH_LEVEL Tcl_GetVar(interp, "tcl_patchLevel", TCL_GLOBAL_ONLY)
#endif

void do_tcl(char *, char *);
int readtclprog(char *fname);
int findanyidx(int);

#endif	/* _EGG_TCL_H	*/
