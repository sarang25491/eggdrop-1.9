/* libavl - manipulates AVL trees.
   Copyright (C) 1998, 1999 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   The author may be contacted at <pfaffben@pilot.msu.edu> on the
   Internet, or as Ben Pfaff, 12167 Airport Rd, DeWitt MI 48820, USA
   through more mundane means. */

/* This is file avl.h in libavl. */

#ifndef _AVL_H_
#define _AVL_H_

/* The default maximum height of 32 allows for AVL trees having
   between 5,704,880 and 4,294,967,295 nodes, depending on order of
   insertion.  You may change this compile-time constant as you
   wish. */
#ifndef AVL_MAX_HEIGHT
#define AVL_MAX_HEIGHT	32
#endif

/* Structure for a node in an AVL tree. */
typedef struct avl_node_b {
    void *data;			/* Pointer to data. */
    struct avl_node_b *link[2];	/* Subtrees. */
    signed char bal;		/* Balance factor. */
    char cache;			/* Used during insertion. */
    signed char pad[2];		/* Unused.  Reserved for threaded trees. */
} avl_node_t;

/* Used for traversing an AVL tree. */
typedef struct avl_traverser_b {
    int init;			/* Initialized? */
    int nstack;			/* Top of stack. */
    const avl_node_t *p;		/* Used for traversal. */
    const avl_node_t *stack[AVL_MAX_HEIGHT];/* Descended trees. */
} avl_traverser_t;

/* Function types. */
typedef int (*avl_comparison_func) (const void *a, const void *b, void *param);
typedef void (*avl_node_func) (void *data, void *param);

/* Structure which holds information about an AVL tree. */
typedef struct avl_tree_b {
    avl_node_t root;		/* Tree root node. */
    avl_comparison_func cmp;	/* Used to compare keys. */
    int count;			/* Number of nodes in the tree. */
    void *param;		/* Arbitary user data. */
} avl_tree_t;

/* General functions. */
avl_tree_t *avl_create (avl_tree_t *tree, avl_comparison_func, void *param);
void avl_destroy (avl_tree_t *, avl_node_func);
void avl_free (avl_tree_t *);
int avl_count (const avl_tree_t *);

/* Walk the tree. */
void avl_walk (const avl_tree_t *, avl_node_func, void *param);
void *avl_traverse (const avl_tree_t *, avl_traverser_t *);
#define avl_init_traverser(TRAVERSER) ((TRAVERSER)->init = 0)

/* Search for a given item. */
void **avl_probe (avl_tree_t *, void *);
void *avl_delete (avl_tree_t *, const void *);
void *avl_find (const avl_tree_t *, const void *);
void *avl_find_close (const avl_tree_t *, const void *);

/* Insert/replace items. */
void *avl_insert (avl_tree_t *tree, void *item);
void *avl_replace (avl_tree_t *tree, void *item);

#endif /* avl_h */
