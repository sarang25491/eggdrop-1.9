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
static const char rcsid[] = "$Id: xmlwrite.c,v 1.8 2004/06/23 17:24:43 wingman Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <eggdrop/memory.h>
#include <eggdrop/memutil.h>
#include <eggdrop/xml.h>

#define STREAM_STRING 	0
#define STREAM_FILE	1

#define XML_INDENT_CHAR	'\t'

static int write_options = XML_NONE;

typedef struct 
{
	union
	{
		FILE *file;
		struct
		{
			char *buf;
			size_t length;
			size_t size;
		} string;
	} data;
	int type;
} stream_t;

static int xml_write_node(stream_t *stream, xml_node_t *node);

static int stream_printf(stream_t *stream, const char *format, ...)
{
	va_list args;
	int ret = 0;

	va_start(args, format);
	switch (stream->type) {

		case (STREAM_STRING):
			while (1) {
				/* (try to) write into our buffer */
				if (stream->data.string.buf != NULL) {
					ret = vsnprintf(stream->data.string.buf + stream->data.string.length,
						stream->data.string.size - stream->data.string.length, format, args);
				}

				/* check if we succeeded */
				if (stream->data.string.buf == NULL
					|| ret >= stream->data.string.size - stream->data.string.length)
				{	
					/* we need to resize our buffer */
					stream->data.string.buf = realloc(stream->data.string.buf,
						stream->data.string.size + 256);

					/* realloc failed, damn */
					if (stream->data.string.buf == NULL)
						return (-1);

					memset(stream->data.string.buf + stream->data.string.length, 0,
						stream->data.string.size - stream->data.string.length);

					/* update size */
					stream->data.string.size += 256;

				} else {	
					stream->data.string.length += ret;
					break;
				}
			}

			break;

		case (STREAM_FILE):
			ret = vfprintf(stream->data.file, format, args);
			break;
	
		default:
			ret = -1;
			break;
	}
	va_end(args);

	return ret;
}

static int level = 0;
static char indent[32] = {0};

static int xml_write_children(stream_t *stream, xml_node_t *node)
{
	int i, ret = 0;

	/* init indent */
	memset(indent, '\t', sizeof(indent));

	indent[++level] = 0;

	/* XXX: encode text */
	if (node->text && *node->text)
		stream_printf(stream, "%s%s\n", indent, node->text);

        for (i = 0; i < node->nchildren; i++) {
		stream_printf(stream, "%s", indent);
                if ((ret = xml_write_node(stream, node->children[i])) == -1)
			break;
	}

	indent[--level] = 0;

        return ret;
}

static int xml_write_attributes(stream_t *stream, xml_node_t *node)
{
	int i;

	for (i = 0; i < node->nattributes; i++) {
		xml_attr_t *attr = &node->attributes[i];
		if (stream_printf(stream, " %s=\"%s\"", attr->name, attr->value) == -1)
			return (-1);
	}

	return (0);
}

static int xml_write_element(stream_t *stream, xml_node_t *node)
{
	if (stream_printf(stream, "<%s", node->name) == -1)
		return (-1);
	
	if (xml_write_attributes(stream, node) == -1)
		return (-1);

	if (node->nchildren == 0) {
		if (node->text) {
			stream_printf(stream, ">%s</%s>\n", node->text, node->name);
		} else {
			stream_printf(stream, "/>\n");
		}

		return (0);
	}

	stream_printf(stream, ">\n");
	if (xml_write_children(stream, node) == -1)
		return (-1);

	return stream_printf(stream, "%s</%s>\n", indent, node->name);
}

static int xml_write_document(stream_t *stream, xml_node_t *node)
{
	return xml_write_children(stream, node);
}

static int xml_write_comment(stream_t *stream, xml_node_t *node)
{
	/* XXX: that's wrong, text needs to encoded */
	return stream_printf(stream, "<!--%s-->\n", node->text);
}

static int xml_write_cdata_section(stream_t *stream, xml_node_t *node)
{
	return stream_printf(stream, "<[CDATA[%s]]>\n", node->text);
}

static int xml_write_processing_instruction(stream_t *stream, xml_node_t *node)
{
	if (stream_printf(stream, "<?%s", node->name) == -1)
		return (-1);
	
	if (xml_write_attributes(stream, node) == -1)
		return (-1);

	if (stream_printf(stream, "?>\n") == -1)
		return (-1);

	return (0);
}

static int xml_write_node(stream_t *stream, xml_node_t *node)
{
	switch (node->type) {
	
		case (XML_DOCUMENT):
			return xml_write_document(stream, node);

		case (XML_ELEMENT):
			return xml_write_element(stream, node);

		case (XML_COMMENT):
			return xml_write_comment(stream, node);
	
		case (XML_CDATA_SECTION):
			return xml_write_cdata_section(stream, node);

		case (XML_PROCESSING_INSTRUCTION):
			return xml_write_processing_instruction(stream, node);
			
	}

	return (-1);
}

int xml_save(FILE *fd, xml_node_t *node, int options)
{
        stream_t stream;
        int ret;

        stream.data.file = fd;
        stream.type = STREAM_FILE;

        if (stream.data.file == NULL)
                return -1;

	write_options = options;
        ret = xml_write_node(&stream, node);
	write_options = XML_NONE;

        return ret;
}

int xml_save_file(const char *file, xml_node_t *node, int options)
{
	stream_t stream;
	int ret;

	stream.data.file = fopen(file, "w");
	stream.type = STREAM_FILE;

	if (stream.data.file == NULL)
		return -1;

	write_options = options;
	ret = xml_write_node(&stream, node);
	write_options = XML_NONE;

	fclose(stream.data.file);

	return ret;
}

int xml_save_str(char **str, xml_node_t *node, int options)
{
	stream_t stream;
	int ret;

	stream.data.string.buf = NULL;
	stream.data.string.length = 0;
	stream.data.string.size = 0;

	stream.type = STREAM_STRING;

	write_options = options;
	ret = xml_write_node(&stream, node);
	write_options = XML_NONE;
	
	*str = NULL;
	if (ret == -1) {
		if (stream.data.string.length > 0)
			free(stream.data.string.buf);
	} else {
		(*str) = stream.data.string.buf;
	}

	return ret;
}
