/*
 * strlcpy.h --
 *
 *	prototypes for strlcpy.c
 */
/*
 * Copyright (C) 2002, 2003 Eggheads Development Team
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
 * $Id: strlcpy.h,v 1.3 2003/01/02 21:33:13 wcc Exp $
 */

#ifndef _EGG_STRLCPY_H
#define _EGG_STRLCPY_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif

#endif				/* !_EGG_STRLCPY_H */
