/*
 * match.c
 *   wildcard matching functions
 *
 * $Id: match.c,v 1.8 2001/10/12 02:27:45 stdarg Exp $
 */
/*
 * Once this code was working, I added support for % so that I could
 * use the same code both in Eggdrop and in my IrcII client.
 * Pleased with this, I added the option of a fourth wildcard, ~,
 * which matches varying amounts of whitespace (at LEAST one space,
 * though, for sanity reasons).
 *
 * This code would not have been possible without the prior work and
 * suggestions of various sources.  Special thanks to Robey for
 * all his time/help tracking down bugs and his ever-helpful advice.
 *
 * 04/09:  Fixed the "*\*" against "*a" bug (caused an endless loop)
 *
 *   Chris Fuller  (aka Fred1@IRC & Fwitz@IRC)
 *     crf@cfox.bchs.uh.edu
 *
 * I hereby release this code into the public domain
 */

/* prototypes */
#include "match.h"
#include "irccmp.h"	/* irctoupper() */


/* The quoting character -- what overrides wildcards */
#define QUOTE '\\'

/* The "matches ANYTHING" wildcard */
#define WILDS '*'

/* The "matches ANY NUMBER OF NON-SPACE CHARS" wildcard */
#define WILDP '%'

/* The "matches EXACTLY ONE CHARACTER" wildcard */
#define WILDQ '?'

/* The "matches AT LEAST ONE SPACE" wildcard */
#define WILDT '~'

/* Changing these is probably counter-productive :) */
#define UNQUOTED (0x7FFF)
#define QUOTED   (0x8000)
#define NOMATCH 0
#define MATCH ((match+sofar)&UNQUOTED)
#define MATCH_PER (match+saved+sofar)


/*
 * wild_match(char *ma, char *na)
 *
 * Features:  Backwards, case-insensitive, ?, *
 * Best use:  Matching of hostmasks (since they are likely to begin
 *            with a * rather than end with one).
 */
/*
 * sofar's high bit is used as a flag of whether or not we are quoting.
 * The other matchers don't need this because when you're going forward,
 * you just skip over the quote char.
 * No "saved" is used to track "%" and no special quoting ability is
 * needed, so we just have (match+sofar) as the result.
 */
int wild_match(register unsigned char *m, register unsigned char *n)
{
  unsigned char *ma = m, *na = n, *lsm = 0, *lsn = 0;
  int match = 1;
  register int sofar = 0;

  /* take care of null strings (should never match) */
  if ((ma == 0) || (na == 0) || (!*ma) || (!*na))
    return NOMATCH;
  /* find the end of each string */
  while (*(++m));
    m--;
  while (*(++n));
    n--;

  while (n >= na) {
    if ((m <= ma) || (m[-1] != QUOTE)) {	/* Only look if no quote */
      switch (*m) {
      case WILDS:		/* Matches anything */
	do
	  m--;			/* Zap redundant wilds */
	while ((m >= ma) && ((*m == WILDS) || (*m == WILDP)));
	if ((m >= ma) && (*m == '\\'))
	  m++;			/* Keep quoted wildcard! */
	lsm = m;
	lsn = n;
	match += sofar;
	sofar = 0;		/* Update fallback pos */
	continue;		/* Next char, please */
      case WILDQ:
	m--;
	n--;
	continue;		/* '?' always matches */
      }
      sofar &= UNQUOTED;	/* Remember not quoted */
    } else
      sofar |= QUOTED;		/* Remember quoted */
    if (_irctoupper(*m) == _irctoupper(*n)) {	/* If matching char */
      m--;
      n--;
      sofar++;			/* Tally the match */
      if (sofar & QUOTED)
	m--;			/* Skip the quote char */
      continue;			/* Next char, please */
    }
    if (lsm) {			/* To to fallback on '*' */
      n = --lsn;
      m = lsm;
      if (n < na)
	lsm = 0;		/* Rewind to saved pos */
      sofar = 0;
      continue;			/* Next char, please */
    }
    return NOMATCH;		/* No fallback=No match */
  }
  while ((m >= ma) && ((*m == WILDS) || (*m == WILDP)))
    m--;			/* Zap leftover %s & *s */
  return (m >= ma) ? NOMATCH : MATCH;	/* Start of both = match */
}


/*
 * wild_match_per(char *m, char *n)
 *
 * Features:  Forward, case-insensitive, ?, *, %, ~(optional)
 * Best use:  Generic string matching, such as in IrcII-esque bindings
 */
int wild_match_per(register unsigned char *m, register unsigned char *n)
{
  unsigned char *ma = m, *lsm = 0, *lsn = 0, *lpm = 0, *lpn = 0;
  int match = 1, saved = 0, space;
  register unsigned int sofar = 0;

  /* take care of null strings (should never match) */
  if ((m == 0) || (n == 0) || (!*n))
    return NOMATCH;
  /* (!*m) test used to be here, too, but I got rid of it.  After all,
   * If (!*n) was false, there must be a character in the name (the
   * second string), so if the mask is empty it is a non-match.  Since
   * the algorithm handles this correctly without testing for it here
   * and this shouldn't be called with null masks anyway, it should be
   * a bit faster this way */

  while (*n) {
    /* Used to test for (!*m) here, but this scheme seems to work better */
    if (*m == WILDT) {		/* Match >=1 space */
      space = 0;		/* Don't need any spaces */
      do {
	m++;
	space++;
      }				/* Tally 1 more space ... */
      while ((*m == WILDT) || (*m == ' '));	/*  for each space or ~ */
      sofar += space;		/* Each counts as exact */
      while (*n == ' ') {
	n++;
	space--;
      }				/* Do we have enough? */
      if (space <= 0)
	continue;		/* Had enough spaces! */
    }
    /* Do the fallback       */
    else {
      switch (*m) {
      case 0:
	do
	  m--;			/* Search backwards */
	while ((m > ma) && (*m == '?'));	/* For first non-? char */
	if ((m > ma) ? ((*m == '*') && (m[-1] != QUOTE)) : (*m == '*'))
	  return MATCH_PER;		/* nonquoted * = match */
	break;
      case WILDP:
	while (*(++m) == WILDP);	/* Zap redundant %s */
	if (*m != WILDS) {	/* Don't both if next=* */
	  if (*n != ' ') {	/* WILDS can't match ' ' */
	    lpm = m;
	    lpn = n;		/* Save '%' fallback spot */
	    saved += sofar;
	    sofar = 0;		/* And save tally count */
	  }
	  continue;		/* Done with '%' */
	}
	/* FALL THROUGH */
      case WILDS:
	do
	  m++;			/* Zap redundant wilds */
	while ((*m == WILDS) || (*m == WILDP));
	lsm = m;
	lsn = n;
	lpm = 0;		/* Save '*' fallback spot */
	match += (saved + sofar);	/* Save tally count */
	saved = sofar = 0;
	continue;		/* Done with '*' */
      case WILDQ:
	m++;
	n++;
	continue;		/* Match one char */
      case QUOTE:
	m++;			/* Handle quoting */
      }
      if (_irctoupper(*m) == _irctoupper(*n)) {		/* If matching */
	m++;
	n++;
	sofar++;
	continue;		/* Tally the match */
      }
    }
    if (lpm) {			/* Try to fallback on '%' */
      n = ++lpn;
      m = lpm;
      sofar = 0;		/* Restore position */
      if ((*n | 32) == 32)
	lpm = 0;		/* Can't match 0 or ' ' */
      continue;			/* Next char, please */
    }
    if (lsm) {			/* Try to fallback on '*' */
      n = ++lsn;
      m = lsm;			/* Restore position */
      /* Used to test for (!*n) here but it wasn't necessary so it's gone */
      saved = sofar = 0;
      continue;			/* Next char, please */
    }
    return NOMATCH;		/* No fallbacks=No match */
  }
  while ((*m == WILDS) || (*m == WILDP))
    m++;			/* Zap leftover %s & *s */
  return (*m) ? NOMATCH : MATCH_PER;	/* End of both = match */
}
