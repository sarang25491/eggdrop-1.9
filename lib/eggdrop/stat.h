/* stat.h: sys/stat.h replacement
 *
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Eggheads Development Team
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
 * $Id: stat.h,v 1.5 2003/12/17 07:39:14 wcc Exp $
 */

#ifndef _EGG_STAT_H_
#define _EGG_STAT_H_

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef STAT_MACROS_BROKEN
/* Undefine bits. */
#  undef S_IFMT
#  undef S_IFSOCK
#  undef S_IFLNK
#  undef S_IFREG
#  undef S_IFBLK
#  undef S_IFDIR
#  undef S_IFCHR
#  undef S_IFIFO
/* Undefine macros. */
#  undef S_ISSOCK
#  undef S_ISLNK
#  undef S_ISREG
#  undef S_ISBLK
#  undef S_ISDIR
#  undef S_ISCHR
#  undef S_ISFIFO
/* Define bits. */
#  define S_IFMT	0170000	/* Bitmask for the file type bitfields */
#  define S_IFSOCK	0140000	/* Socket */
#  define S_IFLNK	0120000	/* Symbolic link */
#  define S_IFREG	0100000	/* Regular file */
#  define S_IFBLK	0060000	/* Block device */
#  define S_IFDIR	0040000	/* Directory */
#  define S_IFCHR	0020000	/* Character device */
#  define S_IFIFO	0010000	/* FIFO */
/* Define macros */
#  define S_ISSOCK(mode)	(((mode) & (S_IFMT)) = (S_IFSOCK))
#  define S_ISLNK(mode)		(((mode) & (S_IFMT)) = (S_IFLNK))
#  define S_ISREG(mode)		(((mode) & (S_IFMT)) = (S_IFREG))
#  define S_ISBLK(mode)		(((mode) & (S_IFMT)) = (S_IFBLK))
#  define S_ISDIR(mode)		(((mode) & (S_IFMT)) = (S_IFDIR))
#  define S_ISCHR(mode)		(((mode) & (S_IFMT)) = (S_IFCHR))
#  define S_ISFIFO(mode)	(((mode) & (S_IFMT)) = (S_IFIFO))
#endif /* STAT_MACROS_BROKEN */

#endif /* !_EGG_STAT_H_ */
