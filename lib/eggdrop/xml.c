/* xml.c: xml parser
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
static const char rcsid[] = "$Id: xml.c,v 1.18 2004/06/30 17:07:20 wingman Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>			/* isdigit		*/

#include "xml.h"			/* prototypes		*/

#define XML_PATH_SEPARATOR '/'

xml_amp_conversion_t builtin_conversions[] =
{
	{"quot",	'"'  },
	{"lt",		'<'  },
	{"gt",		'>'  },
	{"amp",		'&'  },
	{"apos",	'\'' },
	{"nbsp",	' '  },
	{0}
};

char *last_error = NULL;

const char *xml_last_error(void)
{
	return last_error;
}

/* Get a new, blank node. */
xml_node_t *xml_node_new(void)
{
	xml_node_t *node;

	node = (xml_node_t *)malloc(sizeof(xml_node_t));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(xml_node_t));

	/* default value is STRING NULL */
	node->data.value.s_val = NULL;
	node->data.type = VARIANT_STRING;

	/* default type is element */	
	node->type = XML_ELEMENT;
	
	return node;
}

xml_node_t *xml_create_element(const char *name)
{
	xml_node_t *node;
	
	node = xml_node_new();
	node->name = (name) ? strdup(name) : NULL;
	
	return node;
}

xml_node_t *xml_create_attribute(const char *name)
{
	xml_attr_t *attr;
	
	attr = malloc(sizeof(xml_attr_t));
	if (attr == NULL)
		return NULL;
	memset(attr, 0, sizeof(xml_attr_t));
	
	attr->type = XML_ATTRIBUTE;
	attr->name = (name) ? strdup(name) : NULL;
	
	return (xml_node_t *)attr;
}

void xml_node_delete(xml_node_t *node)
{
	xml_node_delete_callbacked(node, NULL);
}

void xml_node_delete_callbacked(xml_node_t *node, void (*callback)(void *))
{
	int i;

	if (node->client_data && callback)
		callback(node->client_data);

	if (node->parent) {
		xml_node_t *parent = node->parent;
		for (i = 0; i < parent->nchildren; i++) {
			if (parent->children[i] == node) break;
		}
		if (parent->nchildren == 1) {
			free(parent->children);	parent->children = NULL;
		} else {
			memmove(parent->children+i, parent->children+i+1, sizeof(node) * (parent->nchildren-i-1));
		}
		parent->nchildren--;
	}

	/* free attributes */
	for (i = 0; i < node->nattributes; i++) {
		if (node->attributes[i].name) free(node->attributes[i].name);
		if (node->attributes[i].data.type == VARIANT_STRING) {
			if (node->attributes[i].data.value.s_val)
				free(node->attributes[i].data.value.s_val);
		}
	}
	if (node->attributes) free(node->attributes);

	/* free children */
	for (i = 0; i < node->nchildren; i++) {
		node->children[i]->parent = NULL;
		xml_node_delete_callbacked(node->children[i], callback);
	}
	if (node->children) free(node->children);

	/* only string needs to be free'd */
	if (node->data.type == VARIANT_STRING)
		free(node->data.value.s_val);

	free(node->name);
	free(node);
}

const char *xml_node_type(xml_node_t *node)
{
	switch (node->type) {
		case (XML_DOCUMENT): return "XML_DOCUMENT";
		case (XML_ELEMENT): return "XML_ELEMENT";
		case (XML_CDATA_SECTION): return "XML_CDATA_SECTION";
		case (XML_PROCESSING_INSTRUCTION): return "XML_PROCESSING_INSTRUCTION";
		case (XML_COMMENT): return "XML_COMMENT";
		case (XML_ATTRIBUTE): return "XML_ATTRIBUTE";
	}

	return "unknown";
}

xml_node_t *xml_node_vlookup(xml_node_t *root, va_list args, int create)
{
	char *path;
	int index;

	for (; root;) {
		path = va_arg(args, char *);
		if (!path) break;
		index = va_arg(args, int);
		root = xml_node_path_lookup(root, path, index, create);
	}
	return(root);
}

xml_node_t *xml_node_lookup(xml_node_t *root, int create, ...)
{
	va_list args;
	xml_node_t *node;

	va_start(args, create);
	node = xml_node_vlookup(root, args, create);
	va_end(args);
	return(node);
}

xml_node_t *xml_node_path_lookup(xml_node_t *root, const char *path, int index, int create)
{
	int i, thisindex, len;
	xml_node_t *child, *newchild;
	const char *next;
	char *name, *sep, buf[512];

	for (; root && path; path = next) {
		/* Get the next path element. */
		sep = strchr(path, '.');
		if (sep) {
			next = sep+1;
			len = sep - path;
		}
		else {
			next = NULL;
			len = strlen(path);
		}

		/* If it's empty, skip it, otherwise copy it. */
		if (!len) continue;
		else if (len > sizeof(buf) - 10) {
			name = malloc(len+1);
		}
		else {
			name = buf;
		}
		memcpy(name, path, len);
		name[len] = 0;

		/* Ok, now see if there's an [index] at the end. The length
		 * has to be at least 4, because it's like "a[x]" at least. */
		thisindex = 0;
		if (len > 3 && name[len-1] == ']') {
			sep = strrchr(name, '[');
			if (sep) {
				*sep = 0;
				name[len-1] = 0;
				thisindex = atoi(sep+1);
			}
		}

		/* If it's the last path element, add the index param. */
		if (!next) thisindex += index;

		child = NULL;
		if (name[0] == '@') {
			for (i = 0; i < root->nattributes; i++) {
				if (strcasecmp(root->attributes[i].name, name + 1) == 0) {
					child = (xml_node_t *)&root->attributes[i];
					break;
				}
			}
		} else {
			for (i = 0; i < root->nchildren; i++) {			
				if (root->children[i]->name && !strcasecmp(root->children[i]->name, name)) {
					if (thisindex-- > 0) continue;
					child = root->children[i];
					break;
				}
			}
		}

		if (!child && create) {
			if (0 == strcmp(name, "."))
				return NULL;
				
			do {		
				if (name[0] == '@') {
					root->attributes = realloc(root->attributes,
						(root->nattributes + 1) * sizeof(xml_attr_t));
					newchild = (xml_node_t *)&root->attributes[root->nattributes++];
					memset(newchild, 0, sizeof(xml_attr_t));
					newchild->name = strdup(name + 1);
					newchild->type = XML_ATTRIBUTE;
				} else {		
					newchild = xml_create_element(name);
					xml_node_append(root, newchild);
				}									
				
				child = newchild;
			} while (thisindex-- > 0);
		}
		
		root = child;
		if (root && root->type == XML_ATTRIBUTE)
			break;
			
		if (name != buf)
			free(name);
	}
	return(root);
}

char *xml_node_fullname(xml_node_t *thenode)
{
	xml_node_t *node;
	char *name, *name2;
	int len, total_len;

	len = total_len = 0;
	name = calloc(1, 1);
	for (node = thenode; node; node = node->parent) {
		if (!node->name) continue;

		/* Length of name plus the dot. */
		len = strlen(node->name) + 1;

		/* Name plus the null. */
		name2 = malloc(len + total_len + 1);

		/* Create new name. */
		sprintf(name2, "%s.%s", node->name, name);
		free(name);
		name = name2;
		total_len += len;
	}
	if (total_len > 0) name[total_len-1] = 0;
	return(name);
}

int xml_node_get_int(int *value, xml_node_t *node, ...)
{
	va_list args;

	va_start(args, node);
	node = xml_node_vlookup(node, args, 0);
	va_end(args);
	if (node) {
		*value = variant_get_int(&node->data, 0);
		return(0);
	}
	*value = 0;
	return(-1);
}

int xml_node_get_str(char **str, xml_node_t *node, ...)
{
	va_list args;

	va_start(args, node);
	node = xml_node_vlookup(node, args, 0);
	va_end(args);
	if (node) {
		*str = (char *)variant_get_str(&node->data, NULL);
		return(0);
	}
	*str = NULL;
	return(-1);
}

int xml_node_set_int(int value, xml_node_t *node, ...)
{
	va_list args;

	va_start(args, node);
	node = xml_node_vlookup(node, args, 1);
	va_end(args);
	if (!node) return(-1);

	variant_set_int(&node->data, value);

	return(0);
}

int xml_node_set_str(const char *str, xml_node_t *node, ...)
{
	va_list args;

	va_start(args, node);
	node = xml_node_vlookup(node, args, 1);
	va_end(args);
	if (!node) return(-1);

	variant_set_str(&node->data, str);

	return(0);
}

xml_node_t *xml_root_element(xml_node_t *node)
{
	 int i;

	 if (node == NULL)
		  return NULL;

	 while (node && node->parent)
		  node = node->parent;

	 for (i = 0; i < node->nchildren; i++)
		if (node->children[i]->type == XML_ELEMENT)
			return node->children[i];
	 return NULL;
}

xml_attr_t *xml_node_lookup_attr(xml_node_t *node, const char *name)
{
	int i;

	for (i = 0; i < node->nattributes; i++) {
		if (strcasecmp(node->attributes[i].name, name) == 0)
			return &node->attributes[i];
	}

	return NULL;
}

void xml_node_remove(xml_node_t *parent, xml_node_t *child)
{
	int i;

	for (i = 0; i < parent->nchildren; i++) {
		if (parent->children[i] != child)
			continue;

		if (parent->nchildren == 1) {
			free(parent->children); parent->children = NULL;
		} else {
			memmove(parent->children + i,
				parent->children + i + 1,
					parent->nchildren -i - 1);
			parent->children = realloc(parent->children, (parent->nchildren - 1) * sizeof(xml_node_t *));
		}
		parent->nchildren--;

		child->prev = NULL;
		child->parent = NULL;
		child->next = NULL;
		return;
	}
}

void xml_node_append(xml_node_t *parent, xml_node_t *child)
{	
	if (child->type == XML_ATTRIBUTE) {
		xml_node_append_attr(parent, (xml_attr_t *)child);
		return;		
	}
	
	/* remove from previous parent */
	if (child->parent)
		xml_node_remove(child->parent, child);

	parent->children = realloc(parent->children, sizeof(xml_node_t *) * (parent->nchildren + 1));
	parent->children[parent->nchildren] = child;

	if (parent->nchildren > 0)
		parent->children[parent->nchildren - 1]->next = child;

	/* set new parent */
	child->parent = parent;

	parent->nchildren++;
}

void xml_node_append_attr (xml_node_t *node, xml_attr_t *attr)
{
	xml_attr_t *a;
		
	a = xml_node_lookup_attr (node, attr->name);
	if (a == NULL) {
		node->attributes = (xml_attr_t *)realloc(
			node->attributes, sizeof(xml_attr_t) * (node->nattributes+1));
		a = &node->attributes[node->nattributes++];
	}
	
	a->type = XML_ATTRIBUTE;
	a->name = attr->name;
	
	memcpy(&a->data, &attr->data, sizeof(attr->data));
}

void xml_node_remove_attr (xml_node_t *node, xml_attr_t *attr)
{
	int i;

	for (i = 0; i < node->nattributes; i++) {
		if (strcasecmp(node->attributes[i].name, attr->name) == 0) {
			if (node->nattributes == 1) {
				free (node->attributes); node->attributes = NULL;
			} else {
				memmove (node->attributes + i,
					node->attributes + i + 1,
						sizeof (xml_attr_t) * 
							node->nattributes - i - 1);
			}
			node->nattributes--;
			return;
		}
	}
}

static xml_attr_t *xml_node_create_attr(xml_node_t *node, const char *name)
{
	xml_attr_t *attr;
	
	node->attributes = (xml_attr_t *)realloc(
		node->attributes, sizeof(xml_attr_t) * (node->nattributes+1));
	attr = &node->attributes[node->nattributes++];
	memset(attr, 0, sizeof(xml_attr_t));
		
	attr->name = strdup(name);
	
	return attr;
}


#define XML_DEFINE_GET_AND_SET(name, type) \
	void xml_set_text_## name(xml_node_t *node, type value) \
	{ \
		if (node == NULL) return; \
		variant_set_## name(&node->data, value); \
	} \
	\
	type xml_get_text_## name(xml_node_t *node, type nil) \
	{ \
		if (node == NULL) return nil; \
		return variant_get_## name(&node->data, nil); \
	} \
	type xml_get_attr_## name(xml_node_t *node, const char *n, type nil) \
	{ \
		xml_attr_t *attr; \
		attr = xml_node_lookup_attr(node, n); \
		if (attr == NULL) return nil; \
		return variant_get_## name(&attr->data, nil); \
	} \
	void xml_set_attr_## name(xml_node_t *node, const char *n, type value) \
	{ \
		xml_attr_t *attr; \
		attr = xml_node_lookup_attr(node, n); \
		if (attr == NULL) attr = xml_node_create_attr(node, n); \
		variant_set_## name(&attr->data, value); \
	}

XML_DEFINE_GET_AND_SET(str, const char *)
XML_DEFINE_GET_AND_SET(int, int)
XML_DEFINE_GET_AND_SET(bool, int)
XML_DEFINE_GET_AND_SET(ts, time_t)

void xml_dump_data(variant_t *data)
{
	switch (data->type) {

		case (VARIANT_STRING):
			printf("%s (STRING)\n", data->value.s_val);
			break;

		case (VARIANT_INT):
			printf("%i (INT)\n", data->value.i_val);
			break;

		case (VARIANT_BOOL):
			printf("%s (BOOL)\n", (data->value.b_val) ? "yes" : "no");
			break;

		case (VARIANT_TIMESTAMP):
			printf("%li (TIMESTAMP)\n", data->value.ts_val);
			break;
	}
}

void xml_dump(xml_node_t *node)
{
	int i;

	printf("%s %s\n", node->name, xml_node_type(node));
	for (i = 0; i < node->nattributes; i++) {
		printf("\t%s: ", node->attributes[i].name);
		xml_dump_data(&node->attributes[i].data);	
	}
	for (i = 0; i < node->nchildren; i++) {
		xml_dump(node->children[i]);
	}
}

void xml_node_remove_by_name(xml_node_t *parent, const char *name)
{
	int i;

	for (i = parent->nchildren - 1; i >= 0; i--) {
		if (0 == strcmp(parent->children[i]->name, name))
			xml_node_remove(parent, parent->children[i]);
	}
}
