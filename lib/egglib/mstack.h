#ifndef _MSTACK_H_
#define _MSTACK_H_

typedef struct mstack_b {
	int len;
	int max;
	void **stack;
} mstack_t;

mstack_t *mstack_new(int initial_size);
int mstack_destroy(mstack_t *m);
void *mstack_push(mstack_t *m, void *item);
int mstack_pop(mstack_t *m, void *itemptr);
int mstack_grow(mstack_t *m, int nsteps);

#endif
