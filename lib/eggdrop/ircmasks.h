#ifndef _IRCMASKS_H_
#define _IRCMASKS_H_

typedef struct {
	char *ircmask;
	int len;
	void *data;
} ircmask_list_entry_t;

typedef struct {
	int len;
	ircmask_list_entry_t *list;
} ircmask_list_t;

int ircmask_list_add(ircmask_list_t *list, const char *ircmask, void *data);
int ircmask_list_del(ircmask_list_t *list, const char *ircmask, void *data);
int ircmask_list_find(ircmask_list_t *list, const char *irchost, void *dataptr);

#endif
