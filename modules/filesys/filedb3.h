/*
 * filedb3.h --
 *
 *	filedb header file
 *
 * Written by Fabian Knittel <fknittel@gmx.de>
 */
/*
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Eggheads Development Team
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
/*
 * $Id: filedb3.h,v 1.8 2003/12/11 00:49:10 wcc Exp $
 */

#ifndef _EGG_MOD_FILESYS_FILEDB3_H
#define _EGG_MOD_FILESYS_FILEDB3_H

/*
 * FIXME: These really should be in the .c files that include this header
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* for time_t */
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/* Top of each DB */
typedef struct {
  char version;				/* DB version			*/
  time_t timestamp;			/* Last updated			*/
} filedb_top;

/* Header of each entry */
typedef struct {
  time_t uploaded;			/* Upload time			*/
  unsigned int size;			/* File size			*/
  unsigned short int stat;		/* Misc information		*/
  unsigned short int gots;		/* Number of gets		*/
  unsigned short int filename_len; 	/* Length of filename buf	*/
  unsigned short int desc_len;	 	/* Length of description buf	*/
  unsigned short int chan_len;		/* Length of channel name buf	*/
  unsigned short int uploader_len;	/* Length of uploader buf	*/
  unsigned short int flags_req_len;	/* Length of flags buf		*/
  unsigned short int buffer_len;	/* Length of additional buffer	*/
} filedb_header;

/* Structure used to pass data between lower level
 * and higher level functions.
 */
typedef struct {
  time_t uploaded;			/* Upload time			*/
  unsigned int size;			/* File size			*/
  unsigned short int stat;		/* Misc information		*/
  unsigned short int gots;		/* Number of gets		*/
  unsigned short int _type;		/* Type of entry (private)	*/

  /* NOTE: These three are only valid during one db open/close. Entry
   *       movements often invalidate them too, so make sure you know
   *       what you're doing before using/relying on them.
   */
  long pos;				/* Last position in the filedb	*/
  unsigned short int dyn_len;		/* Length of dynamic data in DB	*/
  unsigned short int buf_len;		/* Length of additional buffer	*/

  char *filename;			/* Filename			*/
  char *desc;				/* Description			*/
  char *chan;				/* Channel name			*/
  char *uploader;			/* Uploader			*/
  char *flags_req;			/* Required flags		*/
} filedb_entry;


/*
 * Macros
 */

/* Macro to calculate the total length of dynamic data. */
#define filedb_tot_dynspace(fdh) ((fdh).filename_len + (fdh).desc_len +	\
	(fdh).chan_len + (fdh).uploader_len + (fdh).flags_req_len)

#define filedb_zero_dynspace(fdh) {					\
	(fdh).filename_len	= 0;					\
	(fdh).desc_len		= 0;					\
	(fdh).chan_len		= 0;					\
	(fdh).uploader_len	= 0;					\
	(fdh).flags_req_len	= 0;					\
}

/* Memory debugging makros */
#define malloc_fdbe() _malloc_fdbe(__FILE__, __LINE__)
#define filedb_getfile(fdb, pos, get) _filedb_getfile(fdb, pos, get, __FILE__, __LINE__)
#define filedb_matchfile(fdb, pos, match) _filedb_matchfile(fdb, pos, match, __FILE__, __LINE__)
#define filedb_updatefile(fdb, pos, fdbe, update) _filedb_updatefile(fdb, pos, fdbe, update, __FILE__, __LINE__)
#define filedb_addfile(fdb, fdbe) _filedb_addfile(fdb, fdbe, __FILE__, __LINE__)
#define filedb_movefile(fdb, pos, fdbe) _filedb_movefile(fdb, pos, fdbe, __FILE__, __LINE__)

/* File matching */
#define FILEMATCH (match+sofar)
#define FILEQUOTE '\\'
#define FILEWILDS '*'
#define FILEWILDQ '?'

/*
 *  Constants
 */

#define FILEDB_VERSION1	0x0001
#define FILEDB_VERSION2	0x0002	/* DB version used for 1.3, 1.4		*/
#define FILEDB_VERSION3	0x0003
#define FILEDB_NEWEST_VER FILEDB_VERSION3	/* Newest DB version	*/

#define POS_NEW		0	/* Position which indicates that the
				   entry wants to be repositioned.	*/

#define FILE_UNUSED	0x0001	/* Deleted entry.			*/
#define FILE_DIR	0x0002	/* It's actually a directory.		*/
#define FILE_HIDDEN	0x0004	/* Hidden file.				*/

#define FILEDB_ESTDYN	50	/* Estimated dynamic length of an entry	*/

enum {
  GET_HEADER,			/* Only save minimal data		*/
  GET_FILENAME,			/* Additionally save filename		*/
  GET_FULL,			/* Save all data			*/

  UPDATE_HEADER,		/* Only update header			*/
  UPDATE_SIZE,			/* Update header, enforce new buf sizes	*/
  UPDATE_ALL,			/* Update additional data too		*/

  TYPE_NEW,			/* New entry				*/
  TYPE_EXIST			/* Existing entry			*/
};


/*
 *  filedb3.c prototypes
 */

static void free_fdbe(filedb_entry **);
static filedb_entry *_malloc_fdbe(char *, int);
static int filedb_readtop(FILE *, filedb_top *);
static int filedb_writetop(FILE *, filedb_top *);
static int filedb_delfile(FILE *, long);
static filedb_entry *filedb_findempty(FILE *, int);
static int _filedb_updatefile(FILE *, long, filedb_entry *, int, char *, int);
static int _filedb_movefile(FILE *, long, filedb_entry *, char *, int);
static int _filedb_addfile(FILE *, filedb_entry *, char *, int);
static filedb_entry *_filedb_getfile(FILE *, long, int, char *, int);
static filedb_entry *_filedb_matchfile(FILE *, long, char *, char *, int);
static filedb_entry *filedb_getentry(char *, char *);

#endif				/* !_EGG_MOD_FILESYS_FILEDB3_H */
