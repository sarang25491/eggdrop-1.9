/*
 * eggdrop.h --
 *
 *	libeggdrop header file
 */
/*
 * Copyright (C) 2001, 2002 Eggheads Development Team
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
 * $Id: eggdrop.h,v 1.11 2002/10/07 22:33:54 stdarg Exp $
 */

#ifndef _EGGDROP_H
#define _EGGDROP_H

#include "../egglib/egglib.h"
#include <eggdrop/common.h>
#include <eggdrop/botnetutil.h>
#include <eggdrop/memutil.h>
#include <eggdrop/fileutil.h>
#include <eggdrop/script.h>
#include <eggdrop/my_socket.h>
#include <eggdrop/sockbuf.h>
#include <eggdrop/eggnet.h>
#include <eggdrop/eggdns.h>
#include <eggdrop/eggident.h>
#include <eggdrop/linemode.h>
#include <eggdrop/eggtimer.h>
#include <eggdrop/throttle.h>
#include <eggdrop/hash_table.h>
#include <eggdrop/xml.h>

BEGIN_C_DECLS

typedef struct eggdrop {
  Function *global;		/* FIXME: this field will be removed once the
				   global_funcs mess is cleaned up */
} eggdrop_t;

extern eggdrop_t *eggdrop_new(void);
extern eggdrop_t *eggdrop_delete(eggdrop_t *);

END_C_DECLS

#endif				/* !_EGGDROP_H */
