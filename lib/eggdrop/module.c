/* module.c: module utility functions
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
static const char rcsid[] = "$Id: module.c,v 1.4 2004/06/23 21:12:57 stdarg Exp $";
#endif

#include <ltdl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <eggdrop/eggdrop.h>

typedef struct module_list {
	struct module_list *next;
	egg_module_t modinfo;
	lt_dlhandle hand;
	int refcount;
} module_list_t;

static module_list_t *module_list_head = NULL;
static bind_table_t *BT_load, *BT_unload;

static module_list_t *find_module(const char *name)
{
	module_list_t *entry;

	for (entry = module_list_head; entry; entry = entry->next) {
		if (!strcasecmp(name, entry->modinfo.name)) return(entry);
	}
	return(NULL);
}

int module_init(void)
{
	BT_load = bind_table_add(BTN_LOAD_MODULE, 1, "s", MATCH_MASK, 0);		/* DDD	*/
	BT_unload = bind_table_add(BTN_UNLOAD_MODULE, 1, "s", MATCH_MASK, 0);		/* DDD	*/
	return(0);
}

int module_shutdown(void)
{
	bind_table_del(BT_load);
	bind_table_del(BT_unload);

	return (0);
}

int module_add_dir(const char *moddir)
{
	char *fixed_moddir;

	if (*moddir != '/') {
		char cwd[1024];

		cwd[0] = 0;
		getcwd(cwd, sizeof(cwd));
		cwd[sizeof(cwd)-1] = 0;
		fixed_moddir = egg_mprintf("%s/%s", cwd, moddir);
	}
	else fixed_moddir = (char *)moddir;

	lt_dladdsearchdir(fixed_moddir);
	if (fixed_moddir != moddir) free(fixed_moddir);
	return(0);
}

int module_load(const char *name)
{
	lt_dlhandle hand;
	module_list_t *entry;
	egg_start_func_t startfunc;

	/* See if it's already loaded. */
	entry = find_module(name);
	if (entry) return(-1);

	hand = lt_dlopenext(name);
	if (!hand) {
		const char *err = lt_dlerror();
		putlog(LOG_MISC, "*", "Error loading module %s: %s", name, err);
		return(-2);
	}

	startfunc = (egg_start_func_t)lt_dlsym(hand, "start");
	if (!startfunc) {
		lt_dlclose(hand);
		return(-3);
	}

	/* Create an entry for it. */
	entry = calloc(1, sizeof(*entry));
	entry->next = module_list_head;
	entry->refcount = 0;
	module_list_head = entry;

	if (startfunc(&entry->modinfo)) {
		module_list_head = module_list_head->next;
		free(entry);
		return(-4);
	}

	putlog(LOG_MISC, "*", "Module loaded: %s", name);
	bind_check(BT_load, NULL, name, name);

	return(0);
}

int module_unload(const char *name, int why)
{
	module_list_t *entry, *prev;
	int retval;

	for (entry = module_list_head, prev = NULL; entry; prev = entry, entry = entry->next) {
		if (!strcasecmp(entry->modinfo.name, name)) break;
	}
	if (!entry) return(-1);
	if (entry->refcount > 0) return(-2);
	if (entry->modinfo.close_func) {
		retval = (entry->modinfo.close_func)(why);
		if (retval) return(-3);
	}

	lt_dlclose(entry->hand);
	if (prev) prev->next = entry->next;
	else module_list_head = entry->next;
	free(entry);
	bind_check(BT_unload, NULL, name, name);
	putlog(LOG_MISC, "*", "Module unloaded: %s", name);
	return(0);
}

egg_module_t *module_lookup(const char *name)
{
	module_list_t *entry;

	entry = find_module(name);
	if (entry) return(&entry->modinfo);
	return(NULL);
}

int module_loaded(const char *name)
{
	if (name == NULL) return 0;

	return (find_module (name) != NULL);
}

void *module_get_api(const char *name, int major, int minor)
{
	module_list_t *entry;

	entry = find_module(name);
	if (!entry) return(NULL);
	entry->refcount++;
	return entry->modinfo.module_api;
}

int module_addref(const char *name)
{
	module_list_t *entry;

	entry = find_module(name);
	if (!entry) return(-1);
	entry->refcount++;
	return(0);
}

int module_decref(const char *name)
{
	module_list_t *entry;

	entry = find_module(name);
	if (!entry) return(-1);
	entry->refcount--;
	return(0);
}

int module_list(const char ***names)
{
	module_list_t *entry;
	int i = 0;

	for (entry = module_list_head; entry; entry = entry->next) i++;
	*names = malloc((i + 1) * sizeof(char **));
	i = 0;
	for (entry = module_list_head; entry; entry = entry->next) {
		(*names)[i] = entry->modinfo.name;
		i++;
	}
	(*names)[i] = NULL;
	return(i);
}
