/*
 * eggdrop.c --
 *
 *	libeggdrop 
 */
/*
 * Copyright (C) 2002 Eggheads Development Team
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
static const char rcsid[] = "$Id: eggdrop.c,v 1.3 2002/05/05 16:40:32 tothwolf Exp $";
#endif

#include <stdlib.h>
#include <string.h>
#include "eggdrop.h"

eggdrop_t *eggdrop_new(void)
{
  eggdrop_t *egg;

  egg = (eggdrop_t *) malloc(sizeof(eggdrop_t));
  memset(egg, 0, sizeof(eggdrop_t));

  return egg;
}

eggdrop_t *eggdrop_delete(eggdrop_t * egg)
{
  free_null(egg);

  return NULL;
}
