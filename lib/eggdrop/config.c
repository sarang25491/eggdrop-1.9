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
static const char rcsid[] = "$Id: config.c,v 1.8 2004/06/30 17:10:46 wingman Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
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
int nroots = 0;

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

/* XXX: remove this */
xml_node_t *doc;

void *config_load(const char *fname)
{
	if (xml_load_file(fname, &doc, XML_TRIM_TEXT) != 0) {
		/* XXX: do not use putlog here since it might not be initialized yet */
		fprintf(stderr, "ERROR\n");
		fprintf(stderr, "\tFailed to load config '%s': %s\n\n", fname, xml_last_error());
		return NULL;
	}

	return xml_root_element(doc);
}

int config_save(const char *handle, const char *fname)
{
	void *root;

	root = config_get_root(handle);
	if (root) bind_check(BT_config_save, NULL, handle, handle);
	if (!fname) fname = "config.xml";

	if (xml_save_file(fname, doc, XML_INDENT) == 0)
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
	int intval;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 0);
	va_end(args);

	if (!xml_node_get_int(&intval, root, NULL)) {
		*intptr = intval;
		return(0);
	}
	return(-1);
}

int config_get_str(char **strptr, void *config_root, ...)
{
	va_list args;
	xml_node_t *root = config_root;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 0);
	va_end(args);

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

/****************************************************************************/


config_type_t CONFIG_TYPE_STRING    = { "#string#", sizeof(char *), NULL };
config_type_t CONFIG_TYPE_INT       = { "#int#", sizeof(int), NULL };
config_type_t CONFIG_TYPE_BOOL      = { "#bool#", sizeof(int), NULL };
config_type_t CONFIG_TYPE_TIMESTAMP = { "#timestamp#", sizeof(time_t), NULL };

static int link_node(xml_node_t *node, config_type_t *type, void *addr);
static int link_addr(xml_node_t *node, variant_t *data, config_variable_t *var, void **addr);

static int sync_node(xml_node_t *node, config_type_t *type, void *addr);
static int sync_addr(xml_node_t *node, variant_t *data, config_variable_t *var, void *addr);

int config2_link(int config, config_type_t *type, void *addr)
{
	int ret;
	ret = link_node(xml_node_path_lookup(xml_root_element(doc),type->name, 0, 1), type, addr);
	return ret;
}

int config2_sync(int config, config_type_t *type, void *addr)
{
	int ret;
	ret = sync_node(xml_node_path_lookup(xml_root_element(doc),type->name, 0, 1), type, addr);
	return ret;
}

static int sync_node(xml_node_t *node, config_type_t *type, void *addr)
{
	config_variable_t *var, *e;
	unsigned char *bytes;
	xml_node_t *child, *c;
	variant_t *data;
	
	bytes = (unsigned char *)addr;
	if (bytes == NULL)
		return (-1);	
	
	for (var = type->vars; var->type; var++) {
		if (var->path == NULL)
			child = node;			
		else
			child = xml_node_path_lookup(node, var->path, 0, 1);		
		data = &child->data;												
		switch (var->modifier) {
		
			case (CONFIG_NONE):
			{
				if (sync_addr(child, data, var, bytes) != 0)
					continue;					
				bytes += var->type->size;
				break;
			}
				
			case (CONFIG_ENUM):
			{
				int val, i;
				
				/* remove all nodes */
				for (i = child->nchildren - 1; i >= 0; i--) {
					if (0 == strcmp(child->children[i]->name, var->type->name)) {
						xml_node_remove(child, child->children[i]);
					}
				}

				val = *(int *)bytes; bytes += sizeof(int);

				for (e = var->type->vars; e->path; e++) {
					if (val & e->modifier) {
						c = xml_create_element(var->type->name);
						xml_set_text_str(c, e->path);
						xml_node_append(child, c);
					}
				}

				break;
			}

			case (CONFIG_LIST):
			case (CONFIG_LIST_DL):
			{
				void *head_ptr, *item, *next;
				unsigned char *cur;
			
				head_ptr = *((void **)bytes); bytes += sizeof(void *);
	
				/* remove all nodes */
				xml_node_remove_by_name(child, var->type->name);
		
				for (item = head_ptr; item; ) {
					cur = (unsigned char *)item;
	
					/* skip prev ptr */
					if (var->modifier == CONFIG_LIST_DL)
						cur += sizeof(void *);

					/* save next and skip next ptr */
					next = *((void **)cur); cur += sizeof(void *);

					c = xml_create_element(var->type->name);
					sync_node(c, var->type, cur);
					xml_node_append(child, c);

					item = next;
				}				
				break;
			}

			case (CONFIG_ARRAY):
			{
				void **items; 
				int length, i; 
							
				/* items */	
				items = *((void ***)bytes); bytes += sizeof(void **);
				
				/* length */
				length = *((int *)bytes); bytes += sizeof(int);
				
				/* remove all nodes */
				xml_node_remove_by_name(child, var->type->name);
				
				for (i = 0; i < length; i++) {
					c = xml_create_element(var->type->name);
					xml_node_append(child, c);
					sync_node(c, var->type, &items[i]);
				}
								
				break;
			}			
							
		}
	}
			
	return (0);
}

static int sync_addr(xml_node_t *node, variant_t *data, config_variable_t *var, void *addr)
{
	if (var->type == &CONFIG_TYPE_STRING)
		variant_set_str(data, *((char **)addr));
	else if (var->type == &CONFIG_TYPE_INT)
		variant_set_int(data, *((int *)addr));
	else if (var->type == &CONFIG_TYPE_BOOL)
		variant_set_bool(data, *((int *)addr));
	else if (var->type == &CONFIG_TYPE_TIMESTAMP)
		variant_set_ts(data, *((time_t *)addr));
	else {
		if (sync_node(node, var->type, addr) != 0)
			return (-1);
	}
	
	return (0);
}

static int link_addr(xml_node_t *node, variant_t *data, config_variable_t *var, void **addr)
{	
	if (var->type == &CONFIG_TYPE_STRING)
		*((char **)addr) = (char *)variant_get_str(data, NULL);
	else if (var->type == &CONFIG_TYPE_INT)
		*((int *)addr) = variant_get_int(data, MIN_INT);
	else if (var->type == &CONFIG_TYPE_BOOL)
		*((int *)addr) = variant_get_bool(data, 0);
	else if (var->type == &CONFIG_TYPE_TIMESTAMP)
		*((time_t *)addr) = variant_get_ts(data, (time_t)0);
	else {
		if (link_node(node, var->type, addr) != 0)
			return (-1);
	}

	return (0);
}

static int link_node(xml_node_t *node, config_type_t *type, void *addr)
{
	config_variable_t *var, *e;
	xml_node_t *child;
	variant_t *data;
	unsigned char *bytes;
	
	bytes = (unsigned char *)addr;
	if (bytes == NULL)
		return (-1);
	memset(bytes, 0, type->size);
	
	for (var = type->vars; var->type; var++) {				
		if (var->path == NULL)
			child = node;
		else
			child = xml_node_path_lookup(node, var->path, 0, 1);			
		data = &child->data;
		
		switch (var->modifier) {
		
			case (CONFIG_NONE):
			{
				if (link_addr(child, data, var, (void *)bytes) != 0)
					;					
				bytes += var->type->size;
				break;
			}
				
			case (CONFIG_ENUM):
			{
				int i;
				int *en;

				en = (int *)bytes; bytes += sizeof(int);

				for (i = 0; i < child->nchildren; i++) {
					xml_node_t *c = child->children[i];
					if (0 != strcmp(c->name, var->type->name))
						continue;

					for (e = var->type->vars; e->path; e++) {
						if (0 == strcmp(xml_get_text_str(c, ""), e->path)) {
							(*en) |= e->modifier;
						}	
					}	
				}
				break;
			}

			case (CONFIG_LIST):
			case (CONFIG_LIST_DL):
			{
				int i;
				void **head_ptr;
				unsigned char *cur, *prev;

				prev = NULL;
				head_ptr = (void **)bytes; bytes += sizeof(void *);

				for (i = 0; i < child->nchildren; i++) {
					xml_node_t *c = child->children[i];
					if (0 != strcmp(c->name, var->type->name)) {
						continue;
					}

					/* allocate memory for current item */
					cur = (unsigned char *)malloc(var->type->size);
					if (cur == NULL)
						return (-1);
					memset(cur, 0, var->type->size);		
					
					cur += sizeof(void *); /* skip either next or prev ptr */
					if (var->modifier == CONFIG_LIST_DL)
						cur += sizeof(void *); /* skip next ptr */

					/* link it */
					if (link_addr(c, &c->data, var, (void *)cur) != 0) {
						/* failed to link, free this item and continue
 						 * with next one */
						free(cur);
						continue;
					}

					cur -= sizeof(void *);
					if (var->modifier == CONFIG_LIST_DL)
						cur -= sizeof(void *); 

					/* set head if not yet set */
					if (*head_ptr == NULL) {
						*head_ptr = cur;
					}

					/* set current item's prev */
					if (prev) {
						if (var->modifier == CONFIG_LIST_DL) {
							*((void **)bytes) = prev;
						}	
					}
		
					/* set previous item's next */
					if (prev) {
						if (var->modifier == CONFIG_LIST_DL) {
							prev += sizeof(void *); /* skip prev item */
							*((void **)prev) = cur;
						} else
							*((void **)prev) = cur;
					}

					/* remember prev */
					prev = cur;
				}
				break;
			}

			case (CONFIG_ARRAY):
			{
				void ***items; 
				int *length, i; 
							
				/* items */	
				items = (void ***)bytes; bytes += sizeof(void *);
				
				/* length */
				length = (int *)bytes; bytes += sizeof(int);
					
				/* allocate enough memory (though not all memory by used here
				 * since not all children must be items of this array */
				*items = malloc(child->nchildren * var->type->size);
				memset(*items, 0, child->nchildren * var->type->size);
				
				for (i = 0; i < child->nchildren; i++) {
					if (0 != strcmp(child->children[i]->name, var->type->name))
						continue;
						
					/* now link array item with the correspondending xml node */
					if (link_addr(child->children[i], &child->children[i]->data, var, &(*items)[*length]) == 0) {
						(*length)++;
					}
				}
			}				
		}				
	}

	return (0);
}


