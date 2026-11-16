#ifndef BST_H
#define BST_H

#include <stddef.h>

#include "vector.h"

/*
 * Unbalanced binary search tree (a sorted map).
 *
 * Keys are compared with a function you pass to bst_init, so any key type
 * works. Keys are unique: inserting an existing key replaces its value.
 *
 * Ownership: the tree owns its nodes but NOT the keys or values. Both are
 * stored as plain pointers, so they must stay alive as long as they are in
 * the tree.
 *
 * It is NOT balanced. Inserting keys in sorted order makes it a linked list
 * with O(n) lookups (the benchmark shows this). See the README for why I
 * stopped here. All the operations are written as loops rather than
 * recursion so that a very deep tree cannot overflow the C stack.
 */

typedef int (*bst_cmp_fn)(const void *a, const void *b); /* <0, 0, >0 */

typedef struct BstNode {
    const void *key;
    void *value;
    struct BstNode *left;
    struct BstNode *right;
} BstNode;

typedef struct {
    BstNode *root;
    size_t len;
    bst_cmp_fn cmp;
} Bst;

void bst_init(Bst *t, bst_cmp_fn cmp);
void bst_free(Bst *t);

/* 1 if new, 0 if the value of an existing key was replaced, -1 on OOM. */
int bst_insert(Bst *t, const void *key, void *value);
int bst_find(const Bst *t, const void *key, void **out); /* 1 found, 0 not */
int bst_remove(Bst *t, const void *key, void **out);     /* 1 removed, 0 not */

size_t bst_len(const Bst *t);
size_t bst_height(const Bst *t); /* 0 for an empty tree, 1 for one node */
int bst_min(const Bst *t, const void **key, void **value); /* 0, or -1 if empty */
int bst_max(const Bst *t, const void **key, void **value);

/*
 * In-order iterator (ascending key order). It keeps an explicit stack of the
 * nodes still to visit, so it uses O(height) memory. Do not change the tree
 * while iterating. Call bst_iter_free when done, even after an early break.
 *
 *     BstIter it;
 *     bst_iter_init(&it, &tree);
 *     while (bst_iter_next(&it, &key, &value)) { ... }
 *     bst_iter_free(&it);
 *
 * bst_iter_next returns 1 for each item, 0 at the end, and -1 if pushing on
 * the stack failed to allocate.
 */
typedef struct {
    Vec stack; /* of BstNode * */
    BstNode *next;
} BstIter;

void bst_iter_init(BstIter *it, const Bst *t);
int bst_iter_next(BstIter *it, const void **key, void **value);
void bst_iter_free(BstIter *it);

#endif
