/* core_binds.h: header for core_binds.c
 *
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
 *
 * $Id: core_binds.h,v 1.9 2003/12/20 00:34:37 stdarg Exp $
 */

#ifndef _EGG_CORE_BINDS_H_
#define _EGG_CORE_BINDS_H_

void core_binds_init();
void check_bind_time(struct tm *tm);
void check_bind_secondly();
void check_bind_status(partymember_t *p, const char *text);

#endif /* !_EGG_CORE_BINDS_H_ */
