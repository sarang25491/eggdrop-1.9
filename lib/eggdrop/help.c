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

/* XXX: Todo
 * XXX:   * 1.6: Output ordered by flags (Commands for Owners, Masters, ...)
 * XXX:   * 1.6: Help formatting (dunno how to support that and if its really needed)
 * XXX:   * Make help_path, help_lang_default, help_lang_fallback configurable
 */
 
#ifndef lint
static const char rcsid[] = "$Id: help.c,v 1.4 2004/06/17 13:32:43 wingman Exp $";
#endif

#include <sys/types.h>
#include <ctype.h>			/* toupper		*/
#include <dirent.h>			/* opendir, closedir	*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <eggdrop/eggdrop.h>


/* Hash table to associate help entry names with help entries. */
static hash_table_t *entries = NULL;
int nentries = 0;

/* Hash table to associate help sections with help entries */
static hash_table_t *sections = NULL;
int nsections = 0;

/* XXX: make this configurable (just how?) */
static char *help_path = "help/";
static char *help_lang_default = "en_EN";
static char *help_lang_fallback = "en_EN";

static help_entry_t	*help_load_entry	(const char *module, const char *filename, xml_node_t *node);
static void		 help_delete_entry	(help_entry_t *entry);
static help_section_t	*help_load_section	(help_section_t *section, const char *module, const char *filename, xml_node_t *node);
static void		 help_delete_section	(help_section_t *section);

static int		 help_load_internal	(const char *module, const char *filename);
static int		 help_unload_internal	(const char *module, const char *filename);


typedef struct
{
	partymember_t	*member;
	const char	*text;
	int		 hits;
} help_search_t;

typedef struct
{
	const char	*filename;
	const char	*module;
} help_unload_t;

int
help_init (void)
{
	entries = hash_table_create(NULL, NULL, HELP_HASH_SIZE, 
			HASH_TABLE_STRINGS);
	sections = hash_table_create(NULL, NULL, HELP_HASH_SIZE,
			HASH_TABLE_STRINGS);

	bind_add_simple ("load", NULL, "*", (Function)help_load_by_module);
	bind_add_simple ("unload", NULL, "*", (Function)help_unload_by_module);

	return 0;
}

int
help_shutdown (void)
{
	hash_table_destroy (entries);
	hash_table_destroy (sections);
	return 0;
}

int
help_count_sections (void)
{
	return nsections;
}

int
help_count_entries (void)
{
	return nentries;
}

int
help_load_by_module (const char *mod)
{
	char buf[255];
	int ret[3];

	snprintf (buf, sizeof (buf), "%s.xml", mod);
	ret[0] = help_load_internal (mod, buf);
	
	snprintf (buf, sizeof (buf), "%s-commands.xml", mod);
	ret[1] = help_load_internal (mod, buf);

	snprintf (buf, sizeof (buf), "%s-variables.xml", mod);
	ret[2] = help_load_internal (mod, buf);

	if (ret[0] || ret[1] || ret[2]) 
		putlog (LOG_MISC, "*", "Help loaded: %s", mod);
		
	return (ret[0] || ret[1] || ret[2]);
}

int
help_unload_by_module (const char *mod)
{
	char buf[255];
	int ret[3];

	snprintf (buf, sizeof (buf), "%s.xml", mod);
	ret[0] = help_unload_internal (mod, buf);
	
	snprintf (buf, sizeof (buf), "%s-commands.xml", mod);
	ret[1] = help_unload_internal (mod, buf);

	snprintf (buf, sizeof (buf), "%s-variables.xml", mod);
	ret[2] = help_unload_internal (mod, buf);

	if (ret[0] || ret[1] || ret[2]) 
		putlog (LOG_MISC, "*", "Help unloaded: %s", mod);
		
	return (ret[0] || ret[1] || ret[2]);
}

static void
help_load_description (help_desc_t *desc, xml_node_t *parent)
{
	char *text;
	char *ptr;
	int indent = -1, i;

	desc->lines = NULL;
	desc->nlines = 0;

	if (parent == NULL)
		return;

	text = parent->text;
	if (text == NULL || !*text)
		return;

	do {
		if (indent == -1) {
			for (indent = 0; (*text == ' ' || *text == '\t'); indent++)
				text++;
		} else {
			for (i = 0; (*text == ' ' || *text == '\t') && i < indent; i++)
				text++;
		}
		
		/* I have those dual handlings of different line breaks... */
		ptr = strchr (text, '\r');		
		if (ptr == NULL)
			ptr = strchr (text, '\n');
		if (ptr != NULL)
			text [(size_t)(ptr - text)] = '\0';

		if (desc->nlines == 0 && strlen (text) == 0)
			indent = -1;
			
		desc->lines = realloc (desc->lines, 
			(desc->nlines + 1) * sizeof(char *));
		desc->lines[desc->nlines++] = strdup (text);

		if (ptr == NULL)
			return;

		text += (size_t)(ptr - text) + 1;
		if (text [-1] == '\r' && text[0] == '\n')
			text++;
	} while (1);
}

static char *
help_new_syntax (xml_node_t *node)
{
	xml_node_t *args, *arg;
	char buf[255];
	char *name, *optional;
	int i;
	size_t len;	
	
	args = xml_node_select (node, "args");
	if (args == NULL)
		return NULL;
	
	len = 0;	
	memset (buf, 0, sizeof (buf));
	
	for (i = 0; i < args->nchildren; i++) {
		arg = args->children[i];
		if (0 != strcmp (arg->name, "arg"))
			continue;
			
		name = arg->text;
		optional = xml_attr_get_str (arg, "optional");

		if (*buf) {
			strcat (buf + len, " "); len++;
		}
				
		if (len + strlen(name) + 2 > sizeof (buf))
			break;
		
		if (optional == NULL || (optional && 0 == strcmp (optional, "no")))
			sprintf (buf + len, "<%s>", name);
		else
			sprintf (buf + len, "[%s]", name);
			
		len += strlen (name) + 2;
	}
	
	return strdup (buf);
}

static help_entry_t *
help_load_entry (const char *module, const char *filename, xml_node_t *node)
{
	help_entry_t *entry;
	int i;
	
	entry = malloc (sizeof (help_entry_t));
	memset (entry, 0, sizeof (help_entry_t));

	str_redup (&entry->name, xml_attr_get_str (node, "name"));
	str_redup (&entry->flags, xml_attr_get_str (node, "flags"));

	entry->type = strdup (node->name);
	entry->type[0] = toupper (entry->type[0]);
	
	entry->module = strdup (module);
	entry->filename = strdup (filename);
	
	if (0 == strcmp (entry->type, "Command")) {
		entry->ext.command.syntax = help_new_syntax (node);
	} else if (0 == strcmp (entry->type, "Variable")) {
		str_redup (&entry->ext.variable.path, xml_attr_get_str (node, "path"));
		str_redup (&entry->ext.variable.type, xml_attr_get_str (node, "type"));		
	
	}

	help_load_description (&entry->desc, xml_node_select (node, "description"));

	entry->seealso = NULL;
	entry->nseealso = 0;
	
	for (i = 0; i < node->nchildren; i++) {
		if (0 != strcmp (node->children[i]->name, "seealso"))
			continue;

		entry->seealso = realloc (entry->seealso, sizeof (char *) *
					(entry->nseealso + 1));
		entry->seealso [entry->nseealso++] = strdup (node->children[i]->text);
	}

	/* add to hash table */
	hash_table_insert (entries, entry->name, entry);
	
	nentries++;
	
	return entry;
}

help_entry_t *
help_lookup_entry (const char *name)
{
	help_entry_t *entry = NULL;
	if (hash_table_find (entries, name, &entry) != 0)
		return NULL;
	return entry;
}

static void
help_delete_entry (help_entry_t *entry)
{
	int i;

	if (entry->name) free (entry->name);
	if (entry->filename) free (entry->filename);
	if (entry->flags) free (entry->flags);

	if (0 == strcmp (entry->type, "Variable")) {
		if (entry->ext.command.syntax) free (entry->ext.command.syntax);
	} else if (0 == strcmp (entry->type, "Command")) {	
		if (entry->ext.variable.type) free (entry->ext.variable.type);
		if (entry->ext.variable.path) free (entry->ext.variable.path);
	}

	if (entry->type) free (entry->type);
	
	for (i = 0; i < entry->nseealso; i++)
		free (entry->seealso[i]);
	if (entry->seealso) free (entry->seealso);

	for (i = 0; i < entry->desc.nlines; i++)
		free (entry->desc.lines[i]);
	if (entry->desc.lines) free (entry->desc.lines);

	free (entry);
	
	nentries--;
}

static void
help_delete_section (help_section_t *section)
{
	int i;
	
	for (i = 0; i < section->nentries; i++)
		help_delete_entry (section->entries[i]);
	if (section->entries) free (section->entries);

        for (i = 0; i < section->desc.nlines; i++)
                free (section->desc.lines[i]);
        if (section->desc.lines) free (section->desc.lines);

	free (section->name);
	
	nsections--;	
}

static help_section_t *
help_load_section (help_section_t *section,
	const char *module, const char *filename, xml_node_t *node)
{
	xml_node_t *child;
	help_entry_t *entry;
	int i;
			
	if (section == NULL) {
		section = malloc (sizeof (help_section_t));
		memset (section, 0, sizeof (help_section_t));
		str_redup (&section->name, xml_attr_get_str (node, "name"));
	
		section->nentries = 0;
		section->entries = NULL;
	
		help_load_description (&section->desc, xml_node_select (node, "description"));
	
		/* add to hash table */
		hash_table_insert (sections, section->name, section);
		
		nsections++;		
	}
	
	/* load entries */
	for (i = 0; i < node->nchildren; i++) {
		char *name;
	
		/* we just want <command /> or <variable /> tags */
		child = node->children[i];
		if (0 != strcmp (child->name, "command")
			&& 0 != strcmp (child->name, "variable"))
			continue;	
			
		name = xml_attr_get_str (child, "name");
		if (name == NULL)
			continue;
			
		entry = help_lookup_entry (name);
		if (entry != NULL) {
			/* XXX: perhaps allow or overwrite multiple entries? */
			putlog (LOG_MISC, "*", _("Duplicate help entry: '%s'."), name);	
			continue;
		}
		
		/* create entry */
		entry = help_load_entry (module, filename, child);
		if (entry == NULL)
			continue;			


		entry->section = section;
		
		/* add to section */
		section->entries = realloc(section->entries, 
					sizeof (help_entry_t *) *
						(section->nentries + 1));
		section->entries[section->nentries++] = entry;
	}
	
	return section;
}

help_section_t *
help_lookup_section (const char *name)
{
	help_section_t *section = NULL;
	if (hash_table_find (sections, name, &section) != 0)
		return NULL;
	return section;
}

static FILE *
help_open_localized (char *buf, size_t size, const char *filename)
{
	char *lang;
	char *pos;
	FILE *fd;

	lang = getenv ("LANG");
	if (lang != NULL) {
		pos = strchr (lang, '.');
		if (pos == NULL)
			lang = (char *)help_lang_default;
		else
			lang[(pos - lang)] = '\0';
	} else {
		lang = (char *)help_lang_default;
	}


	snprintf (buf, size, "%s/%s/%s", help_path, lang, filename);

	/* try to open it */
	fd = fopen (buf, "r");
	if (fd != NULL)
		return fd;

	/* no, try fallback variant */
	if (strcmp (lang, help_lang_fallback) == 0)
		return NULL;
	snprintf (buf, size, "%s/%s/%s", help_path, help_lang_fallback, filename);

	return fopen (buf, "r");
}

static int
help_load_internal (const char *module, const char *filename)
{
	xml_node_t *doc, *root, *node;
	char buf[255];
	char *name;
	FILE *fd;
	int i;
	
	/* load xml document */
	fd = help_open_localized (buf, sizeof (buf), filename); 
	if (fd == NULL)
		return 0;
	if (xml_load (fd, &doc) != 0)
		return 0;
	fclose (fd);
	
	root = xml_root_element (doc);
	for (i = 0; i < root->nchildren; i++) {

		/* we just want <section /> tags */
		node = root->children[i];
		if (0 != strcmp (node->name, "section"))
			continue;
			
		name = xml_attr_get_str (node, "name");
		if (name == NULL)
			continue;
			
		/* load section */
		help_load_section (
			help_lookup_section (name), module, buf, node);
	}
	
	xml_node_destroy (doc);

	return 1;
}

int
help_load (const char *filename)
{
	return help_load_internal (NULL, filename);
}

static void
unload_entry (const char *name, help_entry_t **e, help_unload_t *ul)
{
	help_entry_t *entry = *e;
	help_section_t *section = entry->section;
	int i;
	
	if (0 != strcmp (entry->module, ul->module)
		&& 0 != strcmp (entry->filename, ul->filename))
	{
		return;
	}
	
	/* Remove it from hash table */
	hash_table_delete(entries, name, entry);
	
	/* Remove it from section */
	if (section->nentries > 1) {
		for (i = 0; i < section->nentries; i++) {
			if (section->entries[i] != entry)
				continue;
				
			if (i == section->nentries - 1)
				continue;
				
			memmove (section->entries + i, section->entries + i + 1,
					sizeof (help_section_t *) * 
						(section->nentries - i - 1));
							
		}
		section->entries = realloc (section->entries,
					(sizeof (help_section_t *) *
						(section->nentries - 1)));
		section->nentries--;
	} else if (section->nentries == 1) {
		free (section->entries); section->entries = NULL;
		section->nentries--;
	}
	 
	/* Delete entry */
	help_delete_entry (entry);
	
	/* Remove empty sections */
	if (section->nentries == 0)
		help_delete_section (section);		
}

static int
help_unload_internal (const char *module, const char *filename)
{
	help_unload_t unload;
	
	unload.module = module;
	unload.filename = filename;
	
	hash_table_walk(sections, 
		(hash_table_node_func)unload_entry, &unload);
			
	return 1;
}


int
help_unload (const char *filename)
{
	return help_unload_internal (NULL, filename);
}

static void
print_entry (const char *name, help_entry_t *entry, partymember_t *member)
{
	int i;

	/* Command */
	if (0 == strcmp (entry->type, "Command")) {
		partymember_printf (member, "%s: %s %s", entry->type, entry->name,
			entry->ext.command.syntax);	
			
	/* Variable */
	} else if (0 == strcmp (entry->type, "Variable")) {
		partymember_printf (member, "%s: %s (%s)", entry->type, entry->name,
			entry->ext.variable.path);		
		partymember_printf (member, ("  Type: %s"), entry->ext.variable.type);
	}

	if (entry->flags)
                partymember_printf (member, _("  Flags required: %s"), entry->flags);

	for (i = 0; i < entry->desc.nlines; i++) {
		partymember_printf (member, "    %s", entry->desc.lines[i]);
	}
	for (i = 0; i < entry->nseealso; i++) { 
		partymember_printf (member, _("See also: %s"), entry->seealso[i]);
	}
}

static void
print_section (const char *name, help_section_t **s, partymember_t *member)
{
	int i, j;
	help_section_t *section = *s;
	help_entry_t *entry;
	char *names[4];
	
	partymember_printf (member, _("%s:"), name);
	for (i = 0; i < section->desc.nlines; i++) {
		partymember_printf (member, "  %s", section->desc.lines[i]);
	}	
	if (section->desc.nlines > 0)
		partymember_printf (member, "");

	/* we want 4 entries in one row */
	for (i = 0; i < section->nentries; ) {
		names[0] = names[1] = names[2] = names[3] = "";

		for (j = 0; j < 4 && i < section->nentries; ) {
			entry = section->entries[i++]; 

		        /* check if user is allowed to see this entry */
			if (entry->flags && !user_check_flags_str (member->user, NULL, entry->flags))
				continue;

			names[j++] = entry->name;
		}

		partymember_printf (member, 
			"    %-16s %-16s %-16s %-16s",
				names[0], names[1], names[2], names[3]);
	}
	partymember_printf (member, _(""));
}
 
static void
search_entries (const char *name, help_entry_t **entry, help_search_t *search)
{
	/* check if user is allowed to see this entry */
	if ((*entry)->flags && !user_check_flags_str (search->member->user, NULL, (*entry)->flags))
		return;

	if (wild_match (search->text, name) > 0) {
		print_entry (name, *entry, search->member);
		search->hits++;
	} else {
		int i;

		/* do a fulltext search */
		for (i = 0; i < (*entry)->desc.nlines; i++) {
			if (wild_match (search->text, (*entry)->desc.lines[i]) > 0) {
				print_entry (name, *entry, search->member);
				search->hits++;
				return;
			}
		}
	}
}

static int
help_search (partymember_t *member, const char *text)
{
	help_search_t search;

	search.member = member;	
	search.text = text;
	search.hits = 0;

	hash_table_walk (entries, (hash_table_node_func)search_entries, &search);

	return (search.hits > 0);
}

int
help_print_party (partymember_t *member, const char *args)
{
	help_entry_t *entry;
	help_section_t *section;
		 
	if (nentries == 0) {
		partymember_printf (member, _("No help entries loaded."));
		return BIND_RET_LOG;
	}

	/* if no args given just print out full help */
	if (args == NULL || 0 == strcmp (args, "all")) {
		hash_table_walk(sections, 
			(hash_table_node_func)print_section, member);
		partymember_printf (member, ("You can get help on individual command or variable: '.help <entry>'."));
		partymember_printf (member, ("If you only remember a part of the entrie's name you are"));
		partymember_printf (member, ("searching for, just use wildcards (e.g. '.help *bot*') and"));
		partymember_printf (member, ("all matching help texts will be displayed."));
		return BIND_RET_LOG;
	}
	
	/* may be either a section or a entry, whatever comes first */
	entry = help_lookup_entry (args);
	if (entry != NULL) {
	        /* check if user is allowed to see this entry */
        	if (!entry->flags || user_check_flags_str (member->user, NULL, entry->flags)) {
			print_entry (entry->name, entry, member);
			return BIND_RET_LOG;
		}
	}
		
	section = help_lookup_section (args);
	if (section != NULL) {
		print_section (section->name, &section, member);
		return BIND_RET_LOG;
	}

	/* try a wildmatch search */
	if (!help_search (member, args)) {
		partymember_printf (member, _("Fulltext search resulted in no entries for '%s'."),
			args);
		return BIND_RET_BREAK;
	}
		
	return BIND_RET_LOG;
}
