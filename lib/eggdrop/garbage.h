#ifndef _GARBAGE_H_
#define _GARBAGE_H_

#define GARBAGE_ONCE	1

typedef int (*garbage_proc_t)(void *client_data);
int garbage_add(garbage_proc_t garbage_proc, void *client_data, int flags);
int garbage_run();

#endif
