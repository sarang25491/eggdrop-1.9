/*
 * match.h
 *   prototypes for match.c
 *
 * $Id: match.h,v 1.1 2001/10/11 18:24:01 tothwolf Exp $
 */
#ifndef _EGG_MATCH_H
#define _EGG_MATCH_H


/*
 * prototypes
 */
extern int wild_match(register unsigned char *, register unsigned char *);
extern int wild_match_per(register unsigned char *, register unsigned char *);

#endif				/* !_EGG_MATCH_H */
