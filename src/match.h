/*
 * match.h
 *   prototypes for match.c
 *
 * $Id: match.h,v 1.2 2001/10/31 04:02:51 tothwolf Exp $
 */
#ifndef _EGG_MATCH_H
#define _EGG_MATCH_H


/*
 * Prototypes
 */
extern int wild_match(register unsigned char *, register unsigned char *);
extern int wild_match_per(register unsigned char *, register unsigned char *);

#endif				/* !_EGG_MATCH_H */
