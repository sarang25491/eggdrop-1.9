/* common.h: common macros, etc
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Eggheads Development Team
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
 *
 * $Id: common.h,v 1.10 2004/10/17 05:14:06 stdarg Exp $
 */

#ifndef _EGG_COMMON_H_
#define _EGG_COMMON_H_

#ifdef __cplusplus
#  define BEGIN_C_DECLS	extern "C" {
#  define END_C_DECLS	}
#else
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif

#if HAVE_CONFIG_H
	#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
#endif

#ifdef HAVE_STRING_H
	#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
	#include <strings.h>
#endif
#ifdef HAVE_CTYPE_H
	#include <ctype.h>
#endif

#ifdef __STDC__
#  ifdef HAVE_STDARG_H
#    include <stdarg.h>
#  else
#    ifdef HAVE_STD_ARGS_H
#      include <std_args.h>
#    endif
#  endif
#else
#  include <varargs.h>
#endif

#ifdef HAVE_INTTYPES_H
	#include <inttypes.h>
#else
	#ifdef HAVE_STDINT_H
		#include <stdint.h>
	#endif
#endif

#ifdef TIME_WITH_SYS_TIME
	#include <sys/time.h>
	#include <time.h>
#else
	#ifdef HAVE_SYS_TIME_H
		#include <sys/time.h>
	#else
		#include <time.h>
	#endif
#endif

#include "lib/compat/compat.h"

typedef int (*Function) ();

#endif /* !_EGG_COMMON_H_ */
