/* chanserv.c: channel services
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
static const char rcsid[] = "$Id: chanserv.c,v 1.1 2004/12/13 15:02:29 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "chanserv.h"

chan_config_t chan_config = {0};
egg_server_api_t *server = NULL;

static config_var_t chan_config_vars[] = {
	{"enforcebans", &chan_config.enforcebans, CONFIG_INT},
	{0}
};

EXPORT_SCOPE int chanserv_LTX_start(egg_module_t *modinfo);
static int chanserv_close(int why);

static int chanserv_init()
{
	server = module_get_api("server", 1, 0);
	events_init();
	return(0);
}

static int chanserv_close(int why)
{
	return(0);
}

int chanserv_LTX_start(egg_module_t *modinfo)
{
	void *config_root;

	modinfo->name = "chanserv";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "channel services";
	modinfo->close_func = chanserv_close;

	/* Set defaults. */
	memset(&chan_config, 0, sizeof(chan_config));

	/* Link our vars in. */
	config_root = config_get_root("eggdrop");
	config_link_table(chan_config_vars, config_root, "chanserv", 0, NULL);
	config_update_table(chan_config_vars, config_root, "chanserv", 0, NULL);

	chanserv_init();
	return(0);
}
