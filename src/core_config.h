/* core_config.h: header for core_config.c
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
 * $Id: core_config.h,v 1.10 2004/06/28 17:36:34 wingman Exp $
 */

#ifndef _EGG_CORE_CONFIG_H_
#define _EGG_CORE_CONFIG_H_

#include "logfile.h"				/* logging_t		*/


typedef struct {
	/* General bot stuff. */
	char *botname;	/* Name of the bot as seen by user. */
	char *userfile;	/* File we store users in. */
	char *lockfile;	/* To make sure only one instance runs. */

	/* Owner stuff. */
	char *owner;
	char *admin;

	/* Paths. */
	char *help_path;
	char *temp_path;
	char *text_path;
	char *module_path;

	/* Logging. */
	logging_t logging;

	/* Info to display in whois command. */
	char *whois_items;

	/* Other. */
	int die_on_sigterm;
} core_config_t;

extern core_config_t core_config;

int core_config_init(const char *);
int core_config_save(void);

#endif /* !_EGG_CORE_CONFIG_H_ */
