/*
 * main.h --
 *
 *	include file to include most other include files
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
 * $Id: main.h,v 1.30 2003/01/02 21:33:16 wcc Exp $
 */

#ifndef _EGG_MAIN_H
#define _EGG_MAIN_H

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "debug.h"
#include "egg.h"
#include <eggdrop/eggdrop.h>
#include "tclegg.h"
#include "lib/compat/compat.h"

extern eggdrop_t *egg;

void fatal(const char *, int);
void patch(const char *);

#endif				/* !_EGG_MAIN_H */
