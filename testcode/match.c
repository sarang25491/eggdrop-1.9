/*
 * match.c
 *   new wildcard matching functions updated for speed.
 *     --Ian.
 */
/*
 * Copyright (C) 2001, 2002 Eggheads Development Team
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

#define QUOTE '\\'		/* quoting character... for matching "*" etc. */
#define WILDS '*'		/* matches any number of characters */
#define WILDP '%'		/* matches any nunber of non-space characters */
#define WILDQ '?'		/* matches exactly one character */
#define WILDT '~'		/* matches any number of spaces */

#include <ctype.h>
#define irctoupper toupper

/* matching for binds */
int wild_match_per(char *wild, char *matchtext) {
  char *fallback=0;
  int match = 0, saved = 0, count = 0, wildcard = 0;

  /* If either string is NULL, they can't match */
  if (!matchtext || !wild)
    return 0;

  while(*matchtext && *wild) {
    switch (*wild) {
      case WILDP :
          fallback = wild++;			/* setup fallback */
          match = 1;
          while((*(wild++)==WILDP));		/* get rid of redundant %s ... %%%% */
          if (*wild==QUOTE)			/* is the match char quoted? */
            wild++;
          while(*matchtext && (irctoupper(*wild) != irctoupper(*matchtext)))
            if (*matchtext==' ') {
              match=0;				/* loop until we find the next char... or don't */
              break;
            } else
              matchtext++;
          break;
      case WILDQ :				/* inc both... let the while() check for errors */
          wild++;
          matchtext++;
          break;
      case WILDT :
          wild++;
          if (*matchtext!=' '){			/* need at least one space */
            count = saved;
            if (!fallback)			/* no match and no fallback? die */
              return 0;
            wild = fallback-1;			/* fall back */
            break;
          }
          while ((*(matchtext++)==' '));	/* loop through spaces */
          count++;
          break;
      case WILDS :
          if (!*++wild)
            return (count+saved+1);
          wildcard = 1;
          break;
      default :
          if (*wild==QUOTE)			/* is it quoted? */
            wild++;
          if (wildcard) {
            saved = count;
            match = count = 0;
            fallback = wild;			/* setup fallback in case we hit a false positive */
            while(*matchtext && (irctoupper(*wild) != irctoupper(*matchtext)))
              matchtext++;
            if (!*matchtext) {
              match=1;
              break;
            }
          }
          if (irctoupper(*wild) != irctoupper(*matchtext)) {
            if (!fallback)		/* no match and no fallback? die */
              return 0;
            count = saved;
            wild = fallback-1;		/* fall back */
            break;
          }
          count++;
          wildcard = 0;
          match=1;
          if (*wild)
            wild++;
          if (*matchtext)
            matchtext++;
    }
  }
  return (!match || *wild || *matchtext)?0:(count+saved+1);
}

/* generic hostmask matching... */
int wild_match(char *wild, char *matchtext) {
  char *fallback=0;
  int match = 0, count = 0, saved = 0, wildcard = 0;

  /* If either string is NULL, they can't match */
  if (!matchtext || !wild)
    return 0;

  while(*matchtext && *wild) {
    switch (*wild) {
      case WILDQ :				/* inc both... let the while() check for errors */
          wild++;
          matchtext++;
          break;
          while ((*(matchtext++)==' '));	/* loop through spaces */
          break;
      case WILDS :
          if (!*++wild)
            return (count+saved+1);
          wildcard = 1;
          break;
      default :
          if (*wild==QUOTE)			/* is it quoted? */
            wild++;
          if (wildcard) {
            saved = count;
            match = count = 0;
            fallback = wild;			/* setup fallback in case we hit a false positive */
            while(*matchtext && (irctoupper(*wild) != irctoupper(*matchtext)))
              matchtext++;
            if (!*matchtext) {
              match=1;
              break;
            }
          }
          if (irctoupper(*wild) != irctoupper(*matchtext)) {
            if (!fallback)		/* no match and no fallback? die */
              return 0;
            count = saved;
            wild = fallback-1;		/* fall back */
            break;
          }
          wildcard = 0;
          count++;
          match=1;
          if (*wild)
            wild++;
          if (*matchtext)
            matchtext++;
    }
  }
  return (!match || *wild || *matchtext)?0:(count+saved+1);
}
