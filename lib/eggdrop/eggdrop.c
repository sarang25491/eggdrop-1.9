/*
 * eggdrop.c
 *   libeggdrop 
 *
 * $Id: eggdrop.c,v 1.2 2002/03/26 01:06:21 ite Exp $
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
