/* xml.h: header for xml.c
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
 *
 * $Id: xml.h,v 1.6 2004/03/01 22:58:32 stdarg Exp $
 */

#ifndef _EGG_XML_H_
#define _EGG_XML_H_

typedef struct {
	char *name;
	char *value;
	int len;
} xml_attribute_t;

typedef struct xml_node_b {
	struct xml_node_b *next, *prev;
	char *name;

	char *text;
	int len;
	int whitespace;

	xml_attribute_t *attributes;
	int nattributes;

	struct xml_node_b *parent;
	struct xml_node_b **children;
	int nchildren;

	int type; /* 0 = normal, 1 = decl (<?xml ... ?>), 2 = CDATA,
			3 = comment, 4 = .. nothing yet. */

	char *data;

	void *client_data;
} xml_node_t;

typedef struct {
	char *key;
	char value;
} xml_amp_conversion_t;

xml_node_t *xml_node_new();
int xml_node_destroy(xml_node_t *node);
xml_node_t *xml_node_add(xml_node_t *parent, xml_node_t *child);
xml_node_t *xml_node_vlookup(xml_node_t *root, va_list args, int create);
xml_node_t *xml_node_lookup(xml_node_t *root, int create, ...);
xml_node_t *xml_node_path_lookup(xml_node_t *root, const char *path, int index, int create);
char *xml_node_fullname(xml_node_t *thenode);
int xml_node_get_int(int *value, xml_node_t *node, ...);
int xml_node_get_str(char **str, xml_node_t *node, ...);
int xml_node_set_int(int value, xml_node_t *node, ...);
int xml_node_set_str(const char *str, xml_node_t *node, ...);
xml_attribute_t *xml_attribute_add(xml_node_t *node, xml_attribute_t *attr);
xml_attribute_t *xml_attribute_get(xml_node_t *node, char *name);
int xml_attribute_set(xml_node_t *node, xml_attribute_t *attr);
int xml_attr_get_int(xml_node_t *node, const char *name);
char *xml_attr_get_str(xml_node_t *node, const char *name);
int xml_write_node(FILE *fp, xml_node_t *node, int indent);
int xml_read_node(xml_node_t *parent, char **data);
int xml_read(xml_node_t *root, const char *fname);

#endif /* !_EGG_XML_H_ */
