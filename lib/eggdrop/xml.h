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
 * $Id: xml.h,v 1.22 2004/10/01 15:31:18 stdarg Exp $
 */

#ifndef _EGG_XML_H_
#define _EGG_XML_H_

#include <stdio.h>			/* FILE			*/
#include <stdarg.h>			/* va_list		*/

#define XML_NONE	(1 << 0)
#define XML_INDENT	(1 << 1)
#define XML_TRIM_TEXT	(1 << 2)

typedef enum {
	XML_ELEMENT = 0,		/* <element /> 		*/
	XML_PROCESSING_INSTRUCTION,	/* <?pi?>		*/
	XML_COMMENT,			/* <!-- comment -->	*/
	XML_CDATA_SECTION,		/* <![CDATA[ ... ]]>	*/
	/* XML_TEXT, */			/* not supported	*/
	XML_ATTRIBUTE,			/* var=""		*/
	XML_DOCUMENT,			/* none			*/
	/* XML_DOCUMENT_FRAGMENT, */	/* not supported	*/
} xml_node_type_t;

typedef struct {
	char *name;
	char *value;
	int len;
} xml_attr_t;

typedef struct {
	char *name;
	char *value;
} xml_entity_t;

typedef struct xml_node {
	xml_node_type_t type;
	char *name;

	char *text;
	int len;

	void *client_data;
	
	struct xml_node *next, *prev;
	struct xml_node *next_sibling, *prev_sibling;
	struct xml_node *parent, *children, *last_child;
	int nchildren;

	xml_attr_t **attributes;
	int nattributes;	
} xml_node_t;

typedef struct {
	char *filename;
	xml_node_t *root;
} xml_document_t;

xml_node_t *xml_node_vlookup(xml_node_t *root, va_list args, int create);
xml_node_t *xml_node_lookup(xml_node_t *root, int create, ...);
xml_node_t *xml_node_path_lookup(xml_node_t *root, const char *path, int index, int create);

char *xml_node_fullname(xml_node_t *node);

int xml_node_get_str(char **str, xml_node_t *node, ...);
char *xml_node_str(xml_node_t *node, char *def);
int xml_node_set_str(const char *str, xml_node_t *node, ...);

int xml_node_get_int(int *value, xml_node_t *node, ...);
int xml_node_int(xml_node_t *node, int def);
int xml_node_set_int(int value, xml_node_t *node, ...);

int xml_node_get_vars(xml_node_t *node, const char *fmt, ...);

xml_node_t *xml_node_new(void);
void xml_node_free(xml_node_t *node);
void xml_node_unlink(xml_node_t *node);
void xml_node_delete(xml_node_t *node);
void xml_node_delete_callbacked(xml_node_t *node, void (*callback)(void *));
void xml_doc_delete(xml_node_t *root);
void xml_node_append(xml_node_t *parent, xml_node_t *child);

xml_node_t *xml_root_element(xml_node_t *node);

xml_attr_t *xml_attr_new(char *name, char *value);
void xml_attr_free(xml_attr_t *attr);
int xml_node_append_attr(xml_node_t *node, xml_attr_t *attr);

xml_node_t *xml_parse_file(const char *fname);
int xml_save_file(const char *file, xml_node_t *node, int options);
void xml_set_error(const char *err);
const char *xml_last_error(void);

#endif /* !_EGG_XML_H_ */
