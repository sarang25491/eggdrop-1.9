/*
 * debug.h --
 *
 *	assert debugging
 */
/*
 * Copyright (C) 2001, 2002, 2003, 2004 Eggheads Development Team
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
 * $Id: debug.h,v 1.5 2003/12/11 00:49:11 wcc Exp $
 */

#ifndef _EGG_DEBUG_H
#define _EGG_DEBUG_H

#ifdef DEBUG
# undef NDEBUG
/*
 * Some pre-ANSI implementations (e.g. SunOS 4)
 * need stderr defined if assertion checking is enabled.
 */
# include <stdio.h>		
# include <assert.h>
#else				/* !DEBUG */
# undef assert
# define assert(e)
#endif				/* DEBUG */

#endif				/* !_EGG_DEBUG_H */
