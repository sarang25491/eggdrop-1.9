#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "xml.h"
#include "eggconfig.h"

int config_init()
{
	return(0);
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

int config_get_int(int *intptr, void *config_root, ...)
{
	va_list args;
	xml_node_t *root = config_root;
	int intval;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	if (!xml_node_get_int(&intval, root, NULL)) {
		*intptr = intval;
	}
	else {
		intval = *intptr;
		xml_node_set_int(intval, root, NULL);
	}
	return(0);
}

int config_get_str(char **strptr, void *config_root, ...)
{
	va_list args;
	xml_node_t *root = config_root;
	char *str;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	if (!xml_node_get_str(&str, root, NULL)) {
		if (*strptr) free(*strptr);
		*strptr = strdup(str);
	}
	else {
		str = *strptr;
		if (str) xml_node_set_str(str, root, NULL);
	}
	return(0);
}

int config_set_int(int intval, void *config_root, ...)
{
	va_list args;
	xml_node_t *root = config_root;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	return xml_node_set_int(intval, root, NULL);
}

int config_set_str(char *strval, void *config_root, ...)
{
	va_list args;
	xml_node_t *root = config_root;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	return xml_node_set_str(strval, root, NULL);
}

int config_get_table(config_var_t *table, void *config_root, ...)
{
	xml_node_t *root = config_root;
	va_list args;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	if (!root) return(-1);

	while (table->name) {
		if (table->type == CONFIG_INT) config_get_int(table->ptr, root, table->name, 0, NULL);
		else if (table->type == CONFIG_STRING) config_get_str(table->ptr, root, table->name, 0, NULL);
		table++;
	}
	return(0);
}

int config_set_table(config_var_t *table, void *config_root, ...)
{
	xml_node_t *root = config_root;
	va_list args;

	va_start(args, config_root);
	root = xml_node_vlookup(root, args, 1);
	va_end(args);

	if (!root) return(-1);

	while (table->name) {
		if (table->type == CONFIG_INT) config_set_int(*(char **)table->ptr, root, table->name, 0, NULL);
		else if (table->type == CONFIG_STRING) config_set_str(*(int *)table->ptr, root, table->name, 0, NULL);
		table++;
	}
	return(0);
}
