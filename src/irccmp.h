/*
 * irccmp.h
 *   prototypes and macros for irccmp.c
 *
 * $Id: irccmp.h,v 1.1 2001/10/11 18:24:01 tothwolf Exp $
 */
/*
 * Copyright (C) 1990 Jarkko Oikarinen
 * Copyright (C) 1999, 2000, 2001 Eggheads Development Team
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
#ifndef _EGG_IRCCMP_H
#define _EGG_IRCCMP_H


/*
 * prototypes
 */
extern int _irccmp(const char *, const char *);
extern int _ircncmp(const char *, const char *, int);
extern int _irctolower(int);
extern int _irctoupper(int);

/*
 * character macros
 */
extern const unsigned char ToLowerTab[];
#define ToLower(c) (ToLowerTab[(unsigned char)(c)])

extern const unsigned char ToUpperTab[];
#define ToUpper(c) (ToUpperTab[(unsigned char)(c)])

#endif				/* !_EGG_IRCCMP_H */
