/*
 * eggdrop.h --
 */

#ifndef _EGGDROP_H
#define _EGGDROP_H

#include "../egglib/egglib.h"
#include <eggdrop/common.h>
#include <eggdrop/eggconfig.h>
#include <eggdrop/md5.h>
#include <eggdrop/base64.h>
#include <eggdrop/match.h>
#include <eggdrop/binds.h>
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
#include <eggdrop/flags.h>
#include <eggdrop/ircmasks.h>
#include <eggdrop/users.h>
#include <eggdrop/partyline.h>
#include <eggdrop/egglog.h>

BEGIN_C_DECLS

typedef struct eggdrop {
  Function *global;		/* FIXME: this field will be removed once the
				   global_funcs mess is cleaned up */
} eggdrop_t;

extern eggdrop_t *eggdrop_new(void);
extern eggdrop_t *eggdrop_delete(eggdrop_t *);

END_C_DECLS

#endif				/* !_EGGDROP_H */
