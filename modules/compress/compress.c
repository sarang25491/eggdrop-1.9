/*
 * compress.c --
 *
 *	uses the compression library libz to compress and uncompress the
 *	userfiles during the sharing process
 *
 * Written by Fabian Knittel <fknittel@gmx.de>.
 * Based on zlib examples by Jean-loup Gailly and Miguel Albrecht.
 */
/*
 * Copyright (C) 2000, 2001, 2002, 2003 Eggheads Development Team
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
static const char rcsid[] = "$Id: compress.c,v 1.9 2003/02/15 05:04:57 wcc Exp $";
#endif

#define MODULE_NAME "compress"
#define MAKING_COMPRESS

#include <string.h>
#include <errno.h>
#include <zlib.h>

#include "lib/eggdrop/module.h"

#ifdef HAVE_MMAP
#  include <sys/types.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#endif /* HAVE_MMAP */
#include "compress.h"

#define start compress_LTX_start

#define BUFLEN	512


static eggdrop_t *egg = NULL;

static unsigned int compressed_files;	/* Number of files compressed.      */
static unsigned int uncompressed_files;	/* Number of files uncompressed.    */
static unsigned int compress_level;	/* Default compression used.	    */


static int uncompress_to_file(char *f_src, char *f_target);
static int compress_to_file(char *f_src, char *f_target, int mode_num);
static int compress_file(char *filename, int mode_num);
static int uncompress_file(char *filename);
static int is_compressedfile(char *filename);

#include "tclcompress.c"


/*
 *    Misc functions.
 */

static int is_compressedfile(char *filename)
{
  char		  buf1[50], buf2[50];
  FILE		 *fin;
  register int    len1, len2, i;

  memset(buf1, 0, 50);
  memset(buf2, 0, 50);
  if (!is_file(filename))
    return COMPF_FAILED;

  /* Read data with zlib routines.
   */
  fin = gzopen(filename, "rb");
  if (!fin)
    return COMPF_FAILED;
  len1 = gzread(fin, buf1, sizeof(buf1));
  if (len1 < 0)
    return COMPF_FAILED;
  if (gzclose(fin) != Z_OK)
    return COMPF_FAILED;

  /* Read raw data.
   */
  fin = fopen(filename, "rb");
  if (!fin)
    return COMPF_FAILED;
  len2 = fread(buf2, 1, sizeof(buf2), fin);
  if (ferror(fin))
    return COMPF_FAILED;
  fclose(fin);

  /* Compare what we found.
   */
  if (len1 != len2)
    return COMPF_COMPRESSED;
  for (i = 0; i < sizeof(buf1); i++)
    if (buf1[i] != buf2[i])
      return COMPF_COMPRESSED;
  return COMPF_UNCOMPRESSED;
}


/*
 *    General compression / uncompression functions
 */

/* Uncompresses a file `f_src' and saves it as `f_target'.
 */
static int uncompress_to_file(char *f_src, char *f_target)
{
  char buf[BUFLEN];
  int len;
  FILE *fin, *fout;

  if (!is_file(f_src)) {
    putlog(LOG_MISC, "*", _("Failed to uncompress file `%s': not a file."),
	   f_src);
    return COMPF_ERROR;
  }
  fin = gzopen(f_src, "rb");
  if (!fin) {
    putlog(LOG_MISC, "*", _("Failed to uncompress file `%s': gzopen failed."),
	   f_src);
    return COMPF_ERROR;
  }

  fout = fopen(f_target, "wb");
  if (!fout) {
    putlog(LOG_MISC, "*", _("Failed to uncompress file `%1$s': open failed: %2$s."),
	   f_src, strerror(errno));
    return COMPF_ERROR;
  }

  while (1) {
    len = gzread(fin, buf, sizeof(buf));
    if (len < 0) {
      putlog(LOG_MISC, "*", _("Failed to uncompress file `%s': gzread failed."),
	     f_src);
      return COMPF_ERROR;
    }
    if (!len)
      break;
    if ((int) fwrite(buf, 1, (unsigned int) len, fout) != len) {
      putlog(LOG_MISC, "*", _("Failed to uncompress file `%1$s': fwrite failed: %2$s."),
	     f_src, strerror(errno));
      return COMPF_ERROR;
    }
  }
  if (fclose(fout)) {
    putlog(LOG_MISC, "*", _("Failed to uncompress file `%1$s': fclose failed: %2$s."),
	   f_src, strerror(errno));
    return COMPF_ERROR;
  }
  if (gzclose(fin) != Z_OK) {
    putlog(LOG_MISC, "*", _("Failed to uncompress file `%s': gzclose failed."),
	   f_src);
    return COMPF_ERROR;
  }
  uncompressed_files++;
  return COMPF_SUCCESS;
}

/* Enforce limits.
 */
inline static void adjust_mode_num(int *mode)
{
  if (*mode > 9)
    *mode = 9;
  else if (*mode < 0)
    *mode = 0;
}

#ifdef HAVE_MMAP
/* Attempt to compress in one go, by mmap'ing the file to memory.
 */
static int compress_to_file_mmap(FILE *fout, FILE *fin)
{
    int		  len, ifd = fileno(fin);
    char	 *buf;
    struct stat	  st;

    /* Find out size of file */
    if (fstat(ifd, &st) < 0)
      return COMPF_ERROR;
    if (st.st_size <= 0)
      return COMPF_ERROR;

    /* mmap file contents to memory */
    buf = mmap(0, st.st_size, PROT_READ, MAP_SHARED, ifd, 0);
    if (buf < 0)
      return COMPF_ERROR;

    /* Compress the whole file in one go */
    len = gzwrite(fout, buf, st.st_size);
    if (len != (int) st.st_size)
      return COMPF_ERROR;

    munmap(buf, st.st_size);
    fclose(fin);
    if (gzclose(fout) != Z_OK)
      return COMPF_ERROR;
    return COMPF_SUCCESS;
}
#endif /* HAVE_MMAP */

/* Compresses a file `f_src' and saves it as `f_target'.
 */
static int compress_to_file(char *f_src, char *f_target, int mode_num)
{
  char  buf[BUFLEN], mode[5];
  FILE *fin, *fout;
  int   len;

  adjust_mode_num(&mode_num);
  snprintf(mode, sizeof mode, "wb%d", mode_num);

  if (!is_file(f_src)) {
    putlog(LOG_MISC, "*", _("Failed to compress file `%s': not a file."),
	   f_src);
    return COMPF_ERROR;
  }
  fin = fopen(f_src, "rb");
  if (!fin) {
    putlog(LOG_MISC, "*", _("Failed to compress file `%1$s': open failed: %2$s."),
	   f_src, strerror(errno));
    return COMPF_ERROR;
  }

  fout = gzopen(f_target, mode);
  if (!fout) {
    putlog(LOG_MISC, "*", _("Failed to compress file `%s': gzopen failed."),
	   f_src);
    return COMPF_ERROR;
  }

#ifdef HAVE_MMAP
  if (compress_to_file_mmap(fout, fin) == COMPF_SUCCESS) {
    compressed_files++;
    return COMPF_SUCCESS;
  } else {
    /* To be on the safe side, close the file before attempting
     * to write again.
     */
    gzclose(fout);
    fout = gzopen(f_target, mode);
  }
#endif /* HAVE_MMAP */

  while (1) {
    len = fread(buf, 1, sizeof(buf), fin);
    if (ferror(fin)) {
      putlog(LOG_MISC, "*", _("Failed to compress file `%1$s': fread failed: %2$s"),
	     f_src, strerror(errno));
      return COMPF_ERROR;
    }
    if (!len)
      break;
    if (gzwrite(fout, buf, (unsigned int) len) != len) {
      putlog(LOG_MISC, "*", _("Failed to compress file `%s': gzwrite failed."),
	     f_src);
      return COMPF_ERROR;
    }
  }
  fclose(fin);
  if (gzclose(fout) != Z_OK) {
    putlog(LOG_MISC, "*", _("Failed to compress file `%s': gzclose failed."),
	   f_src);
    return COMPF_ERROR;
  }
  compressed_files++;
  return COMPF_SUCCESS;
}

/* Compresses a file `filename' and saves it as `filename'.
 */
static int compress_file(char *filename, int mode_num)
{
  char *temp_fn, randstr[5];
  int   ret;

  /* Create temporary filename. */
  temp_fn = malloc(strlen(filename) + 5);
  make_rand_str(randstr, 4);
  strcpy(temp_fn, filename);
  strcat(temp_fn, randstr);

  /* Compress file. */
  ret = compress_to_file(filename, temp_fn, mode_num);

  /* Overwrite old file with compressed version.  Only do so
   * if the compression routine succeeded.
   */
  if (ret == COMPF_SUCCESS)
    movefile(temp_fn, filename);

  free(temp_fn);
  return ret;
}

/* Uncompresses a file `filename' and saves it as `filename'.
 */
static int uncompress_file(char *filename)
{
  char *temp_fn, randstr[5];
  int   ret;

  /* Create temporary filename. */
  temp_fn = malloc(strlen(filename) + 5);
  make_rand_str(randstr, 4);
  strcpy(temp_fn, filename);
  strcat(temp_fn, randstr);

  /* Uncompress file. */
  ret = uncompress_to_file(filename, temp_fn);

  /* Overwrite old file with uncompressed version.  Only do so
   * if the uncompression routine succeeded.
   */
  if (ret == COMPF_SUCCESS)
    movefile(temp_fn, filename);

  free(temp_fn);
  return ret;
}

/*
 *    Compress module related code
 */

static tcl_ints my_tcl_ints[] = {
  {"compress_level",		&compress_level},
  {NULL,                	NULL}
};

static int compress_report(int idx, int details)
{
  if (details) {
    dprintf(idx, P_("    Compressed %u file,", "    Compressed %u files,",
                    compressed_files), compressed_files);
    dprintf(idx, P_(" uncompressed %u file.\n", " uncompressed %u files.\n",
                    uncompressed_files), uncompressed_files);
  }
  return 0;
}

static char *compress_close()
{
  rem_help_reference("compress.help");
  rem_tcl_commands(my_tcl_cmds);
  rem_tcl_ints(my_tcl_ints);
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *start();

static Function compress_table[] =
{
  /* 0 - 3 */
  (Function) start,
  (Function) compress_close,
  (Function) 0,
  (Function) compress_report,
  /* 4 - 7 */
  (Function) compress_to_file,
  (Function) compress_file,
  (Function) uncompress_to_file,
  (Function) uncompress_file,
  /* 8 - 11 */
  (Function) is_compressedfile,
};

char *start(eggdrop_t *eggdrop)
{
  egg = eggdrop;
  compressed_files	= 0;
  uncompressed_files	= 0;
  compress_level	= 9;

  module_register(MODULE_NAME, compress_table, 1, 1);
  if (!module_depend(MODULE_NAME, "eggdrop", 107, 0)) {
    module_undepend(MODULE_NAME);
    return _("This module needs eggdrop1.7.0 or later");
  }
  add_tcl_ints(my_tcl_ints);
  add_tcl_commands(my_tcl_cmds);
  add_help_reference("compress.help");
  return NULL;
}
