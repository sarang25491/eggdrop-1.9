#include <eggdrop/eggdrop.h>
#include <ltdl.h>
#include <unistd.h>
#include "eggmod.h"

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

int module_init()
{
	BT_load = bind_table_add("load", 1, "s", MATCH_MASK, 0);
	BT_unload = bind_table_add("unload", 1, "s", MATCH_MASK, 0);
	return(0);
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

	if (startfunc(&entry->modinfo)) return(-4);

	bind_check(BT_load, NULL, name, name);
	putlog(LOG_MISC, "*", "Module loaded: %s", name);
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

void *module_get_api(const char *name)
{
	module_list_t *entry;

	entry = find_module(name);
	if (!entry) return(NULL);
	return entry->modinfo.module_api;
}

void *module_release_api(const char *name)
{
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

int get_module_list(char ***names) {
	module_list_t *entry;
	int i = 0, c = 0;

	for (entry = module_list_head; entry; entry = entry->next) i++;
	*names = malloc((i + 1) * sizeof(char **));
	for (entry = module_list_head; entry; entry = entry->next) {
		(*names)[c] = (char *) entry->modinfo.name;
		c++;
	}
	(*names)[i] = NULL;
	return(i);
}
