/* socktimer.h: header for socktimer.c
 *
 * Copyright (C) 2002, 2003, 2004 Eggheads Development Team
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
 * $Id: socktimer.h,v 1.1 2008/11/02 17:46:13 sven Exp $
 */

#ifndef _EGG_SOCKTIMER_H_
#define _EGG_SOCKTIMER_H_

int socktimer_on(int idx, int timeout, int flags, void (*Callback)(int idx, void *cd), void *client_data, event_owner_t *owner);
int socktimer_off(int idx);
int socktimer_check(int idx);

#endif /* !_EGG_SOCKTIMER_H_ */
