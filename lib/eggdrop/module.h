/*
 * module.h --
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Eggheads Development Team
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
 * $Id: module.h,v 1.44 2003/02/25 10:28:21 stdarg Exp $
 */

#ifndef _EGG_MOD_MODULE_H
#define _EGG_MOD_MODULE_H

/* FIXME: remove this ugliness ASAP! */
#define MAKING_MODS

#include "src/main.h"		/* NOTE: when removing this, include config.h */

/* FIXME: filter out files which are not needed (it was easier after removing
   proto.h and cleaning up main.h to just include all .h files here w/o
   validating if they aren't needed */
#include "src/users.h"
#include "src/bg.h"
#include "src/chan.h"
#include "src/cmdt.h"
#include "src/core_binds.h"
#include "src/debug.h"
#include "src/egg.h"
#include "src/flags.h"
#include "src/logfile.h"
#include "src/misc.h"
#include "src/modules.h"
#include "src/logfile.h"

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
#undef wild_match_per
#undef maskhost
#undef maskban

#if defined (__CYGWIN__) 
#  define EXPORT_SCOPE	__declspec(dllexport)
#else
#  define EXPORT_SCOPE
#endif

/* Redefine for module-relevance */

#define module_rename ((int (*)(char *, char *))egg->global[0])
#define module_register ((int (*)(char *, Function *, int, int))egg->global[1])
#define module_find ((module_entry * (*)(char *,int,int))egg->global[2])
#define module_depend ((Function *(*)(char *,char *,int,int))egg->global[3])
#define module_undepend ((int(*)(char *))egg->global[4])
#define egg_timeval_now (*(egg_timeval_t *)egg->global[5])
#define owner_check ((int (*)(const char *))egg->global[6])

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
