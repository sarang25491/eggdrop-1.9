/*
 * registry.c
 *   eggdrop registry.
 *
 * $Id: registry.c,v 1.1 2002/03/22 16:01:16 ite Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002 Eggheads Development Team
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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>		/* strlen(), memset(), memmove() */
#include <stdlib.h>

#include "../lib/egglib/hash_table.h"
#include "eggdrop.h"
#include "registry.h"

typedef struct registry_internal_entry_b {
  char *key;
  Function callback;
  void *client_data;
  registry_entry_t **main;	/* Pointer to the "main" function(s). */
  int nmain;
  registry_entry_t **chains;	/* Chain functions. */
  int nchains;
  Function **listeners;		/* Addresses of those who are using this entry. */
  void ***listener_handles;	/* Ditto. */
  int nlisteners;
} registry_internal_entry_t;

static int update_listeners(registry_internal_entry_t * internal);

/* FIXME: VARARGS */
static int arbitrator(registry_internal_entry_t * internal, ...);

int registry_add(eggdrop_t * egg, registry_entry_t * entry)
{
  registry_internal_entry_t *internal;
  char *key;

  if (!egg->registry_table) {
    egg->registry_table = hash_table_create(NULL, NULL, 13, 0);
  }

  key = (char *) malloc(strlen(entry->aclass) + strlen(entry->name) + 4);
  sprintf(key, "%s / %s", entry->aclass, entry->name);

  /* Look up registry entry. */
  if (hash_table_find(egg->registry_table, key, &internal)) {
    /* Doesn't exist yet, get a new one. */
    internal = (registry_internal_entry_t *) malloc(sizeof(*internal));
    memset(internal, 0, sizeof(*internal));
    internal->key = key;
    hash_table_insert(egg->registry_table, key, internal);
  } else
    free(key);

  if (entry->flags & REGISTRY_MAIN) {
    internal->nmain++;
    internal->main =
      (registry_entry_t **) realloc(internal->main,
				    internal->nmain * sizeof(void *));
    internal->main[internal->nmain - 1] = entry;
  } else if (entry->flags & REGISTRY_CHAIN) {
    internal->nchains++;
    internal->chains =
      (registry_entry_t **) realloc(internal->chains,
				    internal->nchains * sizeof(void *));
    if (entry->flags & REGISTRY_PREPEND) {
      memmove(internal->chains + 1, internal->chains,
	      sizeof(void *) * (internal->nchains - 1));
      internal->chains[0] = entry;
    } else {
      internal->chains[internal->nchains - 1] = entry;
    }
  }

  update_listeners(internal);

  return (0);
}

int registry_add_table(eggdrop_t * egg, registry_entry_t * entries)
{
  while (entries->callback) {
    registry_add(egg, entries);
    entries++;
  }
  return (0);
}

int registry_add_simple_chains(eggdrop_t * egg,
			       registry_simple_chain_t * entries)
{
  registry_entry_t *entry;
  char *aclass;

  /* First entry gives the class. */
  aclass = entries->name;
  entries++;
  while (entries->name) {
    entry = (registry_entry_t *) calloc(1, sizeof(*entry));
    entry->aclass = aclass;
    entry->name = entries->name;
    entry->callback = entries->callback;
    entry->nargs = entries->nargs;
    entry->flags = REGISTRY_CHAIN;
    registry_add(egg, entry);
    entries++;
  }
  return (0);
}

/* FIXME: unfinished */
int registry_remove(eggdrop_t * egg, registry_entry_t * entry)
{
  printf("registry_remove(%s, %s)\n", entry->aclass, entry->name);
  return (0);
}

/* FIXME: unfinished */
int registry_remove_table(eggdrop_t * egg, registry_entry_t * entries)
{
  while (entries->callback) {
    registry_remove(egg, entries);
    entries++;
  }
  return (0);
}

int registry_lookup(eggdrop_t * egg, const char *aclass, const char *name,
		    Function * funcptr, void **handleptr)
{
  registry_internal_entry_t *internal;
  char *key;

  key = (char *) malloc(strlen(aclass) + strlen(name) + 4);
  sprintf(key, "%s / %s", aclass, name);

  if (!egg->registry_table) {
    egg->registry_table = hash_table_create(NULL, NULL, 13, 0);
  }

  if (hash_table_find(egg->registry_table, key, &internal)) {
    /* Doesn't exist -- create it. */
    internal = (registry_internal_entry_t *) malloc(sizeof(*internal));
    memset(internal, 0, sizeof(*internal));
    internal->key = key;
    hash_table_insert(egg->registry_table, key, internal);
    internal->callback = (Function) arbitrator;
    internal->client_data = (void *) internal;
  } else
    free(key);

  *funcptr = internal->callback;
  *handleptr = internal->client_data;

  internal->nlisteners++;
  internal->listeners =
    (Function **) realloc(internal->listeners,
			  internal->nlisteners * sizeof(void *));
  internal->listener_handles =
    (void ***) realloc(internal->listener_handles,
		       internal->nlisteners * sizeof(void *));
  internal->listeners[internal->nlisteners - 1] = funcptr;
  internal->listener_handles[internal->nlisteners - 1] = handleptr;

  return (0);
}

/* FIXME: unfinished */
int registry_unlookup(eggdrop_t * egg, const char *aclass,
		      const char *name, Function * funcptr,
		      void **handleptr)
{
/*
	memmove(entry->listeners+i, entry->listeners+i+1, (entry->nlisteners-i-1) * sizeof(Function *));
	entry->nlisteners--;
	if (!entry->nlisteners) {
		free(entry->listeners);
		entry->listeners = NULL;
	}
*/
  return (0);
}

static int update_listeners(registry_internal_entry_t * internal)
{
  int i;

  if (internal->nmain != 0 && internal->nchains == 0) {
    /* If there's just a main function, give them that. */
    internal->callback = internal->main[internal->nmain - 1]->callback;
    internal->client_data =
      internal->main[internal->nmain - 1]->client_data;
  } else {
    /* Otherwise we use an arbitrator. */
    internal->callback = (Function) arbitrator;
    internal->client_data = (void *) internal;
  }

  for (i = 0; i < internal->nlisteners; i++) {
    *(internal->listeners[i]) = internal->callback;
    *(internal->listener_handles[i]) = internal->client_data;
  }
  return (0);
}

/* FIXME: VARARGS */
static int arbitrator(registry_internal_entry_t * internal, ...)
{
  registry_entry_t *entry;
  int i;

  /* FIXME: accessing var args like that isn't portable */
  int *al = (int *) &internal;

  /* Call chains backwards (first-in-last-out). */
  for (i = internal->nchains - 1; i >= 0; i--) {
    entry = internal->chains[i];
    entry->action = 0;
    switch (entry->nargs) {
    case 1:
      entry->callback(entry);
      break;
    case 2:
      entry->callback(entry, al[1]);
      break;
    case 3:
      entry->callback(entry, al[1], al[2]);
      break;
    case 4:
      entry->callback(entry, al[1], al[2], al[3]);
      break;
    case 5:
      entry->callback(entry, al[1], al[2], al[3], al[4]);
      break;
    case 6:
      entry->callback(entry, al[1], al[2], al[3], al[4], al[5]);
      break;
    case 7:
      entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6]);
      break;
    case 8:
      entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6],
		      al[7]);
      break;
    case 9:
      entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6],
		      al[7], al[8]);
      break;
    case 10:
      entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6],
		      al[7], al[8], al[9]);
    }
    if (entry->action & REGISTRY_HALT)
      break;
  }

  if (internal->nmain != 0) {
    entry = internal->main[internal->nmain - 1];
    switch (entry->nargs) {
    case 1:
      entry->callback(entry);
      break;
    case 2:
      entry->callback(entry, al[1]);
      break;
    case 3:
      entry->callback(entry, al[1], al[2]);
      break;
    case 4:
      entry->callback(entry, al[1], al[2], al[3]);
      break;
    case 5:
      entry->callback(entry, al[1], al[2], al[3], al[4]);
      break;
    case 6:
      entry->callback(entry, al[1], al[2], al[3], al[4], al[5]);
      break;
    case 7:
      entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6]);
      break;
    case 8:
      entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6],
		      al[7]);
      break;
    case 9:
      entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6],
		      al[7], al[8]);
      break;
    case 10:
      entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6],
		      al[7], al[8], al[9]);
    }
    return ((int) entry->return_value);
  }
  return (0);
}
