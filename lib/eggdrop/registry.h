/*
 * registry.h --
 *
 *	eggdrop registry.
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
 * $Id: registry.h,v 1.3 2002/05/05 16:40:32 tothwolf Exp $
 */

#ifndef _EGG_REGISTRY_H
#define _EGG_REGISTRY_H

#include <eggdrop/common.h>

BEGIN_C_DECLS

#define REGISTRY_DONT_CREATE	1
#define REGISTRY_PREPEND	2
#define REGISTRY_CHAIN		4
#define REGISTRY_MAIN		8
#define REGISTRY_DEFAULT	8

#define REGISTRY_TEMP_LISTENER	1

#define REGISTRY_HALT		1
#define REGISTRY_SKIP_MAIN	2

typedef struct registry_entry_b {
  char *aclass;
  char *name;
  Function callback;
  void *client_data;
  int nargs;
  int flags;
  int action;
  void *return_value;
} registry_entry_t;

typedef struct registry_simple_chain_b {
  char *name;
  Function callback;
  int nargs;
} registry_simple_chain_t;

extern int registry_add(registry_entry_t *);
extern int registry_add_table(registry_entry_t *);
extern int registry_add_simple_chains(registry_simple_chain_t *);
extern int registry_remove(registry_entry_t *);
extern int registry_remove_table(registry_entry_t *);
extern int registry_lookup(const char *, const char *, Function *, void **);
extern int registry_unlookup(const char *, const char *, Function *, void **);

END_C_DECLS

#endif				/* !_EGG_REGISTRY_H */
