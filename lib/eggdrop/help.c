/* help.c: help file support
 *
 * Copyright (C) 2003, 2004 Eggheads Development Team
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
static const char rcsid[] = "$Id: help.c,v 1.3 2004/03/01 22:58:32 stdarg Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <eggdrop/eggdrop.h>

/* The number of total help entries. */
static int nhelp = 0;

/* Hash table to associate help entry names with help entries. */
static hash_table_t *name_ht = NULL;

int help_init()
{
	name_ht = hash_table_create(NULL, NULL, HELP_HASH_SIZE, HASH_TABLE_STRINGS);
	return(0);
}

int help_load(const char *path, const char *fname)
{
	int i, j;
	xml_node_t *root, *help_node, *helpdata_node;
	char *name, *seealso, *flags, *line, *syntax, help_path[512];
	help_t *h;

	root = xml_node_new();
	snprintf(help_path, sizeof(help_path), "%s%s", path, fname);
	if (xml_read(root, help_path) == -1) return(-1); /* Return -1 if file could not be opened. */

	for (i = 0; i < root->nchildren; i++) {
		help_node = root->children[i];
		if (strcasecmp(help_node->name, "help")) continue; /* We only want data inside <help></help> tags. */

		xml_node_get_str(&name, help_node, "name", 0, 0);
		if (!name) continue; /* We must have an entry name. */

		h = help_lookup_by_name(name);
		if (h) continue;  /* Entry name can't already exist. */

		xml_node_get_str(&syntax, help_node, "syntax", 0, 0);
		xml_node_get_str(&flags, help_node, "flags", 0, 0);

		helpdata_node = xml_node_lookup(help_node, 0, "helpdata", 0, 0);
		if (!helpdata_node) continue; /* We must have a helpdata node. */

		h = calloc(1, sizeof(*h));
		h->name = strdup(name);
		h->flags = strdup(flags);
		h->syntax = strdup(syntax);

		/* Get helpdata lines. */
		for (j = 0; ; j++) {
			xml_node_get_str(&line, helpdata_node, "line", j, 0);
			if (!line) break;
			h->lines = realloc(h->lines, sizeof(char *) * (h->nlines + 1));
			h->lines[h->nlines] = strdup(line);
			h->nlines++;
		}

		/* Insert help entry into the hashtable. */
		hash_table_insert(name_ht, h->name, h);

		nhelp++;

		/* Get seealso entries. */
		for (j = 0; ; j++) {
			xml_node_get_str(&seealso, help_node, "seealso", j, 0);
			if (!seealso) break;
			h->seealso = realloc(h->seealso, sizeof(char *) * (h->nseealso + 1));
			h->seealso[h->nseealso] = strdup(seealso);
			h->nseealso++;
		}
	}
	xml_node_destroy(root);
	return(0);
}


help_t *help_lookup_by_name(const char *name)
{
	help_t *h = NULL;

	hash_table_find(name_ht, name, &h);
	return(h);
}

int help_count()
{
	return(nhelp);
}

int help_print_party(partymember_t *p, help_t *h)
{
	int i;
	char seealsobuf[512];

	if (h->syntax && h->syntax[0]) partymember_printf(p, "Syntax: %s", h->syntax);

	if (h->flags && h->flags[0]) partymember_printf(p, "Flags: %s", h->flags);

	if (h->nlines) {
		for (i = 0; i < h->nlines; i++) {
			if (!h->lines[i]) continue;
			if (h->lines[i][0]) partymember_printf(p, h->lines[i]);
			else partymember_printf(p, "");
		}
	}

	if (!h->nseealso) return(0);
	partymember_printf(p, "");
	seealsobuf[0] = 0;
	for (i = 0; i < h->nseealso; i++) {
		if (!h->seealso[i] || !h->seealso[i][0]) continue;
		if ((strlen(seealsobuf) + strlen(h->seealso[i])) > 67) {
			seealsobuf[strlen(seealsobuf) - 2] = 0; /* Remove the comma. */
			partymember_printf(p, "See also: %s", seealsobuf);
			seealsobuf[0] = 0;
		}
		strlcat(seealsobuf, h->seealso[i], sizeof(seealsobuf));
		strlcat(seealsobuf, ", ", sizeof(seealsobuf));
	}
	if (strlen(seealsobuf)) {
		seealsobuf[strlen(seealsobuf) - 2] = 0; /* Remove the comma. */
		partymember_printf(p, "See also: %s", seealsobuf);
	}
	return(0);
}
