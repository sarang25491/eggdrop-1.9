#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xml.h"

#ifdef AMP_CHARS
xml_amp_conversion_t builtin_conversions[] = {
	{"quot", '"'},
	{"lt", '<'},
	{"gt", '>'},
	{"amp", '&'},
	{"apos", '\''},
	{"nbsp", ' '},
	{0}
};
#endif

/* Get a new, blank node. */
xml_node_t *xml_node_new()
{
	xml_node_t *node;

	node = (xml_node_t *)calloc(sizeof(*node), 1);
	return(node);
}

/* Delete a node and its children. */
int xml_node_destroy(xml_node_t *node)
{
	int i;

	if (node->parent) {
		xml_node_t *parent = node->parent;
		for (i = 0; i < parent->nchildren; i++) {
			if (parent->children[i] == node) break;
		}
		memmove(parent->children+i, parent->children+i+1, sizeof(node) * (parent->nchildren-i-1));
		parent->nchildren--;
	}
	if (node->text) free(node->text);
	for (i = 0; i < node->nattributes; i++) {
		if (node->attributes[i].name) free(node->attributes[i].name);
		if (node->attributes[i].value) free(node->attributes[i].value);
	}
	if (node->attributes) free(node->attributes);
	for (i = 0; i < node->nchildren; i++) {
		node->children[i]->parent = NULL;
		xml_node_destroy(node->children[i]);
	}
	if (node->children) free(node->children);
	free(node);
	return(0);
}

/* Append a node to another node's children. */
xml_node_t *xml_node_add(xml_node_t *parent, xml_node_t *child)
{
	xml_node_t *newnode;

	newnode = malloc(sizeof(*newnode));
	memcpy(newnode, child, sizeof(*child));
	newnode->parent = parent;
	parent->children = realloc(parent->children, sizeof(child) * (parent->nchildren+1));
	parent->children[parent->nchildren] = newnode;
	parent->nchildren++;
	return(newnode);
}

xml_node_t *xml_node_vlookup(xml_node_t *root, va_list args, int create)
{
	char *path;
	int i, index;
	xml_node_t *child, newchild;

	for (; root;) {
		path = va_arg(args, char *);
		if (!path) break;
		index = va_arg(args, int);
		child = NULL;
		for (i = 0; i < root->nchildren; i++) {
			if (!strcasecmp(root->children[i]->name, path)) {
				if (index-- > 0) continue;
				child = root->children[i];
				break;
			}
		}
		if (!child && create) {
			memset(&newchild, 0, sizeof(newchild));
			newchild.name = strdup(path);
			child = xml_node_add(root, &newchild);
		}
		root = child;
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

/* Just add an attribute to the end of a node's attribute list. */
xml_attribute_t *xml_attribute_add(xml_node_t *node, xml_attribute_t *attr)
{
	node->attributes = (xml_attribute_t *)realloc(node->attributes, sizeof(*attr) * (node->nattributes+1));
	memcpy(node->attributes+node->nattributes, attr, sizeof(*attr));
	node->nattributes++;
	return(node->attributes+(node->nattributes-1));
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

int xml_node_set_str(char *str, xml_node_t *node, ...)
{
	va_list args;

	va_start(args, node);
	node = xml_node_vlookup(node, args, 1);
	va_end(args);
	if (!node) return(-1);
	if (node->text) free(node->text);
	if (str) {
		node->text = strdup(str);
		node->len = strlen(str);
	}
	else {
		node->text = NULL;
		node->len = -1;
	}
	return(0);
}

xml_attribute_t *xml_attr_lookup(xml_node_t *node, const char *name)
{
	int i;

	for (i = 0; i < node->nattributes; i++) {
		if (!strcasecmp(node->attributes[i].name, name)) break;
	}
	if (i < node->nattributes) return(node->attributes+i);
	return(NULL);
}

int xml_attr_get_int(xml_node_t *node, const char *name)
{
	xml_attribute_t *attr;

	attr = xml_attr_lookup(node, name);
	if (attr && attr->value) return atoi(attr->value);
	return(0);
}

char *xml_attr_get_str(xml_node_t *node, const char *name)
{
	xml_attribute_t *attr;

	attr = xml_attr_lookup(node, name);
	if (attr && attr->value) return(attr->value);
	return(NULL);
}
