/* string.h: header for string.c
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
 * $Id: string.h,v 1.2 2004/06/21 20:35:11 wingman Exp $
 */

#ifndef _EGG_STRING_H_
#define _EGG_STRING_H_

int str_ends_with(const char *text, const char *str);
int str_starts_with(const char *text, const char *str);

int egg_get_word(const unsigned char *text, const char **next, char **word);
int egg_get_arg(const unsigned char *text, const char **next, char **arg);
int egg_get_words(const char *text, const char **next, char **word, ...);
int egg_get_args(const char *text, const char **next, char **arg, ...);
int egg_get_word_array(const char *text, const char **next, char **words, int nwords);
int egg_get_arg_array(const char *text, const char **next, char **args, int nargs);
int egg_free_word_array(char **words, int nwords);
int egg_free_arg_array(char **args, int nargs);
void egg_append_static_str(char **dest, int *remaining, const char *src);
void egg_append_str(char **dest, int *cur, int *max, const char *src);

#endif /* !_EGG_STRING_H_ */
