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
static const char rcsid[] = "$Id: core_config.c,v 1.10 2003/12/16 21:45:35 wcc Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include "core_config.h"

extern char configfile[];
core_config_t core_config = {0};
void *config_root = NULL;

static config_var_t core_config_vars[] = {
	/* General bot stuff. */
	{"botname", &core_config.botname, CONFIG_STRING},
	{"userfile", &core_config.userfile, CONFIG_STRING},

	/* The owner. */
	{"owner", &core_config.owner, CONFIG_STRING},
	{"admin", &core_config.admin, CONFIG_STRING},

	/* Paths. */
	{"help_path", &core_config.help_path, CONFIG_STRING},
	{"temp_path", &core_config.temp_path, CONFIG_STRING},
	{"text_path", &core_config.text_path, CONFIG_STRING},
	{"module_path", &core_config.module_path, CONFIG_STRING},

	/* Logfile. */
	{"logfile_suffix", &core_config.logfile_suffix, CONFIG_STRING},
	{"max_logsize", &core_config.max_logsize, CONFIG_INT},
	{"switch_logfiles_at", &core_config.switch_logfiles_at, CONFIG_INT},
	{"keep_all_logs", &core_config.keep_all_logs, CONFIG_INT},
	{"quick_logs", &core_config.quick_logs, CONFIG_INT},

	/* Other. */
	{"die_on_sigterm", &core_config.die_on_sigterm, CONFIG_INT},
	{0}
};

void core_config_init(const char *fname)
{
	/* Set default vals. */
	memset(&core_config, 0, sizeof(core_config));

	/* Hook the owner variable into libeggdrop. */
	egg_setowner(&core_config.owner);

	core_config.botname = strdup("eggdrop");
	core_config.userfile = strdup("users.xml");

	core_config.logfile_suffix = strdup(".%d%b%Y");

	config_root = config_load(fname);
	config_set_root("eggdrop", config_root);
	config_link_table(core_config_vars, config_root, "eggdrop", 0, NULL);
}

void core_config_save()
{
	config_update_table(core_config_vars, config_root, "eggdrop", 0, NULL);
	config_save(config_root, configfile);
}
