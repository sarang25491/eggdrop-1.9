#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xml.h"

extern xml_amp_conversion_t builtin_conversions[];

/* These are pretty much in 'most common' order. */
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
#ifdef AMP_CHARS
	/* Some variables for &char; conversion. */
	char *amp, *colon, *next;
	int i;
#endif

	/* text = input, result = output */
	orig = result = text;
	end = text + inlen;

	/* Can't start with a space. */
	skip_whitespace(&text);

	while (text < end) {
		/* Get count of non-spaces. */
		n = strcspn(text, spaces);

#ifdef AMP_CHARS
		/* If we're supporting &char; notation, here's where it
			happens. If we can't find the &char; in the conversions
			table, then we leave the '&' for it by default. The
			conversion table is defined in xml.c. */

		amp = text;
		next = text+n;
		while (n > 0 && (amp = memchr(amp, '&', n))) {
			colon = memchr(amp, ';', n);
			amp++;
			if (!colon) break;
			*colon++ = 0;
			for (i = 0; builtin_conversions[i].key; i++) {
				if (!strcasecmp(amp, builtin_conversions[i].key)) {
					*(amp-1) = builtin_conversions[i].value;
					break;
				}
			}
			n -= (colon-amp);

			/* Shift bytes down, including trailing null. */
			memmove(amp, colon, n);
		}
		memcpy(result, text, n);
		text = next;
#else
		/* If we don't support conversions, just copy the text. */
		memcpy(result, text, n);
		text += n;
#endif
		result += n;

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
	xml_attribute_t attr;

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

		xml_attribute_add(node, &attr);
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
	if (**data == '/') {
		return(2);
	}

	/* Initialize the node. */
	memset(&node, 0, sizeof(node));

	/* If this is the start of the xml declaration, <?xml version=...?> then
		we read it like a normal tag, but set the 'decl' member. */
	if (**data == '?') {
		(*data)++;
		node.type = 1;
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
			node.type = 2;
			xml_node_add(parent, &node);
			*data = end+3;
			return(0); /* Skip past '-->' part. */
		}
		printf("Unsupported <! type\n");
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

	dataptr = data;
	while (!xml_read_node(root, &dataptr)) {
		; /* empty */
	}
	free(data);
	return(0);
}
