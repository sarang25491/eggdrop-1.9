/* Implement a stack based on malloc. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mstack.h"

mstack_t *mstack_new(int initial_size)
{
	mstack_t *m;

	if (initial_size <= 0) initial_size = 10;
	m = (mstack_t *)calloc(1, sizeof(mstack_t) + sizeof(int) * initial_size);
	m->len = 0;
	m->max = initial_size;
	m->stack = ((int *)m)+3;
	return(m);
}

int mstack_destroy(mstack_t *m)
{
	if (m->stack != ((int *)m)+3) free(m->stack);
	free(m);
	return(0);
}

void *mstack_push(mstack_t *m, void *item)
{
	if (m->len == m->max) mstack_grow(m, 10);
	m->stack[m->len] = (int) item;
	m->len++;
	return(item);
}

int mstack_pop(mstack_t *m, void **itemptr)
{
	if (m->len == 0) return(1);
	m->len--;
	*itemptr = (void *)m->stack[m->len];
	return(0);
}

int mstack_grow(mstack_t *m, int nsteps)
{
	if (m->stack == ((int *)m)+3) {
		int *newstack;

		newstack = (int *)malloc(sizeof(int) * (m->max + nsteps));
		memcpy(newstack, m->stack, sizeof(int) * m->max);
		m->stack = newstack;
	}
	else {
		m->stack = (int *)realloc(m->stack, sizeof(int) * (m->max + nsteps));
		memset(m->stack+m->max, 0, sizeof(int) * nsteps);
	}

	m->max += nsteps;
	return(0);
}
