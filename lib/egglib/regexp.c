#include <stdlib.h>
#include <regex.h>	/* regcomp, regexec */
#include <string.h>	/* strlen, memcpy */

#include "regexp.h"

/* Default behavior is case-insensitive, perform all substitutions.
	Pass REGEXP_SENSITIVE to do a case-sensitive match.
	Pass REGEXP_FIRST to do only the first substitution.
*/
char *regsub(char *expr, char *text, char *sub, int flags)
{
	regex_t regex;
	int regcomp_flags = REG_EXTENDED;
	regmatch_t match;
	int offset, size, sublen, unmatchlen, textlen;
	char *buf = NULL;

	if (!(flags & REGEXP_SENSITIVE)) regcomp_flags |= REG_ICASE;

	if (regcomp(&regex, expr, regcomp_flags)) return(NULL);

	textlen = strlen(text);
	sublen = strlen(sub);
	offset = 0;
	size = 0;
	while (offset < textlen) {
		if (regexec(&regex, text+offset, 1, &match, 0)) break;
		if (match.rm_so == -1) break;

		unmatchlen = match.rm_so;
		buf = (char *)realloc(buf, size + unmatchlen + sublen + 1);
		memcpy(buf+size, text+offset, unmatchlen);
		size += unmatchlen;
		memcpy(buf+size, sub, sublen);
		size += sublen;
		offset += match.rm_eo;
		if (flags & REGEXP_FIRST) break;
	}
	/* Copy remainder. */
	buf = (char *)realloc(buf, size + textlen - offset);
	memcpy(buf+size, text+offset, textlen - offset);
	size += (textlen - offset);
	buf[size] = 0;

	regfree(&regex);
	return(buf);
}
