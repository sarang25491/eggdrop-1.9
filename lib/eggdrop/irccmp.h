/* match.h: header for match.c
 *
 * Copyright (C) 1990 Jarkko Oikarinen
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
 * $Id: irccmp.h,v 1.3 2003/12/17 07:39:14 wcc Exp $
 */

#ifndef _EGG_IRCCMP_H_
#define _EGG_IRCCMP_H_

#define ToUpper(c) (ToUpperTab[(unsigned)c])
#define ToLower(c) (ToLowerTab[(unsigned)c])

extern int irccmp(const char *, const char *);
extern int ircncmp(const char *, const char *, int);

#endif /* !_EGG_IRCCMP_H */
