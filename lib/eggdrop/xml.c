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
static const char rcsid[] = "$Id: xml.c,v 1.14 2004/06/22 23:20:23 wingman Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>				/* isdigit		*/

#include <eggdrop/eggdrop.h>			/* egg_return_if_fail	*/
#include <eggdrop/memory.h>			/* malloc, free		*/
#include <eggdrop/memutil.h>			/* str_redup		*/
#include <eggdrop/xml.h>			/* prototypes		*/

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

/* Get a new, blank node. */
xml_node_t *xml_node_new(void)
{
	xml_node_t *node;

	node = (xml_node_t *)malloc (sizeof (xml_node_t));
	if (node == NULL)
		return NULL;
	memset (node, 0, sizeof (xml_node_t));
	
	return node;
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
	if (node->text) free(node->text);
	if (node->name) free(node->name);
	for (i = 0; i < node->nattributes; i++) {
		if (node->attributes[i].name) free(node->attributes[i].name);
		if (node->attributes[i].value) free(node->attributes[i].value);
	}
	if (node->attributes) free(node->attributes);
	for (i = 0; i < node->nchildren; i++) {
		node->children[i]->parent = NULL;
		xml_node_delete_callbacked(node->children[i], callback);
	}
	if (node->children) free(node->children);
	free(node);
}

/* Append a node to another node's children. */
xml_node_t *xml_node_add(xml_node_t *parent, xml_node_t *child)
{
	xml_node_t *newnode, *node;
	int i;

	newnode = malloc(sizeof(*newnode));
	memcpy(newnode, child, sizeof(*child));
	newnode->parent = parent;

	parent->children = realloc(parent->children, sizeof(child) * (parent->nchildren+1));
	parent->children[parent->nchildren] = newnode;
	parent->nchildren++;
	newnode->next = newnode->prev = NULL;

	if (child->name) {
		for (i = parent->nchildren-2; i >= 0; i--) {
			node = parent->children[i];
			if (node->name && !strcasecmp(node->name, newnode->name)) {
				node->next = child;
				newnode->prev = node;
				break;
			}
		}
	}
	return(newnode);
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
	xml_node_t *child, newchild;
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
		for (i = 0; i < root->nchildren; i++) {
			if (root->children[i]->name && !strcasecmp(root->children[i]->name, name)) {
				if (thisindex-- > 0) continue;
				child = root->children[i];
				break;
			}
		}

		if (!child && create) {
			do {
				memset(&newchild, 0, sizeof(newchild));
				newchild.name = strdup(name);
				child = xml_node_add(root, &newchild);
			} while (thisindex-- > 0);
		}
		root = child;

		if (name != buf) free(name);
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
	if (node && node->text) {
		*value = atoi(node->text);
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
	if (node && node->text) {
		*str = node->text;
		return(0);
	}
	*str = NULL;
	return(-1);
}

int xml_node_set_int(int value, xml_node_t *node, ...)
{
	va_list args;
	char str[100];

	va_start(args, node);
	node = xml_node_vlookup(node, args, 1);
	va_end(args);
	if (!node) return(-1);
	if (node->text) free(node->text);
	snprintf(str, sizeof(str), "%d", value);
	str[sizeof(str)-1] = 0;
	node->text = strdup(str);
	node->len = strlen(str);
	return(0);
}

int xml_node_set_str(const char *str, xml_node_t *node, ...)
{
	va_list args;

	va_start(args, node);
	node = xml_node_vlookup(node, args, 1);
	va_end(args);
	if (!node) return(-1);

	str_redup(&node->text, str);
	node->len = (str) ? strlen(str) : -1;

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

xml_node_t *xml_node_select(xml_node_t *parent, const char *path)
{
	xml_node_t *child;
	char *ptr;
	size_t pos, len;
	int i, j, idx;
				
	do {	
		child = NULL;
		idx = 0;
		pos = 0;
		len = strlen (path);
		
		/* get next end of path */
		ptr = strchr (path, XML_PATH_SEPARATOR);
		if (ptr != NULL)
			pos = (size_t)(ptr - path);
		else
			pos = len - 1;
		len = pos;
		
		/* check if we have an index given <path>[0] */
		if (path[pos] == ']') {			
			ptr = strchr (path, '[');
			if (ptr == NULL)
				return NULL;
			pos  = (size_t)(ptr - path);
			len = pos + 1;

			/* here we parse the actual index */
			for (idx = 0; path[len] != ']'; len++) {
				/* invalid index */
				if (!isdigit (path[len]))
					return NULL;
					
				/* convert current char to int */
				idx = (idx * 10) + ((int)path[len] - '0');				
			}
		}
		
		/* this is the main search routine */
		for (j = 0, i = 0; i < parent->nchildren; i++) {
			child = parent->children[i];
			if (0 == strncmp(child->name, path, pos)) {
				/* the name matched, but is it the correct
				 * index? */
				if (j++ == idx) {
					parent = child;	break;
				}
			}
			child = NULL;
		}
		
		/* no such child found */
		if (child == NULL)
			return NULL;
				
		/* skip token + path separator */
		path += len + 1;
	} while (*path);
	
	return parent;
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

void xml_node_remove_child(xml_node_t *parent, xml_node_t *child)
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
		return;
	}
}

void xml_node_append_child(xml_node_t *parent, xml_node_t *child)
{
	egg_return_if_fail(parent != NULL);
	egg_return_if_fail(child != NULL);

	/* remove from previous parent */
	if (child->parent)
		xml_node_remove_child(child->parent, child);

	parent->children = realloc(parent->children, parent->nchildren + 1);
	parent->children[parent->nchildren++] = child;

	/* set new parent */
	child->parent = parent;
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
	a->name = attr->name;
	a->value = attr->value;
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
	
	attr->name = strdup(name);
	attr->value = NULL;
	
	return attr;
}

void xml_attr_set_int(xml_node_t *node, const char *name, int value)
{
	xml_attr_t *attr;
	char buf[100];
	
	attr = xml_node_lookup_attr (node, name);
	if (attr == NULL)
		attr = xml_node_create_attr (node, name);

	snprintf (buf, sizeof(buf), "%d", value);
	str_redup(&attr->value, buf);
}

int xml_attr_get_int(xml_node_t *node, const char *name)
{
	xml_attr_t *attr;

	attr = xml_node_lookup_attr(node, name);
	if (attr && attr->value)
		return atoi(attr->value);
	return (0);
}

void xml_attr_set_str(xml_node_t *node, const char *name, const char *value)
{
	xml_attr_t *attr;
	
	attr = xml_node_lookup_attr (node, name);
	if (value != NULL) {
		if (attr == NULL)
			attr = xml_node_create_attr (node, name);
		str_redup(&attr->value, value);
	} else {
		if (attr != NULL)
			xml_node_remove_attr (node, attr);
	}
}

char *xml_attr_get_str(xml_node_t *node, const char *name)
{
	xml_attr_t *attr;

	attr = xml_node_lookup_attr(node, name);
	if (attr && attr->value)
		return (attr->value);
		
	return (NULL);
}
