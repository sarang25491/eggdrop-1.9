#include <stdio.h>
#include <stdlib.h>
#include "xml.h"

int xml_write_node(FILE *fp, xml_node_t *node, int indent)
{
	char *tabs;
	int i;

	tabs = (char *)malloc(indent+1);
	memset(tabs, '\t', indent);
	tabs[indent] = 0;
	if (node->name) {
		if (node->type == 0) fprintf(fp, "%s<%s", tabs, node->name);
		else fprintf(fp, "%s<?%s", tabs, node->name);
	}
	else if (node->type == 2) {
		/* comment like <!-- ... --> */
		fprintf(fp, "%s<!--%s-->\n", tabs, node->text);
		return(0);
	}

	for (i = 0; i < node->nattributes; i++) {
		fprintf(fp, " %s='%s'", node->attributes[i].name, node->attributes[i].value);
	}
	if (node->len > 50 || node->nchildren) {
		if (node->name) fprintf(fp, ">\n");
	}
	else if (node->len > 0) {
		/* If it's just small text and no children... */
		fprintf(fp, ">%s</%s>\n", node->text, node->name);
		return(0);
	}
	else {
		free(tabs);
		if (node->type == 0) fprintf(fp, " />\n");
		else fprintf(fp, " ?>\n");
		return(0);
	}

	if (node->len) {
		fprintf(fp, "%s\t%s\n", tabs, node->text);
	}
	if (node->name) indent++;
	for (i = 0; i < node->nchildren; i++) {
		xml_write_node(fp, node->children[i], indent);
	}
	if (node->name) {
		fprintf(fp, "%s</%s>\n", tabs, node->name);
	}
	return(0);
}
