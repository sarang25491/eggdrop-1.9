/*
 * match.h --
 *
 *	prototypes for match.c
 */
/*
 * $Id: match.h,v 1.1 2002/10/07 22:33:54 stdarg Exp $
 */

#ifndef _EGG_MATCH_H
#define _EGG_MATCH_H


/*
 * Prototypes
 */
extern int wild_match(const unsigned char *, const unsigned char *);
extern int wild_match_per(const unsigned char *, const unsigned char *);

#endif				/* !_EGG_MATCH_H */
