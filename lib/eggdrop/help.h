/* help.h: header for help.c
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
 *
 * $Id: help.h,v 1.2 2004/06/17 13:32:43 wingman Exp $
 */

#ifndef _EGG_HELP_H_
#define _EGG_HELP_H_

#define HELP_HASH_SIZE		50

typedef struct help_entry	help_entry_t;
typedef struct help_section	help_section_t;

typedef struct
{
	char	**lines;		/* Lines of help data		*/
	int	  nlines;		/* Number of help data lines 	*/
} help_desc_t;

struct help_section
{
	char 	 	 *name;		/* Section name			*/
	help_desc_t	  desc;		/* Description			*/
	help_entry_t	**entries;	/* Entries			*/
	int	  	  nentries;	/* Number of entries		*/
};

struct help_entry
{
	char		 *type;		/* Entry type			*/
	char		 *flags;	/* Entry flags			*/
	char		 *module;	/* Source module		*/
	char		 *filename;	/* Source file			*/
	char  		 *name;		/* Entry name 			*/
	help_section_t	 *section;	/* Entry section		*/
	help_desc_t	  desc;		/* Description 			*/
	char 		**seealso;	/* See also entries 		*/
	int   		  nseealso;	/* Number of see also entries 	*/
		
	union
	{
		struct
		{
			char	*syntax;	/* Command syntax	*/
		} command;
		struct
		{
			char	 *type;		/* Variable type	*/
			char	 *path;		/* Variable path	*/			
		} variable;
	} ext;
};

int	 	 help_init		(void);
int	 	 help_shutdown		(void);

int		 help_count_sections	(void);
int		 help_count_entries	(void);

int	 	 help_load		(const char *filename);
int		 help_unload		(const char *filename);

int		 help_load_by_module	(const char *name);
int		 help_unload_by_module	(const char *name);

help_entry_t	*help_lookup_entry	(const char *name);
help_section_t	*help_lookup_section	(const char *name);

int	 	 help_print_party	(partymember_t *member, const char *args);

#endif /* !_EGG_HELP_H_ */
