/* eggmod.h: header for eggmod.c
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
 *
 * $Id: eggmod.h,v 1.5 2003/12/17 07:39:14 wcc Exp $
 */

#ifndef _EGG_EGGMOD_H_
#define _EGG_EGGMOD_H_

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
	void *module_api;
};

int module_init();
int module_add_dir(const char *moddir);
int module_load(const char *name);
int module_unload(const char *name, int why);
egg_module_t *module_lookup(const char *name);
void *module_get_api(const char *name);
int module_addref(const char *name);
int module_decref(const char *name);
int get_module_list(char ***);

/* Windows hack to export functions from dlls. */
#if defined (__CYGWIN__) 
#  define EXPORT_SCOPE	__declspec(dllexport)
#else
#  define EXPORT_SCOPE
#endif

#endif /* !_EGG_EGGMOD_H_ */
