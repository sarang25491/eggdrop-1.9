/* eggowner.c: functions for working with the 'owner' config file variable
 *
 * Copyright (C) 2003, 2004 Eggheads Development Team
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

#ifndef lint
static const char rcsid[] = "$Id: eggowner.c,v 1.3 2003/12/17 07:39:14 wcc Exp $";
#endif

#include <ctype.h>
#include <eggdrop/eggdrop.h>

static char **owner = NULL;

int egg_setowner(char **_owner)
{
	owner = _owner;
	return(0);
}

int egg_isowner(const char *handle)
{
	int len;
	char *powner;

	if (!owner || !*owner) return(0);

	len = strlen(handle);
	if (!len) return(0);

	powner = *owner;
	while (*powner) {
		while (*powner && !isalnum(*powner)) powner++;
		if (!*powner) break;
		if (!strncasecmp(powner, handle, len)) {
			powner += len;
			if (!*powner || !isalnum(*powner)) return(1);
		}
		while (*powner && isalnum(*powner)) powner++;
	}
	return(0);
}
