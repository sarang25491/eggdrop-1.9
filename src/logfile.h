/* logfile.h: header for logfile.c
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
 *
 * $Id: logfile.h,v 1.11 2004/06/28 17:36:34 wingman Exp $
 */

#ifndef _EGG_LOGILE_H
#define _EGG_LOGILE_H

typedef enum
{
	LOG_STATE_ENABLED = 0,
	LOG_STATE_DISABLED
} logstate_t;

typedef struct
{
	char *filename;
	char *chname;
	int mask;

	logstate_t state;
	char *fname;
	char *last_msg;
	int repeats;
	int flags;
	FILE *fp;
} logfile_t;

typedef struct
{
	int keep_all;
	int quick;
	int max_size;
	int switch_at;
	char *suffix;

	logfile_t *logfiles;
	int nlogfiles;
} logging_t;

extern void logfile_init(void); 
extern void logfile_shutdown(void);

extern char *logfile_add(char *, char *, char *);
extern int logfile_del(char *);

extern void flushlogs(); 

#endif /* !_EGG_LOGFILE_H */
