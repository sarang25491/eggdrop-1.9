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
static const char rcsid[] = "$Id: module.c,v 1.13 2007/01/13 12:23:39 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <ltdl.h>
#include <unistd.h>

typedef struct module_list {
	struct module_list *prev;
	struct module_list *next;
	egg_module_t modinfo;
	lt_dlhandle hand;
	int refcount;
} module_list_t;

static module_list_t *module_list_head = NULL, *deleted_head = NULL;
static bind_table_t *BT_load, *BT_unload;

static int module_cleanup(void *);

#define find_active_module(name) find_module((name), module_list_head)
#define find_deleted_module(name) find_module((name), deleted_head)

static module_list_t *find_module(const char *name, module_list_t *head)
{
	module_list_t *entry;

	for (entry = head; entry; entry = entry->next) {
		if (!strcasecmp(name, entry->modinfo.name)) return(entry);
	}
	return(NULL);
}

int module_init(void)
{
	BT_load = bind_table_add(BTN_LOAD_MODULE, 1, "s", MATCH_MASK, 0);		/* DDD	*/
	BT_unload = bind_table_add(BTN_UNLOAD_MODULE, 2, "ss", MATCH_MASK, 0);		/* DDD	*/
	return(0);
}

/*!
 * \brief Shuts down the module interface.
 *
 * This function unloads all loaded modules and deletes the load and unload bind
 * tables.
 *
 * The module_unload() function is called for every module that is currently
 * loaded. If there is still a module loaded, for example because of
 * dependencies, module_unload() is called again. This is done until either all
 * modules have been unloaded or no more modules could be unloaded.
 *
 * \return Always 0.
 */

int module_shutdown(void)
{
	int unloaded;
	module_list_t *entry, *next;

	do {
		unloaded = 0;
		for (entry = module_list_head; entry; entry = next) {
			next = entry->next;
			if (!module_unload(entry->modinfo.name, MODULE_RESTART)) unloaded = 1;
		}
	} while (unloaded);

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

/*!
 * \brief Load a module.
 *
 * This function loads a module and executes its start function. If the module
 * name does not contain a path the default paths will be searched. If it does
 * have an extenteion the plattforms default library extentions will be added.
 *
 * \param name The name of the module to load.
 *
 * \return 0 on success. (This will be logged.)
 * \return -1 if the module is already loaded. (Nothing will be logged.)
 * \return -2 if the module could not be opened. (This will be logged.)
 * \return -3 if no start function is present. (Nothing will be logged.)
 * \return -4 if the start function returned an error. (Nothing will be logged.)
 * \return -5 if the module can't be loaded because it's marked for unloading. (Nothing will be logged.)
 */

int module_load(const char *name)
{
	lt_dlhandle hand;
	module_list_t *entry;
	egg_start_func_t startfunc;
	char *startname;


	/* See if it's already loaded. */
	entry = find_active_module(name);
	if (entry) return(-1);
	entry = find_deleted_module(name);
	if (entry) return(-5);

	hand = lt_dlopenext(name);
	if (!hand) {
		const char *err = lt_dlerror();
		putlog(LOG_MISC, "*", "Error loading module %s: %s", name, err);
		return(-2);
	}

	startname = egg_mprintf("%s_LTX_start", name);
	startfunc = (egg_start_func_t)lt_dlsym(hand, startname);
	free(startname);
	if (!startfunc) {
		startfunc = (egg_start_func_t)lt_dlsym(hand, "start");
		if (!startfunc) {
			lt_dlclose(hand);
			return(-3);
		}
	}

	/* Create an entry for it. */
	entry = calloc(1, sizeof(*entry));
	entry->prev = NULL;
	entry->next = module_list_head;
	entry->refcount = 0;
	entry->hand = hand;
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

/*!
 * \brief Unload a module.
 *
 * If a module's reference count is 0 its closing function will be executed.
 * If the closing function did not veto the unloading it will be removed from
 * the list of active modules, the "unload" bind is triggered and a cleanup
 * run is sceduled.
 *
 * \param name The name of the module to unload.
 * \param why The reason this function was called: ::MODULE_USER,
 *            ::MODULE_RESTART or ::MODULE_SHUTDOWN.
 *
 * \return 0 on success. (This will be logged.)
 * \return -1 if the module is not loaded. (Nothing will be logged.)
 * \return -2 if the module is in use by another module. (Nothing will be logged.)
 * \return -3 if the module's closing function vetoed. (Nothing will be logged.)
 */

int module_unload(const char *name, int why)
{
	module_list_t *entry;
	int retval;

	entry = find_active_module(name);
	if (!entry) return(-1);
	if (entry->refcount > 0) return(-2);
	if (entry->modinfo.close_func) {
		retval = entry->modinfo.close_func(why);
		if (retval) return(-3);
	}

	if (entry->prev) entry->prev->next = entry->next;
	else module_list_head = entry->next;
	if (entry->next) entry->next->prev = entry->prev;

	entry->next = NULL;
	if (!deleted_head) {
		deleted_head = entry;
		entry->prev = NULL;
	} else {
		module_list_t *tail;

		for (tail = deleted_head; tail->next; tail = tail->next);
		tail->next = entry;
		entry->prev = tail;
	}

	bind_check(BT_unload, NULL, name, name, why == MODULE_USER ? "request" : why == MODULE_SHUTDOWN ? "shutdown" : "restart");
	putlog(LOG_MISC, "*", "Module unloaded: %s", name);
	garbage_add(module_cleanup, NULL, GARBAGE_ONCE);
	return 0;
}

/*!
 * \brief Remove a module from the process's memory.
 *
 * Tis function will call the modules's unload function, remove all asyncronous
 * events owned by this module and finally remove the module itself from
 * memory.
 *
 * \param entry The module to remove. This \b must be in the list of deleted
 *              modules.
 */

static void module_really_unload(module_list_t *entry)
{
	int errors;
	const char *error;

	if (entry->prev) entry->prev->next = entry->next;
	else deleted_head = entry->next;
	if (entry->next) entry->next->prev = entry->prev;

	if (entry->modinfo.unload_func) entry->modinfo.unload_func();

	script_remove_events_by_owner(&entry->modinfo, 0);

	errors = lt_dlclose(entry->hand);
	if (errors) {
		putlog(LOG_MISC, "*", "Error unloading %s!", entry->modinfo.name);
		while ((error = lt_dlerror())) putlog(LOG_MISC, "*", "ltdlerror: %s", error);
	}

	free(entry);
}

/*!
 * \brief Calls module_really_unload() for every module marked for unloading.
 *
 * This function is called automatically by garbage_run() if a module has
 * has been closed.
 *
 * \return Always 0.
 */

static int module_cleanup(void *ignored)
{
	while (deleted_head) module_really_unload(deleted_head);

	return 0;
}

egg_module_t *module_lookup(const char *name)
{
	module_list_t *entry;

	entry = find_active_module(name);
	if (entry) return(&entry->modinfo);
	return(NULL);
}

int module_loaded(const char *name)
{
	if (name == NULL) return 0;

	return (find_active_module(name) != NULL);
}

void *module_get_api(const char *name, int major, int minor)
{
	module_list_t *entry;

	entry = find_active_module(name);
	if (!entry) return(NULL);
	entry->refcount++;
	return entry->modinfo.module_api;
}

int module_addref(const char *name)
{
	module_list_t *entry;

	entry = find_active_module(name);
	if (!entry) return(-1);
	entry->refcount++;
	return(0);
}

int module_decref(const char *name)
{
	module_list_t *entry;

	entry = find_active_module(name);
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
