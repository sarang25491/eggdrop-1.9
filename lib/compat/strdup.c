/*
 * strdup.c --
 *
 *	provides strdup()
 */

#ifndef lint
static const char rcsid[] = "$Id: strdup.c,v 1.2 2002/05/05 16:40:32 tothwolf Exp $";
#endif

static char *strdup(const char *entry)
{
  char *target = (char*)malloc(strlen(entry) + 1);
  if (target == 0) return 0;
  strcpy(target, entry);
  return target;
}

