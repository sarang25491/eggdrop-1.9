#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ircmasks.h"

int ircmask_list_add(ircmask_list_t *list, const char *ircmask, void *data)
{
	int i, stars, len;

	stars = 0;
	len = strlen(ircmask);
	for (i = 0; i < len; i++) {
		if (ircmask[i] == '*') stars++;
	}
	len -= stars;
	for (i = 0; i < list->len; i++) {
		if (len > list->list[i].len) break;
	}
	list->list = realloc(list->list, sizeof(*list->list) * (list->len+1));
	memmove(list->list+i+1, list->list+i, sizeof(*list->list) * (list->len-i));
	list->list[i].ircmask = strdup(ircmask);
	list->list[i].len = len;
	list->list[i].data = data;
	list->len++;
	return(0);
}

int ircmask_list_del(ircmask_list_t *list, const char *ircmask, void *data)
{
}

int ircmask_list_find(ircmask_list_t *list, const char *irchost, void *dataptr)
{
	int i;

	for (i = 0; i < list->len; i++) {
		if (wild_match(list->list[i].ircmask, irchost) > 0) {
			*(void **)dataptr = list->list[i].data;
			return(0);
		}
	}
	*(void **)dataptr = NULL;
	return(-1);
}
