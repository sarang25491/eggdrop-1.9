#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xml.h"

int config_init()
{
	return(0);
}

void *config_load(const char *fname)
{
	xml_node_t *root;

	root = xml_node_new();
	xml_read(root, fname);
	return(0);
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
}

int config_get_str(char **strptr, void *config_root, ...)
{
}
