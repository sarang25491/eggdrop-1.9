/*
 * fileutil.h --
 *
 *	convenience utilities to deal with common file operations
 */
/*
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
 */
/*
 * $Id: fileutil.h,v 1.7 2003/12/11 00:49:10 wcc Exp $
 */

#ifndef _EGG_FILEUTIL_H
#define _EGG_FILEUTIL_H

#include <eggdrop/common.h>

BEGIN_C_DECLS

extern int copyfile(char *, char *);
extern int movefile(char *, char *);
extern int is_file(const char *);
extern int file_readable(const char *);

END_C_DECLS

#endif				/* !_EGG_FILEUTIL_H */
