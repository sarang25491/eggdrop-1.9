/*
 * modules.h --
 *
 *	support for modules in eggdrop
 *
 * by Darrin Smith (beldin@light.iinet.net.au)
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
/*
 * $Id: modules.h,v 1.16 2002/05/05 16:40:38 tothwolf Exp $
 */

#ifndef _EGG_MODULE_H
#define _EGG_MODULE_H

/* Module related structures
 */
#include <eggdrop/modvals.h>

#ifndef MAKING_MODS
extern void (*encrypt_pass) (char *, char *);
extern char *(*encrypt_string) (char *, char *);
extern char *(*decrypt_string) (char *, char *);
extern int (*match_noterej) (struct userrec *, char *);
extern int (*storenote)(char *from, char *to, char *msg, int idx, char *who, int bufsize);
#endif /* MAKING_MODS	*/

#ifndef MAKING_NUMMODS

/* Modules specific functions and functions called by eggdrop
 */
extern void modules_init();
extern void do_module_report(int, int, char *);
extern int module_register(char *name, Function * funcs,
		    int major, int minor);
extern const char *module_load(char *module_name);
extern char *module_unload(char *module_name, char *nick);
extern module_entry *module_find(char *name, int, int);
extern Function *module_depend(char *, char *, int major, int minor);
extern int module_undepend(char *);
extern void add_hook(int hook_num, Function func);
extern void del_hook(int hook_num, Function func);
extern void *get_next_hook(int hook_num, void *func);
struct hook_entry {
  struct hook_entry *next;
  int (*func) ();
} *hook_list[REAL_HOOKS];

#define call_hook(x) do {					\
	register struct hook_entry *p, *pn;			\
								\
	for (p = hook_list[x]; p; p = pn) {			\
		pn = p->next;					\
		p->func();					\
	}							\
} while (0)
extern int call_hook_cccc(int, char *, char *, char *, char *);

#endif

typedef struct _dependancy {
  struct _module_entry *needed;
  struct _module_entry *needing;
  struct _dependancy *next;
  int major;
  int minor;
} dependancy;
extern dependancy *dependancy_list;

#endif				/* !_EGG_MODULE_H */
