/* eggdrop.h: header for eggdrop.c
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
 * $Id: eggdrop.h,v 1.43 2004/08/19 18:39:36 darko Exp $
 */

#ifndef _EGG_EGGDROP_H_
#define _EGG_EGGDROP_H_

#ifndef NULL
#	define NULL 0
#endif

#include <eggdrop/common.h>
#include <eggdrop/flags.h>
#include <eggdrop/ircmasks.h>
#include <eggdrop/hash_table.h>
#include <eggdrop/users.h>
#include <eggdrop/base64.h>
#include <eggdrop/binds.h>
#include <eggdrop/config.h>
#include <eggdrop/dns.h>
#include <eggdrop/ident.h>
#include <eggdrop/logging.h>
#include <eggdrop/module.h>
#include <eggdrop/net.h>
#include <eggdrop/owner.h>
#include <eggdrop/string.h>
#include <eggdrop/timer.h>
#include <eggdrop/fileutil.h>
#include <eggdrop/garbage.h>
#include <eggdrop/irccmp.h>
#include <eggdrop/ircparse.h>
#include <eggdrop/linemode.h>
#include <eggdrop/match.h>
#include <eggdrop/md5.h>
#include <eggdrop/memutil.h>
#include <eggdrop/socket.h>
#include <eggdrop/partyline.h>
#include <eggdrop/help.h>
#include <eggdrop/sockbuf.h>
#include <eggdrop/script.h>
#include <eggdrop/throttle.h>
#include <eggdrop/xml.h>
#include <eggdrop/memory.h>
#include <eggdrop/timeutil.h>

/* Gettext macros */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  define _(x)		gettext(x)
#  define N_(x)		gettext_noop(x)
#  define P_(x1, x2, n)	ngettext(x1, x2, n)
#else
#  define _(x)		(x)
#  define N_(x)		(x)
#  define P_(x1, x2, n)	( ((n) == 1) ? (x1) : (x2) )
#endif

/* Bind table names for eggdrop events */
#define BTN_EVENT	"event"

#define egg_assert(test) \
		do { \
			if (!(test)) { \
				fprintf (stderr, _("*** Assertion failed at %s in line %i: %s\n"), \
					__FILE__, __LINE__, # test); \
				return; \
			} \
		} while (0);

#define egg_assert_val(test, val) \
		do { \
			if (!(test)) { \
                                fprintf (stderr, _("*** Assertion failed at %s in line %i: %s\n"), \
                                        __FILE__, __LINE__, # test); \
				return val; \
			} \
	 	} while (0);

#define egg_return_if_fail(test) \
		if (!(test)) return;

#define egg_return_val_if_fail(test, val) \
		if (!(test)) return val;

				  
extern int eggdrop_init(void);
extern int eggdrop_shutdown(void);

extern int eggdrop_event(const char *event);
extern const char *eggdrop_get_param(const char *key);
extern int eggdrop_set_params(const char **params, int nparams);

#endif /* !_EGG_EGGDROP_H_ */
