/* xmlwrite.c: xml writing functions
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
static const char rcsid[] = "$Id: xmlwrite.c,v 1.7 2004/06/22 23:20:23 wingman Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <eggdrop/memory.h>
#include <eggdrop/xml.h>

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
		free(tabs);
		return(0);
	}

	for (i = 0; i < node->nattributes; i++) fprintf(fp, " %s='%s'", node->attributes[i].name, node->attributes[i].value);
	if (node->len > 50 || node->nchildren) {
		if (node->name) fprintf(fp, ">\n");
	}
	else if (node->len > 0) {
		/* If it's just small text and no children... */
		fprintf(fp, ">%s</%s>\n", node->text, node->name);
		free(tabs);
		return(0);
	}
	else {
		free(tabs);
		if (node->type == 0) fprintf(fp, " />\n");
		else fprintf(fp, " ?>\n");
		return(0);
	}

	if (node->len) fprintf(fp, "%s\t%s\n", tabs, node->text);
	if (node->name) indent++;
	for (i = 0; i < node->nchildren; i++) xml_write_node(fp, node->children[i], indent);
	if (node->name) fprintf(fp, "%s</%s>\n", tabs, node->name);
	free(tabs);
	return(0);
}

int xml_save(FILE *fd, xml_node_t *node, int options)
{
	return xml_write_node(fd, node, (options & XML_INDENT) ? 1 : 0);	
}

int xml_save_file(const char *file, xml_node_t *node, int options)
{
	FILE *fd;
	int ret;

	fd = fopen(file, "r");
	if (fd == NULL)
		return -1;
	ret = xml_save(fd, node, options);
	fclose(fd);

	return ret;
}

#if 0
int xml_save_str(char **str, xml_node_t *node, int options)
{
	return -1; /* not supported */
}
#endif
