#ifndef _EGGMOD_H_
#define _EGGMOD_H_

/* Values for 'why' in module_unload() */
#define EGGMOD_USER	0	/* User is manually unloading a module. */
#define EGGMOD_SHUTDOWN	1	/* Bot is exiting. Can't be ignored. */
#define EGGMOD_RESTART	2	/* Bot is restarting. */

struct egg_module;
typedef struct egg_module egg_module_t;

typedef int (*egg_start_func_t)(egg_module_t *modinfo);
typedef int (*egg_close_func_t)(int why);

struct egg_module {
	const char *name;
	const char *author;
	const char *version;
	const char *description;

	egg_close_func_t close_func;
	void *module_data;
};

int module_init();
int module_add_dir(const char *moddir);
int module_load(const char *name);
int module_unload(const char *name, int why);
egg_module_t *module_lookup(const char *name);
int module_addref(const char *name);
int module_decref(const char *name);

/* Windows hack to export functions from dlls. */
#if defined (__CYGWIN__) 
#  define EXPORT_SCOPE	__declspec(dllexport)
#else
#  define EXPORT_SCOPE
#endif

#endif
