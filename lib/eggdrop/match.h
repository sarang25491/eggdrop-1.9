/*
 * match.h --
 *
 *	prototypes for match.c
 */
/*
 * $Id: match.h,v 1.2 2003/02/25 06:52:19 stdarg Exp $
 */

#ifndef _EGG_MATCH_H
#define _EGG_MATCH_H


/*
 * Prototypes
 */
extern int wild_match(const char *mask, const char *text);
extern int wild_match_per(const char *mask, const char *text);

#endif				/* !_EGG_MATCH_H */
