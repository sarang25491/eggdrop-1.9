/* xmlread.c: xml parser
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
static const char rcsid[] = "$Id: xmlread.c,v 1.10 2004/06/17 13:32:43 wingman Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <eggdrop/eggdrop.h>

extern xml_amp_conversion_t builtin_conversions[];

/* These are pretty much in 'most common' order. */
static int original_text = 0;
static const char *spaces = " \t\n\r\v";
static const char *name_terminators = "= \t\n\r\v?/>";

/*
 * Skip over whitespace.
 * Return number of bytes we used up.
 */
int skip_whitespace(char **data)
{
	int n;

	n = strspn(*data, spaces);
	*data += n;
	return(n);
}

/*
 * Read in data up to a space, =, >, or ?
 */
static void read_name(char **data, char **name)
{
	int n;

	n = strcspn(*data, name_terminators);
	if (!n) {
		*name = NULL;
		return;
	}
	*name = (char *)malloc(n+1);
	memcpy(*name, *data, n);
	(*name)[n] = 0;

	*data += n;
}

/*
 * Read in an attribute value.
 */
static void read_value(char **data, char **value)
{
	const char *terminator;
	int n;

	/* It's supposed to startwith ' or ", but we'll take no ' or " to mean
		it's a one word value. */

	if (**data == '\'') terminator = "'";
	else if (**data == '"') terminator = "\"";
	else {
		terminator = name_terminators;
		(*data)--;
	}

	/* Skip past first ' or " so we can find the ending one. */
	(*data)++;

	n = strcspn(*data, terminator);
	*value = (char *)malloc(n+1);
	memcpy(*value, *data, n);
	(*value)[n] = 0;

	/* Skip past closing ' or ". */
	if (terminator != name_terminators) n++;
	*data += n;
}

static void read_text(xml_node_t *node, char **data)
{
	char *end;
	char *text;
	int len;

	/* Find end-point. */
	end = strchr(*data, '<');
	if (!end) end = *data + strlen(*data);

	/* Get length of text. */
	len = end - *data;

	/* Add it to the node's current text value. */
	text = (char *)realloc(node->text, len + node->len + 1);
	memcpy(text + node->len, *data, len);
	node->text = text;
	node->len += len;
	text[node->len] = 0;

	/* Update data to point to < (or \0). */
	*data = end;
}

/* Decoded result is guaranteed <= original. */
int xml_decode_text(char *text, int inlen)
{
	char *end, *result, *orig;
	int n;
	/* Some variables for &char; conversion. */
	char *amp, *colon, *next;
	int i;

	if (original_text)
		return inlen;
		
	/* text = input, result = output */
	orig = result = text;
	end = text + inlen;

	/* Can't start with a space. */
	skip_whitespace(&text);

	while (text < end) {
		/* Get count of non-spaces. */
		n = strcspn(text, spaces);

		/* If we're supporting &char; notation, here's where it
			happens. If we can't find the &char; in the conversions
			table, then we leave the '&' for it by default. The
			conversion table is defined in xml.c. */

		next = text+n;
		while (n > 0 && (amp = memchr(text, '&', n))) {
			memmove(result, text, (amp-text));
			result += (amp-text);
			n -= (amp-text);
			colon = memchr(amp, ';', n);
			if (!colon) break;
			*colon = 0;
			text = colon+1;
			n -= (colon-amp);
			amp++; /* Skip past &. */
			for (i = 0; builtin_conversions[i].key; i++) {
				if (!strcasecmp(amp, builtin_conversions[i].key)) {
					*result++ = builtin_conversions[i].value;
					break;
				}
			}
		}
		memmove(result, text, n);
		result += n;
		text = next;

		/* Skip over whitespace. */
		n = skip_whitespace(&text);

		/* If there was any, and it's not the end, replace it all with
			a single space. */
		if (n && text < end) {
			*result++ = ' ';
		}
	}

	*result = 0;

	return(result - orig);
}

/* Parse a string of attributes. */
static void read_attributes(xml_node_t *node, char **data)
{
	xml_attr_t attr;

	for (;;) {
		/* Skip over any leading whitespace. */
		skip_whitespace(data);

		/* Read in the attribute name. */
		read_name(data, &attr.name);
		if (!attr.name) return;

		skip_whitespace(data);

		/* Check for '=' sign. */
		if (**data != '=') {
			free(attr.name);
			return;
		}
		(*data)++;

		skip_whitespace(data);

		read_value(data, &attr.value);

		xml_node_append_attr (node, &attr);
	}
}

/*
 * Read an entire node and all its children.
 * 0 - read the node successfully
 * 1 - reached end of input
 * 2 - reached end of node
 */
int xml_read_node(xml_node_t *parent, char **data)
{
	xml_node_t node, *ptr;
	int n;
	char *end;
	

	/* Read in any excess data and save it with the parent. */
	read_text(parent, data);

	/* If read_text() doesn't make us point to an <, we're at the end. */
	if (**data != '<') return(1);

	/* Skip over the < and any whitespace. */
	(*data)++;
	skip_whitespace(data);

	/* If this is a closing tag like </blah> then we're done. */
	if (**data == '/') return(2);

	/* Initialize the node. */
	memset(&node, 0, sizeof(node));

	/* If this is the start of the xml declaration, <?xml version=...?> then
		we read it like a normal tag, but set the 'decl' member. */
	if (**data == '?') {
		(*data)++;
		node.type = XML_PROCESSING_INSTRUCTION;
	}
	else if (**data == '!') {
		(*data)++;
		if (**data == '-') {
			/* Comment <!-- ... --> */
			int len;

			*data += 2; /* Skip past '--' part. */
			end = strstr(*data, "-->");
			len = end - *data;
			node.text = (char *)malloc(len+1);
			memcpy(node.text, *data, len);
			node.text[len] = 0;
			node.len = len;
			node.type = XML_COMMENT;
			xml_node_add(parent, &node);
			*data = end+3;
			return(0); /* Skip past '-->' part. */
		}
	} else {
		node.type = XML_ELEMENT;
	}

	/* Read in the tag name. */
	read_name(data, &node.name);
	
	/* Now that we have a name, go ahead and add the node to the tree. */
	ptr = xml_node_add(parent, &node);

	/* Read in the attributes. */
	read_attributes(ptr, data);

	/* Now we should be pointing at ?, /, or >. */

	/* '?' and '/' are empty tags with no children or text value. */
	if (**data == '/' || **data == '?') {
		end = strchr(*data, '>');
		if (!end) return(1); /* End of input. */
		*data = end + 1;
		return(0);
	}

	/* Skip over closing '>' after attributes. */
	(*data)++;

	/* Parse children and text value. */
	do {
		n = xml_read_node(ptr, data);
	} while (n == 0);

	/* Ok, the recursive xml_read_node() has read in all the text value for
		this node, so decode it now. */
	ptr->len = xml_decode_text(ptr->text, ptr->len);

	/* Ok, now 1 means we reached end-of-input. */
	/* 2 means we reached end-of-node. */
	if (n == 2) {
		/* We don't validate to make sure the start tag and end tag
			matches. */
		end = strchr(*data, '>');
		if (!end) return(1);

		*data = end+1;
		return(0);
		/* End of our node. */
	}

	/* Otherwise, end-of-input. */
	return(1);
}

int
xml_load (FILE *fd, xml_node_t **node)
{
	size_t size, read;
	char *data;
	int ret;

	/* seek to begin */
	fseek (fd, 0l, SEEK_SET);

	/* seek to end */
	fseek (fd, 0l, SEEK_END);
	size = ftell (fd) * sizeof(char);
	fseek (fd, 0l, SEEK_SET);

	data = malloc(size + (1 * sizeof(char)));
	read = fread (data, sizeof(char), size, fd);
	data[read] = 0;

	ret = xml_load_str (data, node);

	free (data);

	return ret;
}

int
xml_load_file (const char *file, xml_node_t **node)
{
	FILE *fd;
	int ret;

	fd = fopen (file, "r");
	if (fd == NULL)
		return -1;
	ret = xml_load (fd, node);
	fclose (fd);

	return ret;
}

int
xml_load_str (char *str, xml_node_t **node)
{
	xml_node_t *root;

	(*node) = NULL;

	root = malloc (sizeof (xml_node_t));
	if (root == NULL)
		return 0;

	memset (root, 0, sizeof (xml_node_t));

	original_text = 1; /* XXX: remove this hack */
	while (!xml_read_node(root, &str)) {
		; /* empty */
	}
	original_text = 0; /* XXX: remove this hack */

	if (root->nchildren == 0) {
		free (root);
		return -1;
	}
	root->len = xml_decode_text(root->text, root->len);

	(*node) = root;

	return 0;
}

int xml_read(xml_node_t *root, const char *fname)
{
	FILE *fp;
	int size;
	char *data, *dataptr;

	fp = fopen(fname, "r");
	if (!fp) return(-1);

	fseek(fp, 0l, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0l, SEEK_SET);

	data = malloc(size+1);
	fread(data, size, 1, fp);
	data[size] = 0;
	fclose(fp);

	memset (root, 0, sizeof (xml_node_t));
	
	dataptr = data;
	while (!xml_read_node(root, &dataptr)) {
		; /* empty */
	}
	free(data);

	/* Ok, the recursive xml_read_node() has read in all the text value for
		this node, so decode it now. */
	root->len = xml_decode_text(root->text, root->len);
	return(0);
}
