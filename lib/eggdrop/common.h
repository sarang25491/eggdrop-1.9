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
 * $Id: common.h,v 1.9 2004/01/10 01:43:18 stdarg Exp $
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

#ifdef __STDC__
#  ifdef HAVE_STDARG_H
#    include <stdarg.h>
#  else
#    ifdef HAVE_STD_ARGS_H
#      include <std_args.h>
#    endif
#  endif
#  define EGG_VARARGS(type, name)		(type name, ...)
#  define EGG_VARARGS_DEF(type, name)		(type name, ...)
#  define EGG_VARARGS_START(type, name, list)	(va_start(list, name), name)
#else
#  include <varargs.h>
#  define EGG_VARARGS(type, name)		()
#  define EGG_VARARGS_DEF(type, name)		(va_alist) va_dcl
#  define EGG_VARARGS_START(type, name, list)	(va_start(list), va_arg(list,type))
#endif

#ifdef HAVE_INTTYPES_H
	#include <inttypes.h>
#else
	#ifdef HAVE_STDINT_H
		#include <stdint.h>
	#endif
#endif

#include "lib/compat/compat.h"

typedef int (*Function) ();

#endif /* !_EGG_COMMON_H_ */
