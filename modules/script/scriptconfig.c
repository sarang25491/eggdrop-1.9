/* scriptconfig.c: config-file-related scripting commands
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
static const char rcsid[] = "$Id: scriptconfig.c,v 1.9 2004/10/17 05:14:07 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>

static void *resolve_path(void *root, char *path)
{
	char *save, *work, *period;

	if (!root) return(NULL);

	work = strdup(path);
	save = work;
	while ((period = strchr(work, '.'))) {
		*period = 0;
		root = config_lookup_section(root, work, 0, NULL);
		if (!root) goto done;
		work = period+1;
	}
	root = config_lookup_section(root, work, 0, NULL);
done:
	free(save);
	return(root);
}

static char *script_config_get(char *handle, char *path)
{
	void *root;
	char *str = NULL;

	if (!handle) handle = "eggdrop";

	root = config_get_root(handle);
	root = resolve_path(root, path);
	if (!root) return(NULL);

	config_get_str(&str, root, NULL);
	return(str);
}

static int script_config_set(char *handle, char *path, char *val)
{
	void *root;

	if (!handle) handle = "eggdrop";

	root = config_get_root(handle);
	root = resolve_path(root, path);
	if (!root) return(-1);

	config_set_str(val, root, NULL);
	return(0);
}

static int script_config_exists(char *handle, char *path)
{
	void *root;
	char *save, *work, *period;
	int r = 0;

	if (!handle) handle = "eggdrop";

	root = config_get_root(handle);
	if (!root) return(0);

	work = strdup(path);
	save = work;
	while ((period = strchr(work, '.'))) {
		*period = 0;
		if (!config_exists(root, work, 0, NULL)) goto notfound;
		root = config_lookup_section(root, work, 0, NULL);
		if (!root) goto notfound;
		work = period+1;
	}
	r = (config_exists(root, work, 0, NULL) == NULL) ? 0 : 1;

notfound:
	free(save);
	return(r);
}

static int script_config_load(char *handle, char *fname)
{
	void *root;

	root = config_load(fname);
	if (!root) return(-1);
	config_set_root(handle, root);
	return(0);
}

script_command_t script_config_cmds[] = {
	{"", "config_exists", script_config_exists, NULL, 1, "ss", "?handle? path", SCRIPT_INTEGER, SCRIPT_VAR_ARGS|SCRIPT_VAR_FRONT},	/* DDD */
	{"", "config_get", script_config_get, NULL, 1, "ss", "?handle? path", SCRIPT_STRING, SCRIPT_VAR_ARGS|SCRIPT_VAR_FRONT}, /* DDD */
	{"", "config_set", script_config_set, NULL, 2, "sss", "?handle? path value", SCRIPT_INTEGER, SCRIPT_VAR_ARGS|SCRIPT_VAR_FRONT}, /* DDD */
	{"", "config_load", script_config_load, NULL, 2, "ss", "handle filename", SCRIPT_INTEGER, 0}, /* DDD */
	{"", "config_save", config_save, NULL, 2, "ss", "handle filename", SCRIPT_INTEGER, 0}, /* DDD */
	{0}
};
