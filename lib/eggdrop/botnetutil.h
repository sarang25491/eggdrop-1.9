/*
 * botnetutil.h
 *   utility functions for the botnet protocol
 *
 * $Id: botnetutil.h,v 1.2 2002/02/07 22:18:59 wcc Exp $
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

#ifndef _EGG_BOTNETUTIL_H
#define _EGG_BOTNETUTIL_H

#include <eggdrop/common.h>

BEGIN_C_DECLS

extern int base64_to_int(char *);
extern char *int_to_base10(int);
extern char *int_to_base64(unsigned int);

extern int simple_sprintf EGG_VARARGS(char *, arg1);

END_C_DECLS

#endif
