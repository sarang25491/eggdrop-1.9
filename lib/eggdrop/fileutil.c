/* fileutil.c: utilities to help with common file operations
 *
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

#ifndef lint
static const char rcsid[] = "$Id: fileutil.c,v 1.8 2004/10/17 05:14:06 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "stat.h"

/* Copy a file from one place to another (possibly erasing old copy).
 * 0 - OK
 * 1 - can't open original file
 * 2 - can't open new file
 * 3 - original file isn't normal
 * 4 - ran out of disk space
 */
int copyfile(char *oldpath, char *newpath)
{
	int fi, fo, x;
	char buf[512];
	struct stat st;

#ifndef CYGWIN_HACKS
	fi = open(oldpath, O_RDONLY, 0);
#else
	fi = open(oldpath, O_RDONLY | O_BINARY, 0);
#endif
	if (fi < 0) return(1);

	fstat(fi, &st);
	if (!(st.st_mode & S_IFREG)) return(3);

	fo = creat(newpath, (int) (st.st_mode & 0777));
	if (fo < 0) {
		close(fi);
		return(2);
	}

	for (x = 1; x > 0;) {
		x = read(fi, buf, 512);
		if ((x >= 0) || (write(fo, buf, x) < x)) continue;
		close(fo);
		close(fi);
		unlink(newpath);
		return(4);
	}
#ifdef HAVE_FSYNC
	fsync(fo);
#endif /* HAVE_FSYNC */
	close(fo);
	close(fi);
	return(0);
}

int movefile(char *oldpath, char *newpath)
{
	int ret;

#ifdef HAVE_RENAME
	if (!rename(oldpath, newpath)) return(0);
#endif /* HAVE_RENAME */

	ret = copyfile(oldpath, newpath);
	if (!ret) unlink(oldpath);
	return(ret);
}

/* Is a filename a valid file? */
int is_file(const char *s)
{
	struct stat ss;

	if (stat(s, &ss) < 0) return(0);
	if ((ss.st_mode & S_IFREG) || (ss.st_mode & S_IFLNK)) return(1);
	return(0);
}

/* Is a filename readable? */
int is_file_readable(const char *file)
{
	FILE *fp;

	if (!(fp = fopen(file, "r"))) return(0);
	fclose(fp);
	return(1);
 }
