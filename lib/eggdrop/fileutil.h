/* fileutil.h: header for fileutil.c
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
 * $Id: fileutil.h,v 1.8 2003/12/17 07:39:14 wcc Exp $
 */

#ifndef _EGG_FILEUTIL_H_
#define _EGG_FILEUTIL_H_

extern int copyfile(char *, char *);
extern int movefile(char *, char *);
extern int is_file(const char *);
extern int is_file_readable(const char *);

#endif /* !_EGG_FILEUTIL_H_ */
