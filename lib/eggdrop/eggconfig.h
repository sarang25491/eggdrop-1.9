/* eggconfig.h: header for eggconfig.c
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
 * $Id: eggconfig.h,v 1.10 2004/06/19 10:30:41 wingman Exp $
 */

#ifndef _EGG_EGGCONFIG_H_
#define _EGG_EGGCONFIG_H_

#define CONFIG_INT	1
#define CONFIG_STRING	2

/* Bind table names for config events */
#define BTN_CONFIG_STR	"config_str"
#define BTN_CONFIG_INT	"config_int"
#define BTN_CONFIG_SAVE	"config_save"

typedef struct {
	const char *name;
	void *ptr;
	int type;
} config_var_t;

int config_init();
void *config_load(const char *fname);
int config_save(const char *handle, const char *fname);
int config_destroy(void *config_root);
void *config_get_root(const char *handle);
int config_set_root(const char *handle, void *config_root);
int config_delete_root(const char *handle);
void *config_lookup_section(void *config_root, ...);
void *config_exists(void *config_root, ...);
int config_get_int(int *intptr, void *config_root, ...);
int config_get_str(char **strptr, void *config_root, ...);
int config_set_int(int intval, void *config_root, ...);
int config_set_str(const char *strval, void *config_root, ...);
int config_link_table(config_var_t *table, void *config_root, ...);
int config_update_table(config_var_t *table, void *config_root, ...);
int config_unlink_table(config_var_t *table, void *config_root, ...);

#endif /* !_EGG_EGGCONFIG_H_ */
