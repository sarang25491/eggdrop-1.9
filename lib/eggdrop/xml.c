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
static const char rcsid[] = "$Id: xml.c,v 1.27 2007/08/19 19:49:17 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

static char *last_error = NULL;

const char *xml_last_error(void)
{
	return last_error;
}

void xml_set_error(const char *err)
{
	str_redup(&last_error, err);
}

/*!
 * \brief Get a new, blank node.
 *
 * \return The new, empty node.
 */

xml_node_t *xml_node_new()
{
	xml_node_t *node;

	node = calloc(sizeof(*node), 1);
	
	return node;
}

/*!
 * \brief Get a new, named node.
 *
 * \param name The name of the node.
 *
 * \return The new, empty node.
 */

xml_node_t *xml_node_new_named(const char *name)
{
	xml_node_t *node = xml_node_new();
	node->name = strdup(name);
	return(node);
}

/*!
 * \brief Free all memory associated with a node.
 *
 * \param node The node to be free'd.
 */

void xml_node_free(xml_node_t *node)
{
	xml_attr_t *attr;
	int i;

	if (node->name) free(node->name);
	if (node->text) free(node->text);
	for (i = 0; i < node->nattributes; i++) {
		attr = node->attributes[i];
		xml_attr_free(attr);
	}
	if (node->attributes) free(node->attributes);
	free(node);
}

/*!
 * \brief Unlinks a XML node from a tree.
 *
 * Removes a node from a tree. Unlinks the node from its parent and siblings,
 * but all nodes descending from this one will still be attached.
 *
 * \param node The node to unlink.
 */

void xml_node_unlink(xml_node_t *node)
{
	xml_node_t *parent = node->parent;

	/* Unlink from parent. */
	if (parent) {
		parent->nchildren--;
		if (parent->children == node) parent->children = node->next;
		if (parent->last_child == node) parent->last_child = node->prev;

		node->parent = NULL;
	}

	/* Unlink from node list. */
	if (node->prev) node->prev->next = node->next;
	if (node->next) node->next->prev = node->prev;

	/* Unlink from sibling list. */
	if (node->prev_sibling) node->prev_sibling->next_sibling = node->next_sibling;
	if (node->next_sibling) node->next_sibling->prev_sibling = node->prev_sibling;
}

/*!
 * \brief Deletes a node.
 *
 * This function is a wrapper for xml_node_delete_callbacked() without a
 * callback.
 *
 * \todo Might be better as a macro.
 *
 * \param node The node to delete.
 */

void xml_node_delete(xml_node_t *node)
{
	xml_node_delete_callbacked(node, NULL);
}

/*!
 * \brief Deletes a node and executes a callback function.
 *
 * This function deletes a node and all of its children and attributes. For
 * every deleted node the callback function is executed.
 *
 * \param node The node to delete.
 * \param callback The callback function to execute. May be NULL.
 */

void xml_node_delete_callbacked(xml_node_t *node, void (*callback)(void *))
{
	xml_node_t *child, *child_next;

	if (node->client_data && callback) callback(node->client_data);

	xml_node_unlink(node);

	/* Delete children. */
	for (child = node->children; child; child = child_next) {
		child_next = child->next;
		child->parent = NULL;
		xml_node_delete_callbacked(child, callback);
	}

	/* Free memory taken by node. */
	xml_node_free(node);
}

/*!
 * \brief Deletes an entire tree.
 *
 * This function will delete the entire tree a node is in. The node does not
 * have to be the root of that tree, any node of the tree will do.
 *
 * \param root Any node in the tree that will be deleted. Does not really have to be the root node.
 */

void xml_doc_delete(xml_node_t *root)
{
	while (root->parent) root = root->parent;
	xml_node_delete(root);
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

/*!
 * \brief Get a child of a node by its name.
 *
 * This function searches a tree beginning with a given node and its children
 * for a child by a given name.
 *
 * This name can be very complex with serveral layers of the tree seperated by
 * '.' and an index with "[x]. Example: "layer1.layer2[3].layer3".
 *
 * \param root The node to start searching from.
 * \param path The name or path of the child to search for.
 * \param index Added to the index of every layer. Best used for a simple query without multiple layers.
 * \param create If this is non-zero and the node was not found it will be created.
 *
 * \return The node searched for or, if it was not found and \e create was zero, NULL.
 */

xml_node_t *xml_node_path_lookup(xml_node_t *root, const char *path, int index, int create)
{
	int thisindex, len;
	xml_node_t *child;
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

		for (child = root->children; child; child = child->next) {
			if (child->name && !strcasecmp(child->name, name)) break;
		}

		while (child && thisindex > 0) {
			thisindex--;
			child = child->next_sibling;
		}

		if (!child && create) {
			do {		
				child = xml_node_new();
				child->type = XML_ELEMENT;
				child->name = strdup(name);
				xml_node_append(root, child);
			} while (thisindex-- > 0);
		}
		if (name != buf) free(name);

		root = child;
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
		*value = strtol(node->text, NULL, 0);
		return(0);
	}
	*value = 0;
	return(-1);
}

/*!
 * \brief Get an int or the default value from a node.
 *
 * Will convert the text of a given node to an int and return the value. If
 * no node was given or the node does not contain text, a default value will
 * be returned.
 *
 * \param node The node to extract the int from.
 * \param def The default value to return in case the node does not contain text.
 *
 * \return Either the value from the node or the default value.
 */

int xml_node_int(xml_node_t *node, int def)
{
	int value;
	char *ptr;

	if (!node || !node->text) return(def);
	value = strtol(node->text, &ptr, 0);
	if (!ptr || *ptr) return(def);
	else return(value);
}

int xml_node_get_str(char **str, xml_node_t *node, ...)
{
	va_list args;

	va_start(args, node);
	node = xml_node_vlookup(node, args, 0);
	va_end(args);
	if (node) {
		*str = node->text;
		return(0);
	}
	*str = NULL;
	return(-1);
}

/*!
 * \brief Get a string or the default value from a node.
 *
 * Will return the text of a given node as a string. If no node was given or
 * the node does not contain text, a default value will be returned.
 *
 * \param node The node to return the string from.
 * \param def The default value to return in case the node does not contain text.
 *
 * \return Either the value from the node or the default value.
 */

char *xml_node_str(xml_node_t *node, char *def)
{
	if (!node || !node->text) return(def);
	else return(node->text);
}

int xml_node_set_int(int value, xml_node_t *node, ...)
{
	char buf[32];
	va_list args;

	va_start(args, node);
	node = xml_node_vlookup(node, args, 1);
	va_end(args);
	if (!node) return(-1);

	snprintf(buf, sizeof(buf), "%d", value);
	str_redup(&node->text, buf);

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

	return(0);
}

int xml_node_get_vars(xml_node_t *node, const char *fmt, ...)
{
	va_list args;
	char *name, **strptr;
	int *intptr;
	xml_node_t **nodeptr;

	va_start(args, fmt);
	while (*fmt) {
		name = va_arg(args, char *);
		switch (*fmt) {
			case 's':
				strptr = va_arg(args, char **);
				xml_node_get_str(strptr, node, name, 0, 0);
				break;
			case 'i':
				intptr = va_arg(args, int *);
				xml_node_get_int(intptr, node, name, 0, 0);
				break;
			case 'n':
				nodeptr = va_arg(args, xml_node_t **);
				*nodeptr = xml_node_path_lookup(node, name, 0, 0);
				break;
		}
		fmt++;
	}
	va_end(args);
	return(0);
}

int xml_node_set_vars(xml_node_t *node, const char *fmt, ...)
{
	va_list args;
	char *name, *strval;
	int intval;

	va_start(args, fmt);
	while (*fmt) {
		name = va_arg(args, char *);
		switch (*fmt) {
			case 's':
				strval = va_arg(args, char *);
				xml_node_set_str(strval, node, name, 0, 0);
				break;
			case 'i':
				intval = va_arg(args, int);
				xml_node_set_int(intval, node, name, 0, 0);
				break;
		}
		fmt++;
	}
	va_end(args);
	return(0);
}

xml_node_t *xml_root_element(xml_node_t *node)
{
	 if (node == NULL) return NULL;

	 /* Zoom up to the document. */
	 while (node && node->parent) node = node->parent;

	 /* Find first element. */
	 node = node->children;
	 while (node && node->type != XML_ELEMENT) node = node->next;
	 return node;
}

/*!
 * \brief Attaches a node to a parent.
 *
 * Sets a node as a child of a parent node. This node must not have a parent!
 * If it does, use xml_node_unlink() first!
 *
 * \param parent The node that will be the parent.
 * \param child The node that will be attached to the parent.
 */

void xml_node_append(xml_node_t *parent, xml_node_t *child)
{	
	xml_node_t *node;

	child->parent = parent;
	parent->nchildren++;

	if (!parent->children) {
		parent->children = child;
	}
	else {
		parent->last_child->next = child;
		child->prev = parent->last_child;
	}

	parent->last_child = child;

	if (!child->name) return;
	for (node = child->prev; node; node = node->prev) {
		if (node->name && !strcasecmp(node->name, child->name)) {
			node->next_sibling = child;
			child->prev_sibling = node;
			break;
		}
	}
}

/*!
 * \brief Create a new attribute.
 *
 * Creates a new attribute and fills its fields. The strings used as
 * parameters to this function will not be dup'd but used as is. The
 * parameters must not be NULL.
 *
 * \param name The name of the attribute.
 * \param value The value of the attribute. An empty string is fine, NULL is not.
 *
 * \return The new attribute.
 */

xml_attr_t *xml_attr_new(char *name, char *value)
{
	xml_attr_t *attr;

	attr = malloc(sizeof(*attr));
	attr->name = name;
	attr->value = value;
	attr->len = strlen(value);
	return(attr);
}

/*!
 * \brief Frees a node's attribute.
 *
 * This will delete an attribute and free all it's memory.
 *
 * \param attr The attribute to free.
 */

void xml_attr_free(xml_attr_t *attr)
{
	if (attr->name) free(attr->name);
	if (attr->value) free(attr->value);
	free(attr);
}

/*!
 * \brief Append an attribute to a node.
 *
 * \param node The node to appand the attribute to.
 * \param attr The attribute to append.
 *
 * \return Always 0.
 */

int xml_node_append_attr(xml_node_t *node, xml_attr_t *attr)
{
	node->attributes = realloc(node->attributes, sizeof(*node->attributes) * (node->nattributes+1));
	node->attributes[node->nattributes++] = attr;
	return(0);
}

/*!
 * \brief Searches for an attribute with a given name.
 *
 * Searches all attributes of a node for a given name. If an attribute with
 * that name is found, it's returned.
 *
 * \param node The node to search.
 * \param name The name of the attribute to search for.
 *
 * \return The attribute if found, otherwise NULL.
 */

xml_attr_t *xml_attr_lookup(xml_node_t *node, const char *name)
{
	int i;

	for (i = 0; i < node->nattributes; i++) {
		if (!strcasecmp(node->attributes[i]->name, name)) return(node->attributes[i]);
	}
	return(NULL);
}

/*!
 * \brief Get an int or the default value from an attribute of a node.
 *
 * Will return the int value of the text of a given attribute of a node. If
 * the node does not have an attribute of the given name or the attribute does
 * not contain text, a default value will be returned.
 *
 * \param node The node to return the int from.
 * \param name The name of the attribute.
 * \param def The default value to return in case no text is available.
 *
 * \return Either the value of the attribute or the default value.
 */

int xml_attr_int(xml_node_t *node, const char *name, int def)
{
	xml_attr_t *attr = xml_attr_lookup(node, name);
	if (attr && attr->value) return atoi(attr->value);
	else return(def);
}

/*!
 * \brief Get a string or the default value from an attribute of a node.
 *
 * Will return the text of a given attribute of a node as a string. If the
 * node does not have an attribute of the given name or the attribute does
 * not contain text, a default value will be returned.
 *
 * \param node The node to return the string from.
 * \param name The name of the attribute.
 * \param def The default value to return in case no text is available.
 *
 * \return Either the value of the attribute or the default value.
 */

char *xml_attr_str(xml_node_t *node, const char *name, char *def)
{
	xml_attr_t *attr = xml_attr_lookup(node, name);
	if (attr && attr->value) return(attr->value);
	else return(def);
}
