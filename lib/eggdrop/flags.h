/* flags.h: header for flags.c
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
 * $Id: flags.h,v 1.6 2003/12/17 07:39:14 wcc Exp $
 */

#ifndef _EGG_FLAGS_H_
#define _EGG_FLAGS_H_

typedef struct {
	int builtin;
	int udef;
} flags_t;

/* str should be at least 26+26+1 = 53 bytes. */
int flag_to_str(flags_t *flags, char *str);
int flag_merge_str(flags_t *flags, const char *str);
int flag_from_str(flags_t *flags, const char *str);
int flag_match_subset(flags_t *left, flags_t *right);
int flag_match_exact(flags_t *left, flags_t *right);
int flag_match_partial(flags_t *left, flags_t *right);

#endif /* !_EGG_FLAGS_H_ */
