/* match.h: header for match.c
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Eggheads Development Team
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
 * $Id: match.h,v 1.3 2003/12/17 07:39:14 wcc Exp $
 */

#ifndef _EGG_MATCH_H_
#define _EGG_MATCH_H_

#define QUOTE '\\'	/* Quoting character, for matching "*", etc. */
#define WILDS '*'	/* Matches any number of characters. */
#define WILDP '%'	/* Matches any nunber of non-space characters. */
#define WILDQ '?'	/* Matches exactly one character. */
#define WILDT '~'	/* Matches any number of spaces. */

extern int wild_match(const char *mask, const char *text);
extern int wild_match_per(const char *mask, const char *text);

#endif /* !_EGG_MATCH_H_ */
