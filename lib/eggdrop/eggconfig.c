#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <eggdrop/eggdrop.h>

/* Config bind tables. */
static bind_table_t *BT_config_str = NULL,
	*BT_config_int = NULL;

/* Keep track of whether we're in a nested get/set, so that we don't
 * trigger binds when we shouldn't. */
static int gconfig_level = 0;

typedef struct {
	char *handle;
	void *root;
} config_handle_t;

static config_handle_t *roots = NULL;
int nroots = 0;

int config_init()
{
	BT_config_str = bind_table_add("config_str", 2, "ss", MATCH_MASK, BIND_STACKABLE);
	BT_config_int = bind_table_add("config_int", 2, "si", MATCH_MASK, BIND_STACKABLE);
	return(0);
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
			memmove(roots+i, roots+i+1, sizeof(*roots) * (nroots-i-1));
			nroots--;
			return(0);
		}
	}
	return(-1);
}

void *config_load(const char *fname)
{
	xml_node_t *root;

	root = xml_node_new();
	xml_read(root, fname);
	return(root);
}

int config_save(void *config_root, const char *fname)
{
	xml_node_t *root = config_root;
	FILE *fp;

	if (!fname) fname = "config.xml";
	fp = fopen(fname, "w");
	if (!fp) return(-1);
	xml_write_node(fp, root, 0);
	fclose(fp);
	return(0);
}

int config_destroy(void *config_root)
{
	xml_node_t *root = config_root;

	xml_node_destroy(root);
	free(root);
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
	char *str;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 0);
	va_end(args);

	if (!xml_node_get_str(&str, root, NULL)) {
		str_redup(strptr, str);
		return(0);
	}
	return(-1);
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
		r = bind_check(BT_config_int, name, name, intval);
		gconfig_level--;
		free(name);
		if (r & BIND_RET_BREAK) return(-1);
	}

	var = root->client_data;
	if (var) *(int *)var->ptr = intval;
	xml_node_set_int(intval, root, NULL);
	return(0);
}

int config_set_str(char *strval, void *config_root, ...)
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
		r = bind_check(BT_config_str, name, name, strval);
		gconfig_level--;
		free(name);
		if (r & BIND_RET_BREAK) return(-1);
	}
	var = root->client_data;
	if (var) str_redup(var->ptr, strval);
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
		else if (table->type == CONFIG_STRING) config_get_str(table->ptr, root, table->name, 0, NULL);
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
		if (table->type == CONFIG_INT) config_set_int(*(int *)table->ptr, root, table->name, 0, NULL);
		else if (table->type == CONFIG_STRING) config_set_str(*(char **)table->ptr, root, table->name, 0, NULL);
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
		if (node) xml_node_destroy(node);
		table++;
	}
	return(0);
}
