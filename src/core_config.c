/* core_config.c: config file
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
static const char rcsid[] = "$Id: core_config.c,v 1.21 2004/06/28 17:36:34 wingman Exp $";
#endif

#include <string.h>
#include <eggdrop/eggdrop.h>
#include "core_config.h"

extern char *configfile;
core_config_t core_config = {0};
void *config_root = NULL;

static config_var_t core_config_vars[] = {
	/* General bot stuff. */
	{"botname", &core_config.botname, CONFIG_STRING},		/* DDC	*/
	{"userfile", &core_config.userfile, CONFIG_STRING},		/* DDC	*/
	{"lockfile", &core_config.lockfile, CONFIG_STRING},		/* DDC	*/

	/* The owner. */
	{"owner", &core_config.owner, CONFIG_STRING},			/* DDC	*/
	{"admin", &core_config.admin, CONFIG_STRING},			/* DDC	*/

	/* Paths. */
	{"help_path", &core_config.help_path, CONFIG_STRING},		/* DDD	*/
	{"temp_path", &core_config.temp_path, CONFIG_STRING},		/* DDD	*/
	{"text_path", &core_config.text_path, CONFIG_STRING},		/* DDD	*/
	{"module_path", &core_config.module_path, CONFIG_STRING},	/* DDD	*/

	/* Whois. */
	{"whois_items", &core_config.whois_items, CONFIG_STRING},	/* DDD	*/

	/* Other. */
	{"die_on_sigterm", &core_config.die_on_sigterm, CONFIG_INT},	/* DDD	*/
	{0}
};

config_variable_t loglevel_vars[] = {
	{ "LOG_MISC",   LOG_MISC,   NULL },
	{ "LOG_PUBLIC", LOG_PUBLIC, NULL }, 
	{ "LOG_MSGS",   LOG_MSGS,   NULL },
	{ "LOG_JOIN",   LOG_JOIN,   NULL },
	{ 0, 0, 0 }
};
config_type_t loglevel_type = { "level", sizeof(int), loglevel_vars };

config_variable_t logfile_vars[] = {
	{ "@filename", CONFIG_NONE, &CONFIG_TYPE_STRING },
	{ "@channel",  CONFIG_NONE, &CONFIG_TYPE_STRING },
	{ NULL,        CONFIG_ENUM, &loglevel_type      },
	{ 0, 0, 0 }
};
config_type_t logfile_type = { "logfile", sizeof(logfile_t), logfile_vars };

config_variable_t logging_vars[] = {
	{ "@keep_all",  CONFIG_NONE,  &CONFIG_TYPE_BOOL   },
	{ "@quick",     CONFIG_NONE,  &CONFIG_TYPE_BOOL   },
	{ "@max_size",  CONFIG_NONE,  &CONFIG_TYPE_INT    },
	{ "@switch_at", CONFIG_NONE,  &CONFIG_TYPE_INT    },
	{ "@suffix",    CONFIG_NONE,  &CONFIG_TYPE_STRING },
	{ "logfiles",   CONFIG_ARRAY, &logfile_type       },	
	{ 0, 0, 0 }
};
config_type_t logging_type = { "logging", sizeof(logging_t), logging_vars };

int core_config_init(const char *fname)
{
	/* Set default vals. */
	memset(&core_config, 0, sizeof(core_config));

	/* Hook the owner variable into libeggdrop. */
	egg_setowner(&core_config.owner);

	config_root = config_load(fname);
	if (config_root == NULL)
		return -1;

	config_set_root("eggdrop", config_root);
	config_link_table(core_config_vars, config_root, "eggdrop", 0, NULL);
	if (!core_config.botname) core_config.botname = strdup("eggdrop");
	if (!core_config.userfile) core_config.userfile = strdup("users.xml");
	if (!core_config.lockfile) core_config.lockfile = strdup("lock");
	if (!core_config.help_path) core_config.help_path = strdup("help/");

	config_update_table(core_config_vars, config_root, "eggdrop", 0, NULL);

	config2_link(0, &logging_type, &core_config.logging);

	return (0);
}

int core_config_save(void)
{
	config_update_table(core_config_vars, config_root, "eggdrop", 0, NULL);

	config2_sync(0, &logging_type, &core_config.logging);

	config_save("eggdrop", configfile);

	return (0);
}
