#ifndef _IRCMASKS_H_
#define _IRCMASKS_H_

typedef struct ircmask_list_entry {
	struct ircmask_list_entry *next;
	char *ircmask;
	int len;
	void *data;
} ircmask_list_entry_t;

typedef struct {
	ircmask_list_entry_t *head;
} ircmask_list_t;

int ircmask_list_add(ircmask_list_t *list, const char *ircmask, void *data);
int ircmask_list_del(ircmask_list_t *list, const char *ircmask, void *data);
int ircmask_list_find(ircmask_list_t *list, const char *irchost, void *dataptr);

#endif
