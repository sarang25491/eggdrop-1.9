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
static const char rcsid[] = "$Id: xmlwrite.c,v 1.11 2004/09/26 09:42:09 stdarg Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <eggdrop/eggdrop.h>

#define XML_INDENT_CHAR	'\t'

static int write_options = XML_NONE;

static int xml_write_node(FILE *fp, xml_node_t *node);

static int level = 0;
static char indent[32] = {0};

static int xml_write_children(FILE *fp, xml_node_t *node)
{
	int ret = 0;
	const char *text;

	/* init indent */
	memset(indent, '\t', sizeof(indent));

	indent[++level] = 0;

	/* XXX: encode text */
	text = node->text;
	if (text && *text) fprintf(fp, "%s%s\n", indent, text);

        for (node = node->children; node; node = node->next) {
		fprintf(fp, "%s", indent);
                if ((ret = xml_write_node(fp, node)) == -1) break;
	}

	indent[--level] = 0;

        return ret;
}

static int xml_write_attributes(FILE *fp, xml_node_t *node)
{
	int i;
	xml_attr_t *attr;

	for (i = 0; i < node->nattributes; i++) {
		attr = node->attributes[i];
		if (attr->value) fprintf(fp, " %s=\"%s\"", attr->name, attr->value);
	}

	return (0);
}

static int xml_write_element(FILE *fp, xml_node_t *node)
{
	fprintf(fp, "<%s", node->name);
	
	if (xml_write_attributes(fp, node) == -1) return(-1);

	if (node->nchildren == 0) {
		if (node->text) {
			fprintf(fp, ">%s</%s>\n", node->text, node->name);
		} else {
			fprintf(fp, "/>\n");
		}

		return (0);
	}

	fprintf(fp, ">\n");
	if (xml_write_children(fp, node) != 0) return(-1);

	fprintf(fp, "%s</%s>\n", indent, node->name);
	return(0);
}

static int xml_write_document(FILE *fp, xml_node_t *node)
{
	int ret;

	ret = 0;
        for (node = node->children; node; node = node->next) {
                if ((ret = xml_write_node(fp, node)) != 0)
                        break;
        }
	return ret;
}

static int xml_write_comment(FILE *fp, xml_node_t *node)
{
	/* XXX: that's wrong, text needs to encoded */
	fprintf(fp, "<!--%s-->\n", node->text);
	return(0);
}

static int xml_write_cdata_section(FILE *fp, xml_node_t *node)
{
	fprintf(fp, "<![CDATA[%s]]>\n", node->text);
	return(0);
}

static int xml_write_processing_instruction(FILE *fp, xml_node_t *node)
{
	fprintf(fp, "<?%s", node->name);

	if (xml_write_attributes(fp, node) != 0) return(-1);

	fprintf(fp, "?>\n");

	return (0);
}

static int xml_write_node(FILE *fp, xml_node_t *node)
{
	switch (node->type) {
	
		case (XML_DOCUMENT):
			return xml_write_document(fp, node);

		case (XML_ELEMENT):
			return xml_write_element(fp, node);

		case (XML_COMMENT):
			return xml_write_comment(fp, node);
	
		case (XML_CDATA_SECTION):
			return xml_write_cdata_section(fp, node);

		case (XML_PROCESSING_INSTRUCTION):
			return xml_write_processing_instruction(fp, node);
			
		case (XML_ATTRIBUTE):
			/* just ignore this */
			return (0);
	}

	xml_set_error("unknown node type");

	return (-1);
}

int xml_save_file(const char *file, xml_node_t *node, int options)
{
	FILE *fp;
	int ret;

	/* clear any last error */
	xml_set_error(NULL);

	fp = fopen(file, "w");

	if (!fp) {
		xml_set_error("failed to open file");
		return -1;
	}

	write_options = options;
	ret = xml_write_node(fp, node);
	write_options = XML_NONE;

	fclose(fp);

	return ret;
}
