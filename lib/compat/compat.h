/*
 * compat.h --
 *
 *	prototypes for compability functions
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
 * $Id: compat.h,v 1.12 2003/12/11 00:49:10 wcc Exp $
 */

#ifndef _EGG_COMPAT_H
#define _EGG_COMPAT_H

/* Include config.h */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/*
 * Include prototypes
 */
#include "memcpy.h"
#include "memset.h"
#include "strcasecmp.h"
#include "strncasecmp.h"
#include "snprintf.h"
#include "strftime.h"
#include "inet_ntop.h"
#include "inet_pton.h"
#include "strdup.h"
#include "strlcpy.h"
#include "strlcat.h"
#include "strerror.h"

#ifndef HAVE_GETOPT_LONG
# include "getopt.h"
#else
# include <getopt.h>
#endif

#endif				/* !_EGG_COMPAT_H */
