/* See header file avl.h for licensing info. */

#include <stdio.h>
#include <stdlib.h>
#include "mempool.h"
#include "avl.h"

static mempool_t *avl_node_mempool;

#define NEW_NODE ((avl_node_t *)mempool_get_chunk(avl_node_mempool));

avl_tree_t *avl_create(avl_tree_t * tree, avl_comparison_func cmp, void *param)
{
	avl_tree_t *mytree;

	/* See if mempool needs to be initialized. */
	if (!avl_node_mempool) {
		avl_node_mempool = mempool_create(NULL, 10, sizeof(avl_node_t));
	}

	if (tree) mytree = tree;
	else mytree = (avl_tree_t *) malloc(sizeof(*tree));

	mytree->root.link[0] = NULL;
	mytree->root.link[1] = NULL;
	mytree->cmp = cmp;
	mytree->count = 0;
	mytree->param = param;

	return (mytree);
}

/* Destroy tree TREE. Function FREE_FUNC is called for every node in
 the tree as it is destroyed.	

 Do not attempt to reuse the tree after it has been freed. Create a
 new one.	*/
void avl_destroy(avl_tree_t * tree, avl_node_func free_func)
{
	/* Uses Knuth's Algorithm 2.3.1T as modified in exercise 13 (postorder traversal). */

	/* T1. */
	avl_node_t *an[AVL_MAX_HEIGHT];	/* Stack A: nodes. */
	char ab[AVL_MAX_HEIGHT];	/* Stack A: bits. */
	int ap = 0;					/* Stack A: height. */
	avl_node_t *p = tree->root.link[0];

	for (;;) {
		/* T2. */
		while (p != NULL) {
			/* T3. */
			ab[ap] = 0;
			an[ap++] = p;
			p = p->link[0];
		}

		/* T4. */
		for (;;) {
			if (ap == 0) goto done;

			p = an[--ap];
			if (ab[ap] == 0) {
				ab[ap++] = 1;
				p = p->link[1];
				break;
			}

			if (free_func) free_func(p->data, tree->param);
			mempool_free_chunk(avl_node_mempool, p);
		}
	}

  done:
	free(tree);
}

/* avl_destroy() with FREE_FUNC hardcoded as free(). */
void avl_free(avl_tree_t * tree)
{
	avl_destroy(tree, (avl_node_func) free);
}

/* Return the number of nodes in TREE. */
int avl_count(const avl_tree_t * tree)
{
	return tree->count;
}

void avl_walk(const avl_tree_t * tree, avl_node_func walk_func, void *param)
{
	/* Uses Knuth's algorithm 2.3.1T (inorder traversal). */

	/* T1. */
	const avl_node_t *an[AVL_MAX_HEIGHT];	/* Stack A: nodes. */
	const avl_node_t **ap = an;	/* Stack A: stack pointer. */
	const avl_node_t *p = tree->root.link[0];

	for (;;) {
		/* T2. */
		while (p != NULL) {
			/* T3. */
			*ap++ = p;
			p = p->link[0];
		}

		/* T4. */
		if (ap == an) return;
		p = *--ap;

		/* T5. */
		walk_func(p->data, param);
		p = p->link[1];
	}
}

/* Each call to this function for a given TREE and TRAV return the
	 next item in the tree in inorder.	Initialize the first element of
	 TRAV (init) to 0 before calling the first time.	Returns NULL when
	 out of elements.	*/
void *avl_traverse(const avl_tree_t * tree, avl_traverser_t * trav)
{

	/* Uses Knuth's algorithm 2.3.1T (inorder traversal). */
	if (trav->init == 0) {
		/* T1. */
		trav->init = 1;
		trav->nstack = 0;
		trav->p = tree->root.link[0];
	}
	else trav->p = trav->p->link[1]; /* T5. */

	/* T2. */
	while (trav->p != NULL) {
		/* T3. */
		trav->stack[trav->nstack++] = trav->p;
		trav->p = trav->p->link[0];
	}

	/* T4. */
	if (trav->nstack == 0) {
		trav->init = 0;
		return NULL;
	}
	trav->p = trav->stack[--trav->nstack];

	/* T5. */
	return trav->p->data;
}

/* Search TREE for an item matching ITEM.	If found, returns a pointer
	 to the address of the item.	If none is found, ITEM is inserted
	 into the tree, and a pointer to the address of ITEM is returned.
	 In either case, the pointer returned can be changed by the caller,
	 or the returned data item can be directly edited, but the key data
	 in the item must not be changed. */
void **avl_probe(avl_tree_t * tree, void *item)
{
	/* Uses Knuth's Algorithm 6.2.3A (balanced tree search and
	   insertion), but caches results of comparisons.   In empirical
	   tests this eliminates about 25% of the comparisons seen under
	   random insertions.   */

	/* A1. */
	avl_node_t *t;
	avl_node_t *s, *p, *q, *r;

	t = &tree->root;
	s = p = t->link[0];

	if (s == NULL) {
		tree->count++;
		q = t->link[0] = NEW_NODE;
		q->data = item;
		q->link[0] = q->link[1] = NULL;
		q->bal = 0;
		return &q->data;
	}

	for (;;) {
		/* A2. */
		int diff = tree->cmp(item, p->data, tree->param);

		/* A3. */
		if (diff < 0) {
			p->cache = 0;
			q = p->link[0];
			if (q == NULL) {
				p->link[0] = q = NEW_NODE;
				break;
			}
		}
		/* A4. */
		else if (diff > 0) {
			p->cache = 1;
			q = p->link[1];
			if (q == NULL) {
				p->link[1] = q = NEW_NODE;
				break;
			}
		}
		else					/* A2. */
			return &p->data;

		/* A3, A4. */
		if (q->bal != 0) t = p, s = q;
		p = q;
	}

	/* A5. */
	tree->count++;
	q->data = item;
	q->link[0] = q->link[1] = NULL;
	q->bal = 0;

	/* A6. */
	r = p = s->link[(int)s->cache];
	while (p != q) {
		p->bal = p->cache * 2 - 1;
		p = p->link[(int)p->cache];
	}

	/* A7. */
	if (s->cache == 0) {
		/* a = -1. */
		if (s->bal == 0) {
			s->bal = -1;
			return &q->data;
		}
		else if (s->bal == +1) {
			s->bal = 0;
			return &q->data;
		}

		if (r->bal == -1) {
			/* A8. */
			p = r;
			s->link[0] = r->link[1];
			r->link[1] = s;
			s->bal = r->bal = 0;
		}
		else {
			/* A9. */
			p = r->link[1];
			r->link[1] = p->link[0];
			p->link[0] = r;
			s->link[0] = p->link[1];
			p->link[1] = s;
			if (p->bal == -1) s->bal = 1, r->bal = 0;
			else if (p->bal == 0) s->bal = r->bal = 0;
			else s->bal = 0, r->bal = -1;
			p->bal = 0;
		}
	}
	else {
		/* a == +1. */
		if (s->bal == 0) {
			s->bal = 1;
			return &q->data;
		}
		else if (s->bal == -1) {
			s->bal = 0;
			return &q->data;
		}

		if (r->bal == +1) {
			/* A8. */
			p = r;
			s->link[1] = r->link[0];
			r->link[0] = s;
			s->bal = r->bal = 0;
		}
		else {
			/* A9. */
			p = r->link[0];
			r->link[0] = p->link[1];
			p->link[1] = r;
			s->link[1] = p->link[0];
			p->link[0] = s;
			if (p->bal == +1) s->bal = -1, r->bal = 0;
			else if (p->bal == 0) s->bal = r->bal = 0;
			else s->bal = 0, r->bal = 1;
			p->bal = 0;
		}
	}

	/* A10. */
	if (t != &tree->root && s == t->link[1]) t->link[1] = p;
	else t->link[0] = p;

	return &q->data;
}

/* Search TREE for an item matching ITEM, and return it if found. */
void *avl_find(const avl_tree_t * tree, const void *item)
{
	const avl_node_t *p;

	for (p = tree->root.link[0]; p;) {
		int diff = tree->cmp(item, p->data, tree->param);

		if (diff < 0) p = p->link[0];
		else if (diff > 0) p = p->link[1];
		else return p->data;
	}

	return NULL;
}

/* Search TREE for an item close to the value of ITEM, and return it.
	 This function will return a null pointer only if TREE is empty. */
void *avl_find_close(const avl_tree_t * tree, const void *item)
{
	const avl_node_t *p;

	p = tree->root.link[0];
	if (p == NULL) return NULL;

	for (;;) {
		int diff = tree->cmp(item, p->data, tree->param);
		int t;

		if (diff < 0) t = 0;
		else if (diff > 0) t = 1;
		else return p->data;

		if (p->link[t]) p = p->link[t];
		else return p->data;
	}
}

/* Searches AVL tree TREE for an item matching ITEM.	If found, the
	 item is removed from the tree and the actual item found is returned
	 to the caller.	If no item matching ITEM exists in the tree,
	 returns NULL. */
void *avl_delete(avl_tree_t * tree, const void *item)
{
	/* Uses my Algorithm D, which can be found at
	   http://www.msu.edu/user/pfaffben/avl.    Algorithm D is based on
	   Knuth's Algorithm 6.2.2D (Tree deletion) and 6.2.3A (Balanced
	   tree search and insertion), as well as the notes on pages 465-466
	   of Vol. 3. */

	/* D1. */
	avl_node_t *pa[AVL_MAX_HEIGHT];	/* Stack P: Nodes. */
	char a[AVL_MAX_HEIGHT];		/* Stack P: Bits. */
	int k = 1;					/* Stack P: Pointer. */

	avl_node_t **q;
	avl_node_t *p;

	a[0] = 0;
	pa[0] = &tree->root;
	p = tree->root.link[0];
	for (;;) {
		/* D2. */
		int diff;

		if (p == NULL) return NULL;

		diff = tree->cmp(item, p->data, tree->param);
		if (diff == 0) break;

		/* D3, D4. */
		pa[k] = p;
		if (diff < 0) {
			p = p->link[0];
			a[k] = 0;
		}
		else if (diff > 0) {
			p = p->link[1];
			a[k] = 1;
		}
		k++;
	}
	tree->count--;

	item = p->data;

	/* D5. */
	q = &pa[k - 1]->link[(int)a[k - 1]];
	if (p->link[1] == NULL) {
		*q = p->link[0];
		if (*q) (*q)->bal = 0;
	}
	else {
		/* D6. */
		avl_node_t *r = p->link[1];
		if (r->link[0] == NULL) {
			r->link[0] = p->link[0];
			*q = r;
			r->bal = p->bal;
			a[k] = 1;
			pa[k++] = r;
		}
		else {
			/* D7. */
			avl_node_t *s = r->link[0];
			int l = k++;

			a[k] = 0;
			pa[k++] = r;

			/* D8. */
			while (s->link[0] != NULL) {
				r = s;
				s = r->link[0];
				a[k] = 0;
				pa[k++] = r;
			}

			/* D9. */
			a[l] = 1;
			pa[l] = s;
			s->link[0] = p->link[0];
			r->link[0] = s->link[1];
			s->link[1] = p->link[1];
			s->bal = p->bal;
			*q = s;
		}
	}

	mempool_free_chunk(avl_node_mempool, p);

	/* D10. */
	while (--k) {
		avl_node_t *s = pa[k], *r;

		if (a[k] == 0) {
			/* D10. */
			if (s->bal == -1) {
				s->bal = 0;
				continue;
			}
			else if (s->bal == 0) {
				s->bal = 1;
				break;
			}

			r = s->link[1];

			if (r->bal == 0) {
				/* D11. */
				s->link[1] = r->link[0];
				r->link[0] = s;
				r->bal = -1;
				pa[k - 1]->link[(int)a[k - 1]] = r;
				break;
			}
			else if (r->bal == +1) {
				/* D12. */
				s->link[1] = r->link[0];
				r->link[0] = s;
				s->bal = r->bal = 0;
				pa[k - 1]->link[(int)a[k - 1]] = r;
			}
			else {
				/* D13. */
				p = r->link[0];
				r->link[0] = p->link[1];
				p->link[1] = r;
				s->link[1] = p->link[0];
				p->link[0] = s;
				if (p->bal == +1) s->bal = -1, r->bal = 0;
				else if (p->bal == 0) s->bal = r->bal = 0;
				else s->bal = 0, r->bal = +1;
				p->bal = 0;
				pa[k - 1]->link[(int)a[k - 1]] = p;
			}
		}
		else {

			/* D10. */
			if (s->bal == +1) {
				s->bal = 0;
				continue;
			}
			else if (s->bal == 0) {
				s->bal = -1;
				break;
			}

			r = s->link[0];

			if (r == NULL || r->bal == 0) {
				/* D11. */
				s->link[0] = r->link[1];
				r->link[1] = s;
				r->bal = 1;
				pa[k - 1]->link[(int)a[k - 1]] = r;
				break;
			}
			else if (r->bal == -1) {
				/* D12. */
				s->link[0] = r->link[1];
				r->link[1] = s;
				s->bal = r->bal = 0;
				pa[k - 1]->link[(int)a[k - 1]] = r;
			}
			else if (r->bal == +1) {
				/* D13. */
				p = r->link[1];
				r->link[1] = p->link[0];
				p->link[0] = r;
				s->link[0] = p->link[1];
				p->link[1] = s;
				if (p->bal == -1) s->bal = 1, r->bal = 0;
				else if (p->bal == 0) s->bal = r->bal = 0;
				else s->bal = 0, r->bal = -1;
				p->bal = 0;
				pa[k - 1]->link[(int)a[k - 1]] = p;
			}
		}
	}

	return (void *)item;
}

/* Inserts ITEM into TREE.	Returns NULL if the item was inserted,
	 otherwise a pointer to the duplicate item. */
void *avl_insert(avl_tree_t * tree, void *item)
{
	void **p;

	p = avl_probe(tree, item);
	return (*p == item) ? NULL : *p;
}

/* If ITEM does not exist in TREE, inserts it and returns NULL.	If a
	 matching item does exist, it is replaced by ITEM and the item
	 replaced is returned.	The caller is responsible for freeing the
	 item returned. */
void *avl_replace(avl_tree_t * tree, void *item)
{
	void **p;

	p = avl_probe(tree, item);
	if (*p == item) return NULL;
	else {
		void *r = *p;
		*p = item;
		return r;
	}
}
