/* config.h: header for config.c
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
 * $Id: config.h,v 1.3 2004/06/28 17:36:34 wingman Exp $
 */

#ifndef _EGG_CONFIG_H_
#define _EGG_CONFIG_H_

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

int config_init(void);
int config_shutdown(void);

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

/* Example usage:
 *
 *  <channel name="test" mode="+nt">
 *    <option>+option1</option>
 *    <option>-option2</option>
 *  </cannel>
 *
 *  typedef struct {
 *    char *name;
 *    char *mode;
 *    char **options;
 *    int noptions;
 *  } channel_t;
 *
 *  config_variable_t channel_vars[] = {
 *    { "@name",  CONFIG_NONE,  &CONFIG_TYPE_STRING },
 *    { "@mode",  CONFIG_NONE,  &CONFIG_TYPE_STRING },
 *    { "option", CONFIG_ARRAY, &CONFIG_TYPE_STRING },
 *    { 0, 0, 0 },
 *  }
 *  config_type_t channel_type = { "channel", sizeof(channel_t), &channel_vars };
 *  
 *  void some_func() {
 *     channel_t channel;
 *  
 *     config2_link(0, &channel_type, &channel); // loading
 *     config2_sync(0, &channel_type, &channel); // saving
 *
 *     printf("channel %s (%s)", channel.name, channel.mode);
 *     for (i = 0; i < channel.noptions; i++) {
 *       printf("option %i: %s\n", i, channel.options[i]);
 *     }
 *  }
 *
 */

typedef struct config_variable config_variable_t;
typedef struct config_type config_type_t;

#define CONFIG_NONE	0
#define CONFIG_ARRAY	1
#define CONFIG_ENUM	2

struct config_variable
{
        char *path;			/* path			*/
        int modifier;      		/* NONE, ARRAY		*/ 
        config_type_t *type;		/* type			*/
};

struct config_type
{
        char *name;			/* name			*/
        size_t size;			/* size			*/
        config_variable_t *vars;	/* children		*/
};

extern config_type_t CONFIG_TYPE_BOOL;
extern config_type_t CONFIG_TYPE_INT;
extern config_type_t CONFIG_TYPE_STRING;
extern config_type_t CONFIG_TYPE_TIMESTAMP;

int config2_link(int config, config_type_t *type, void *instance);
int config2_sync(int config, config_type_t *type, void *instance);

#endif /* !_EGG_CONFIG_H_ */
