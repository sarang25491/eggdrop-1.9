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
static const char rcsid[] = "$Id: help.c,v 1.18 2004/09/29 18:03:53 stdarg Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <eggdrop/eggdrop.h>

static help_file_t **help_files = NULL;
static int nhelp_files = 0;

static help_section_t *sections = NULL;
static int nsections = 0;

/* XXX: make this configurable(just how?) */
static char *help_path = NULL;
static char *help_lang_default = NULL;
static char *help_lang_fallback = "en_US";

int help_init()
{
	help_path = strdup("help/");
	help_lang_default = strdup("en_US");
	return(0);
}

int help_shutdown()
{
	return(0);
}

int help_set_default_path(const char *path)
{
	str_redup(&help_path, path);
	return(0);
}

int help_set_default_lang(const char *lang)
{
	str_redup(&help_lang_default, lang);
	return(0);
}

char *help_parse_syntax(xml_node_t *node)
{
	xml_node_t *arg;
	char *name, *syntax, *args, *args2, *argname;
	int optional;

	xml_node_get_vars(node, "sn", "name", &name, "args.arg", &arg);
	args = strdup("");
	for (; arg; arg = arg->next_sibling) {
		xml_node_get_vars(arg, "si", "name", &argname, "optional", &optional);
		if (!name) continue;
		if (optional) args2 = egg_mprintf("%s [%s]", args, argname);
		else args2 = egg_mprintf("%s <%s>", args, argname);
		free(args);
		args = args2;
	}
	syntax = egg_mprintf("%s%s", name, args);
	free(args);
	return(syntax);
}

help_summary_t *help_summarize_entry(xml_node_t *node)
{
	help_summary_t *entry;
	char *name, *summary, *syntax;
	int len;

	xml_node_get_vars(node, "ss", "name", &name, "summary", &summary);
	if (!name) return(NULL);

	syntax = help_parse_syntax(node);

	/* Allocate everything at once since it's static. */
	len = strlen(name) + strlen(syntax);
	if (summary) len += strlen(summary);
	entry = malloc(sizeof(*entry) + len + 3);
	entry->name = ((char *)entry) + sizeof(*entry);
	entry->syntax = entry->name + strlen(name)+1;
	entry->summary = entry->syntax + strlen(syntax)+1;
	strcpy(entry->name, name);
	strcpy(entry->syntax, syntax);
	if (summary) strcpy(entry->summary, summary);
	else entry->summary[0] = 0;
	free(syntax);
	return(entry);
}

int help_parse_file(const char *fname)
{
	xml_node_t *root = xml_parse_file(fname);
	xml_node_t *node;
	char *secname;
	help_summary_t *entry;
	help_section_t *section;
	int i;

	if (!root) return(-1);

	xml_node_get_vars(root, "sn", "section", &secname, "help", &node);
	if (!secname || !node) {
		xml_node_delete(root);
		return(-1);
	}

	for (i = 0; i < nsections; i++) {
		if (!strcasecmp(sections[i].name, secname)) break;
	}
	if (i == nsections) {
		sections = realloc(sections, sizeof(*sections) * (nsections+1));
		sections[nsections].name = strdup(secname);
		sections[nsections].entries = NULL;
		sections[nsections].nentries = 0;
		nsections++;
	}
	section = sections+i;

	help_files = realloc(help_files, sizeof(*help_files) * (nhelp_files+1));
	help_files[nhelp_files] = malloc(sizeof(**help_files));
	help_files[nhelp_files]->name = strdup(fname);
	help_files[nhelp_files]->ref = 0;

	for (; node; node = node->next_sibling) {
		entry = help_summarize_entry(node);
		if (!entry) continue;
		entry->file = help_files[nhelp_files];
		help_files[nhelp_files]->ref++;
		section->entries = realloc(section->entries, sizeof(entry) * (section->nentries+1));
		section->entries[section->nentries] = entry;
		section->nentries++;
	}
	nhelp_files++;
	xml_node_delete(root);
	return(0);
}

help_summary_t *help_lookup_summary(const char *name)
{
	int i, j;
	help_section_t *section;

	for (i = 0; i < nsections; i++) {
		section = sections+i;
		for (j = 0; j < section->nentries; j++) {
			if (!strcasecmp(section->entries[j]->name, name)) {
					return(section->entries[j]);
			}
		}
	}
	return(NULL);
}

xml_node_t *help_lookup_entry(help_summary_t *entry)
{
	xml_node_t *root = xml_parse_file(entry->file->name);
	xml_node_t *node;
	char *name;

	node = xml_node_lookup(root, 0, "help", 0, 0);
	for (; node; node = node->next_sibling) {
		xml_node_get_str(&name, node, "name", 0, 0);
		if (name && !strcasecmp(name, entry->name)) {
			xml_node_unlink(node);
			break;
		}
	}
	return(node);
}

static void localized_help_fname(char *buf, size_t size, const char *filename)
{
	char *lang;
	char *pos;

	lang = getenv("LANG");
	if (lang) {
		lang = strdup(lang);
		pos = strchr(lang, '.');
		if (pos == NULL) {
			free(lang);
			lang = strdup(help_lang_default);
		}
		else *pos = 0;
	} else {
		lang = strdup(help_lang_default);
	}

	snprintf(buf, size, "%s/%s/%s", help_path, lang, filename);
	free(lang);
}

int help_load_by_module(const char *name)
{
	char fullname[256], buf[256];

	snprintf(fullname, sizeof(fullname), "%s-commands.xml", name);
	localized_help_fname(buf, sizeof(buf), fullname);
	if (help_parse_file(buf)) {
		snprintf(buf, sizeof(buf), "%s/%s/%s", help_path, help_lang_fallback, fullname);
		help_parse_file(buf);
	}
	return(0);
}

help_search_t *help_search_new(const char *searchstr)
{
	help_search_t *search;

	search = calloc(1, sizeof(*search));
	search->search = strdup(searchstr);
	return(search);
}

int help_search_end(help_search_t *search)
{
	free(search->search);
	free(search);
	return(0);
}

help_summary_t *help_search_result(help_search_t *search)
{
	help_section_t *section;
	help_summary_t *entry;

	if (search->cursection < 0 || search->curentry < 0 || !search->search) return(NULL);

	while (search->cursection < nsections) {
		section = sections+search->cursection;
		while (search->curentry < section->nentries) {
			entry = section->entries[search->curentry++];
			if (wild_match(search->search, entry->name)) return(entry);
		}
		search->cursection++;
		search->curentry = 0;
	}
	search->cursection = search->curentry = -1;
	return(NULL);
}
