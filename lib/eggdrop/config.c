/* config.c: config file
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
 */

#ifndef lint
static const char rcsid[] = "$Id: config.c,v 1.10 2004/10/17 05:14:06 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>

/* Config bind tables. */
static bind_table_t *BT_config_str = NULL,
	*BT_config_int = NULL,
	*BT_config_save = NULL;

/* Keep track of whether we're in a nested get/set, so that we don't
 * trigger binds when we shouldn't. */
static int gconfig_level = 0;

typedef struct {
	char *handle;
	void *root;
} config_handle_t;

static config_handle_t *roots = NULL;
static int nroots = 0;

static void config_delete_var(void *client_data);

int config_init(void)
{
	BT_config_str = bind_table_add(BTN_CONFIG_STR, 2, "ss", MATCH_MASK, BIND_STACKABLE);	/* DDD	*/
	BT_config_int = bind_table_add(BTN_CONFIG_INT, 2, "si", MATCH_MASK, BIND_STACKABLE);	/* DDD	*/
	BT_config_save = bind_table_add(BTN_CONFIG_SAVE, 1, "s", MATCH_MASK, BIND_STACKABLE);	/* DDD	*/
	return(0);
}

int config_shutdown(void)
{
	int i;

	for (i = nroots - 1; i >= 0; i--) {
		config_delete_root(roots[i].handle);
	}
	nroots = 0;

	bind_table_del(BT_config_str);
	bind_table_del(BT_config_int);
	bind_table_del(BT_config_save);
	return (0);
}


void *config_get_root(const char *handle)
{
	int i;

	for (i = 0; i < nroots; i++) {
		if (!strcasecmp(handle, roots[i].handle)) return(roots[i].root);
	}
	return(NULL);
}

int config_set_root(const char *handle, void *config_root)
{
	egg_assert_val(handle != NULL, -1);
	egg_assert_val(config_root != NULL, -1);

	roots = realloc(roots, sizeof(*roots) * (nroots+1));
	roots[nroots].handle = strdup(handle);
	roots[nroots].root = config_root;
	nroots++;
	return(0);
}

int config_delete_root(const char *handle)
{
	int i;

	for (i = 0; i < nroots; i++) {
		if (!strcasecmp(handle, roots[i].handle)) {
			if (roots[i].handle) free(roots[i].handle);
			xml_node_delete_callbacked(roots[i].root, config_delete_var);
			memmove(roots+i, roots+i+1, sizeof(*roots) * (nroots-i-1));
			if (--nroots == 0)
				free(roots);
			return (0);	
		}
	}


	return(-1);
}

void *config_load(const char *fname)
{
	xml_node_t *root;

	root = xml_parse_file(fname);
	if (!root) {
		/* XXX: do not use putlog here since it might not be initialized yet */
		fprintf(stderr, "ERROR\n");
		fprintf(stderr, "\tFailed to load config '%s': %s\n\n", fname, xml_last_error());
		return NULL;
	}

	return(root);
}

int config_save(const char *handle, const char *fname)
{
	void *root;

	root = config_get_root(handle);
	if (root) bind_check(BT_config_save, NULL, handle, handle);
	if (!fname) fname = "config.xml";

	if (xml_save_file(fname, root, XML_INDENT) == 0)
		return (0);
	
	putlog(LOG_MISC, "*", _("Failed to save config '%s': %s"), fname, xml_last_error());

	return (-1);
}

static void config_delete_var(void *client_data)
{
	config_var_t *var = client_data;

	switch (var->type) {
		case (CONFIG_STRING):
			free(*(char **)var->ptr);
			var->ptr = NULL;
			break;
		case (CONFIG_INT):
			break;
	}
}

int config_destroy(void *config_root)
{
	xml_node_t *root = config_root;

	xml_node_delete_callbacked(root, config_delete_var);

	return(0);
}

void *config_lookup_section(void *config_root, ...)
{
	va_list args;
	xml_node_t *root = config_root;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	return(root);
}

void *config_exists(void *config_root, ...)
{
	va_list args;
	xml_node_t *root = config_root;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 0);
	va_end(args);
	return(root);
}

int config_get_int(int *intptr, void *config_root, ...)
{
	va_list args;
	xml_node_t *root = config_root;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 0);
	va_end(args);

	if (!root) {
		*intptr = 0;
		return(-1);
	}
	return xml_node_get_int(intptr, root, NULL);
}

int config_get_str(char **strptr, void *config_root, ...)
{
	va_list args;
	xml_node_t *root = config_root;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 0);
	va_end(args);

	if (!root) {
		*strptr = NULL;
		return(-1);
	}
	return xml_node_get_str(strptr, root, NULL);
}

int config_set_int(int intval, void *config_root, ...)
{
	va_list args;
	xml_node_t *root = config_root;
	config_var_t *var;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	if (!gconfig_level) {
		char *name;
		int r;

		name = xml_node_fullname(root);
		gconfig_level++;
		r = bind_check(BT_config_int, NULL, name, name, intval);
		gconfig_level--;
		free(name);
		if (r & BIND_RET_BREAK) return(-1);
	}

	var = root->client_data;
	if (var) {
		if (var->type == CONFIG_INT) *(int *)var->ptr = intval;
	}
	xml_node_set_int(intval, root, NULL);
	return(0);
}

int config_set_str(const char *strval, void *config_root, ...)
{
	va_list args;
	xml_node_t *root = config_root;
	config_var_t *var;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	if (!gconfig_level) {
		char *name;
		int r;

		name = xml_node_fullname(root);
		gconfig_level++;
		r = bind_check(BT_config_str, NULL, name, name, strval);
		gconfig_level--;
		free(name);
		if (r & BIND_RET_BREAK) return(-1);
	}
	var = root->client_data;
	if (var) {
		if (var->type == CONFIG_STRING && *(char **)var->ptr != strval) str_redup(var->ptr, strval);
		else if (var->type == CONFIG_INT) *(int *)var->ptr = atoi(strval);
	}
	xml_node_set_str(strval, root, NULL);
	return(0);
}

int config_link_table(config_var_t *table, void *config_root, ...)
{
	xml_node_t *node, *root = config_root;
	va_list args;

	gconfig_level++;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	if (!root) {
		gconfig_level--;
		return(-1);
	}

	while (table->name) {
		node = xml_node_lookup(root, 1, table->name, 0, NULL);
		node->client_data = table;
		if (table->type == CONFIG_INT) config_get_int(table->ptr, root, table->name, 0, NULL);
		else if (table->type == CONFIG_STRING) {
			char *str;
			config_get_str(&str, root, table->name, 0, NULL);
			str_redup(table->ptr, str);
		}
		table++;
	}

	gconfig_level--;
	return(0);
}

int config_update_table(config_var_t *table, void *config_root, ...)
{
	xml_node_t *node, *root = config_root;
	va_list args;

	gconfig_level++;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	if (!root) {
		gconfig_level--;
		return(-1);
	}

	while (table->name) {
		node = xml_node_lookup(root, 1, table->name, 0, NULL);
		node->client_data = table;
	
		switch (table->type) {

			case (CONFIG_INT):
				config_set_int(*(int *)table->ptr, root, table->name, 0, NULL);
				break;

			case (CONFIG_STRING):
				config_set_str(*(char **)table->ptr, root, table->name, 0, NULL);
				break;
		}

		table++;
	}

	gconfig_level--;
	return(0);
}

int config_unlink_table(config_var_t *table, void *config_root, ...)
{
	xml_node_t *node, *root = config_root;
	va_list args;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	if (!root) return(-1);

	while (table->name) {
		node = xml_node_lookup(root, 0, table->name, 0, NULL);
		if (node) 
			xml_node_delete_callbacked(node, config_delete_var);
		table++;
	}
	return(0);
}
