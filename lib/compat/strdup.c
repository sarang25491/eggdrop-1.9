/*
 * strdup.c
 *   provides strdup()
 *
 * $Id: strdup.c,v 1.1 2002/01/16 22:09:40 ite Exp $
 */

static char *strdup(const char *entry)
{
  char *target = (char*)malloc(strlen(entry) + 1);
  if (target == 0) return 0;
  strcpy(target, entry);
  return target;
}

