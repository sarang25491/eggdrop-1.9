/* flags.c: functions for working with flags
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
 */

#ifndef lint
static const char rcsid[] = "$Id: flags.c,v 1.7 2004/06/17 13:32:43 wingman Exp $";
#endif

#include "flags.h"

/* str must be at least 26+26+1 = 53 bytes. */
int flag_to_str(flags_t *flags, char *str)
{
	int i, j;

	j = 0;
	for (i = 0; i < 26; i++) {
		if (flags->builtin & (1 << i)) str[j++] = 'a' + i;
	}
	for (i = 0; i < 26; i++) {
		if (flags->udef & (1 << i)) str[j++] = 'A' + i;
	}
	str[j] = 0;
	return 1;
}

static inline void add_flag(int *intptr, int dir, int flag)
{
	if (dir < 0) *intptr &= ~(1 << flag);
	else *intptr |= 1 << flag;
}

int flag_merge_str(flags_t *flags, const char *str)
{
	int dir = 1;

	/* If it doesn't start with +/-, then it's absolute. */
	if (*str != '+' && *str != '-') {
		flags->builtin = flags->udef = 0;
	}

	while (*str) {
		if (*str >= 'a' && *str <= 'z') {
			add_flag(&flags->builtin, dir, *str - 'a');
		}
		else if (*str >= 'A' && *str <= 'Z') {
			add_flag(&flags->udef, dir, *str - 'A');
		}
		else if (*str == '-') dir = -1;
		else if (*str == '+') dir = 1;
		str++;
	}

	return 1;
}

int flag_from_str(flags_t *flags, const char *str)
{
	/* Always reset flags first. */
	flags->builtin = flags->udef = 0;
	return flag_merge_str(flags, str);
}


/* Are all on-bits in left also on in right? */
int flag_match_subset(flags_t *left, flags_t *right)
{
	int builtin, udef;

	builtin = (left->builtin & right->builtin) == left->builtin;
	udef = (left->udef & right->udef) == left->udef;
	return (builtin && udef);
}

/* Are all on-bits in left also on in right, and all off-bits also off? */
int flag_match_exact(flags_t *left, flags_t *right)
{
	int builtin, udef;

	builtin = left->builtin ^ right->builtin;
	udef = left->udef ^ right->udef;
	return !(builtin || udef);
}

/* Is at least 1 on-bit in left also on in right? */
int flag_match_partial(flags_t *left, flags_t *right)
{
	int builtin, udef;

	/* If no bits are on in right, it matches automatically. */
	if (!(right->builtin | right->udef)) return(1);

	builtin = left->builtin & right->builtin;
	udef = left->udef & right->udef;
	return (builtin || udef);
}
