#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "match.h"
#include "egglog.h"
#include "ircmasks.h"

int ircmask_list_add(ircmask_list_t *list, const char *ircmask, void *data)
{
	int i, stars, len;
	ircmask_list_entry_t *entry, *prev;

	stars = 0;
	len = strlen(ircmask);
	for (i = 0; i < len; i++) {
		if (ircmask[i] == '*') stars++;
	}
	len -= stars;
	prev = NULL;
	for (entry = list->head; entry; entry = entry->next) {
		if (len >= entry->len) break;
		prev = entry;
	}
	entry = malloc(sizeof(*entry));
	entry->ircmask = strdup(ircmask);
	entry->len = len;
	entry->data = data;
	if (prev) {
		entry->next = prev->next;
		prev->next = entry;
	}
	else {
		entry->next = list->head;
		list->head = entry;
	}
	return(0);
}

int ircmask_list_del(ircmask_list_t *list, const char *ircmask, void *data)
{
	putlog(LOG_DEBUG, "*", "implement me %s:%i, ircmask_list_del()", 
			__FILE__, 
			__LINE__
	);
	return (-1);
}

int ircmask_list_find(ircmask_list_t *list, const char *irchost, void *dataptr)
{
	ircmask_list_entry_t *entry;

	for (entry = list->head; entry; entry = entry->next) {
		if (wild_match(entry->ircmask, irchost) > 0) {
			*(void **)dataptr = entry->data;
			return(0);
		}
	}
	*(void **)dataptr = NULL;
	return(-1);
}
