/* memutil.h: header for memutil.c
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
 * $Id: memutil.h,v 1.18 2004/10/17 05:14:06 stdarg Exp $
 */

#ifndef _EGG_MEMUTIL_H_
#define _EGG_MEMUTIL_H_

extern void str_redup(char **str, const char *newstr);
extern char *egg_mprintf(const char *format, ...);
extern char *egg_msprintf(char *buf, int len, int *final_len, const char *format, ...);
extern char *egg_mvsprintf(char *buf, int len, int *final_len, const char *format, va_list args);

#endif /* !_EGG_MEMUTIL_H_ */
